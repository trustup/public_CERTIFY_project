/**
  ******************************************************************************
  * @file    stack_analysis.c
  * @author  MCD Application Team
  * @brief   This file implements Stack analysis debug facilities
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stack_analysis.h"

#if (USE_STACK_ANALYSIS == 1)

#include <stdio.h>
#include <string.h>
#include "rtosal.h"

#include "error_handler.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif /* (USE_CMD_CONSOLE == 1) */


/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_MAIN, DBL_LVL_P0, "" format "\n\r", ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "SA:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  \
  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "SA:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  \
  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "SA ERROR:" format "\n\r", ## args)
#else
#define PRINT_FORCE(format, args...) (void)printf("" format "\n\r", ## args);
#define PRINT_INFO(format, args...)  (void)printf("SA:" format "\n\r", ## args);
#define PRINT_DBG(...)               __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void)printf("SA ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF  == 0U */

#define STACK_ANALYSIS_MIN(a,b) (((a) < (b)) ? (a) : (b))

/* Private typedef -----------------------------------------------------------*/
typedef char SA_CHAR_t; /* used in stdio.h and string.h service call */

/* Task analysis list item structure - used to save the task data */
typedef struct
{
  void *TaskHandle;
  uint16_t stackSizeAtCreation;
  uint16_t stackSizeFree;
} TaskAnalysis_t;

/* Private define ------------------------------------------------------------*/
/* Characters definition to display the task state */
#define TSK_RUNNING_CHAR   (SA_CHAR_t)('X')
#define TSK_READY_CHAR     (SA_CHAR_t)('R')
#define TSK_BLOCKED_CHAR   (SA_CHAR_t)('B')
#define TSK_SUSPENDED_CHAR (SA_CHAR_t)('S')
#define TSK_DELETED_CHAR   (SA_CHAR_t)('D')
#define TSK_INVALID_CHAR   (SA_CHAR_t)('I')
#define TSK_UNKNOWN_CHAR   (SA_CHAR_t)(' ')

/* Private variables ---------------------------------------------------------*/
/* Mutex to protect access to TaskAnalysisList when add a new entry */
static osMutexId StackAnalysisMutexHandle = NULL;
/* Task Analysis List */
static TaskAnalysis_t TaskAnalysisList[THREAD_NUMBER];
/* Number of Tasks currently defined in the task analysis list*/
static uint8_t TaskAnalysisListNb = 0U;

#if (USE_CMD_CONSOLE == 1)
static uint8_t *stackAnalysis_cmd_label = (uint8_t *)"stack";
#endif /* USE_CMD_CONSOLE == 1 */

#if (STACK_ANALYSIS_TIMER != 0U)
/* When timer call back function is called a message is send to the queue to ask to trace the tasks stack */
static osMessageQId StackAnalysisQueueId = NULL;
static osTimerId StackAnalysisTimerId = NULL;
#endif /* STACK_ANALYSIS_TIMER != 0U */

/* Format of stack analysis trace */
static sa_print_format_t sa_next_print_format;

#if (SA_TRACE_ONLY_THE_CHANGE == 1)
static bool sa_next_force; /* force to trace all stack even if SA_TRACE_ONLY_THE_CHANGE is activated */
#endif /* SA_TRACE_ONLY_THE_CHANGE == 1 */

/* Private function prototypes -----------------------------------------------*/
static bool getStackSizeByHandle(
  const TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static bool getStackSizeByName(
  const SA_CHAR_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree);

static void formatTaskName(SA_CHAR_t *pcBuffer,
                           size_t size_max,
                           const SA_CHAR_t *pcTaskName);

#if (USE_CMD_CONSOLE == 1)
static cmd_status_t stackAnalysis_cmd(uint8_t *cmd_line_p);
#endif /* USE_CMD_CONSOLE == 1 */

#if (STACK_ANALYSIS_TIMER != 0U)
/* In case a periodical display is activated,
   a task is needed to received the message of the timer and to display the task stack status
   callback is not used to do this because display is using freertos functions */
static void stack_analysis_thread(void *p_argument);

static void stack_analysis_timer_cb(void *p_argument);
#endif /* STACK_ANALYSIS_TIMER != 0U */

/* Private functions ---------------------------------------------------------*/


#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void stackAnalysis_cmd_help(void)
{
  CMD_print_help((uint8_t *)"Stack Analysis");
  PRINT_FORCE("%s display              : display task infos according internal settings", stackAnalysis_cmd_label)
  PRINT_FORCE("%s display all          : display all task infos whatever internal settings", stackAnalysis_cmd_label)
  PRINT_FORCE("%s format [print|valid] : set the display format to print or validation", stackAnalysis_cmd_label)
  PRINT_FORCE("%s help                 : display this help", stackAnalysis_cmd_label)
}

/**
  * @brief  cmd management
  * @param  cmd_line_p - command parameters
  * @retval cmd_status_t - status of cmd management
  */
static cmd_status_t stackAnalysis_cmd(uint8_t *cmd_line_p)
{
  uint32_t argc;
  uint8_t  *argv_p[10];
  const uint8_t *cmd_p;

  PRINT_FORCE()

  cmd_p = (uint8_t *)strtok((SA_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (strncmp((const SA_CHAR_t *)cmd_p,
                (const SA_CHAR_t *)stackAnalysis_cmd_label,
                strlen((const SA_CHAR_t *)cmd_p))
        == 0)
    {
      /* parameters parsing */
      for (argc = 0U; argc < 10U; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          break;
        }
      }

      if (argc == 0U)
      {
        PRINT_FORCE("SA: Missing parameters. Usage:")
        stackAnalysis_cmd_help();
      }
      /*  1st parameter analysis */
      else if (strncmp((SA_CHAR_t *)argv_p[0],
                       "display",
                       strlen((const SA_CHAR_t *)argv_p[0]))
               == 0)
      {
        if (argc == 2U)
        {
          if (strncmp((SA_CHAR_t *)argv_p[1],
                      "all",
                      strlen((const SA_CHAR_t *)argv_p[1]))
              == 0)
          {
#if (SA_TRACE_ONLY_THE_CHANGE == 1)
            sa_next_force = true;
            (void)stackAnalysis_trace();
            sa_next_force = false;
#else /* SA_TRACE_ONLY_THE_CHANGE == 0 */
            (void)stackAnalysis_trace();
#endif /* SA_TRACE_ONLY_THE_CHANGE == 1 */
          }
          else
          {
            PRINT_FORCE("SA: Unrecognised parameter \"%s\". Usage:", (SA_CHAR_t *)argv_p[1])
            stackAnalysis_cmd_help();
          }
        }
        else
        {
          (void)stackAnalysis_trace();
        }
      }
      else if (strncmp((SA_CHAR_t *)argv_p[0],
                       "format",
                       strlen((const SA_CHAR_t *)argv_p[0]))
               == 0)
      {
        if (argc == 2U)
        {
          if (strncmp((SA_CHAR_t *)argv_p[1],
                      "print",
                      strlen((const SA_CHAR_t *)argv_p[1]))
              == 0)
          {
            sa_next_print_format = STACK_ANALYSIS_PRINT;
            PRINT_FORCE("SA: Display format set to print")
          }
          else if (strncmp((SA_CHAR_t *)argv_p[1],
                           "valid",
                           strlen((const SA_CHAR_t *)argv_p[1]))
                   == 0)
          {
            sa_next_print_format = STACK_ANALYSIS_VALID;
            PRINT_FORCE("SA: Display format set to validation")
          }
          else
          {
            PRINT_FORCE("SA: Unrecognised parameter \"%s\". Usage:", (SA_CHAR_t *)argv_p[1])
            stackAnalysis_cmd_help();
          }
        }
        else
        {
          PRINT_FORCE("SA: <format> parameter must be provided")
          stackAnalysis_cmd_help();
        }
      }
      else if (strncmp((SA_CHAR_t *)argv_p[0],
                       "help",
                       strlen((const SA_CHAR_t *)argv_p[0])) == 0)
      {
        stackAnalysis_cmd_help();
      }
      else
      {
        PRINT_FORCE("SA: Unrecognised parameter \"%s\". Usage:", (SA_CHAR_t *)argv_p[0])
        stackAnalysis_cmd_help();
      }
    }
  }
  return CMD_OK;
}
#endif /* USE_CMD_CONSOLE == 1 */

/**
  * @brief  Using TaskHandle to find the task, get stack size at creation and last saved size free
  * @param  TaskHandle - Task handle to search
  * @param  stackSizeAtCreation - return the stack size at its creation
  * @param  stackSizeFree - return the last saved stack size free
  * @param  indice - return the index in the task analysis list of the task found
  * @retval bool - false: no task found / true: task found
  */
static bool getStackSizeByHandle(
  const TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  bool found;

  i = 0U;
  found = false;
  while ((found == false)
         && (i < TaskAnalysisListNb))
  {
    if (TaskHandle == TaskAnalysisList[i].TaskHandle)
    {
      found = true;
      /* Update the output parameters */
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

/**
  * @brief  Using TaskName to find the task, get stack size at creation and last saved size free
  * @param  TaskName - Task name to search
  * @param  stackSizeAtCreation - return the stack size at its creation
  * @param  stackSizeFree - return the last saved stack size free
  * @param  indice - return the index in the task analysis list of the task found
  * @retval bool - false: no task found / true: task found
  */
static bool getStackSizeByName(
  const SA_CHAR_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  bool found;

  i = 0U;
  found = false;

  while ((found == false)
         && (i < TaskAnalysisListNb))
  {
    if (strcmp(TaskName,
               (const SA_CHAR_t *)TaskAnalysisList[i].TaskHandle)
        == 0)
    {
      found = true;
      /* Update the output parameters */
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

/**
  * @brief  Using indice in the TaskAnalysisList, update the stack size free of the task
  * @param  indice - index in the task analysis list of the task to update
  * @param  stackSizeFree - stack size free to save
  * @retval -
  */
static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree)
{
  if (indice < TaskAnalysisListNb)
  {
    /* Update stack size free with its last value */
    TaskAnalysisList[indice].stackSizeFree = stackSizeFree;
  }
}

/**
  * @brief  Format a task name according to a size (space padding is added if name is shorter)
  * @param  pcBuffer - pointer to the task name buffer that will contain the result
  * @param  size_max - maximum size of the string result (-1 to take into account \0)
  * @param  pcTaskName - pointer to the task name to format
  * @retval -
  */
static void formatTaskName(SA_CHAR_t *pcBuffer,
                           size_t size_max,
                           const SA_CHAR_t *pcTaskName)
{
  size_t i;

  /* Start by copying the entire string. */
  (void)memcpy(pcBuffer,
               pcTaskName,
               STACK_ANALYSIS_MIN((size_max - 1U),
                                  strlen(pcTaskName)));

  /* Pad the end of the string with spaces to ensure columns line up
     when printed out. */
  for (i = (size_t)(STACK_ANALYSIS_MIN((size_max - 1U),
                                       strlen(pcTaskName)));
       i < (size_max - 1U);
       i++)
  {
    pcBuffer[i] = (SA_CHAR_t)(' ');
  }
  /* Terminate. */
  pcBuffer[i] = (SA_CHAR_t)(0x00);
}

#if (STACK_ANALYSIS_TIMER != 0U)
/**
  * @brief  Callback called when Timer raised
  * @note   Managed Stack Analysis Timer
  * @param  p_argument - unused
  * @retval -
  */
static void stack_analysis_timer_cb(void *p_argument)
{
  UNUSED(p_argument);

  /* Request a trace by sending a message to the stack analysis thread */
  (void)rtosalMessageQueuePut(StackAnalysisQueueId, (uint32_t)0U, 0U);
}
#endif /* STACK_ANALYSIS_TIMER != 0U */

/* Public functions ----------------------------------------------------------*/
/**
  * @brief  Using TaskHandle to refer the task, add the task and its stack size at creation in the task List
  * @param  TaskHandle - Task handle to add
  * @param  stackSizeAtCreation - stack size at its creation
  * @retval bool - false: task not added in the list / true: task added in the list
  */
bool stackAnalysis_addStackSizeByHandle(
  void *TaskHandle,
  uint16_t stackSizeAtCreation)
{
  bool result;

  if (TaskHandle == NULL)
  {
    PRINT_INFO("Error: NULL TaskHandle passed")
    result = false;
  }
  else
  {
    (void)rtosalMutexAcquire(StackAnalysisMutexHandle, RTOSAL_WAIT_FOREVER);
    /* Enough space to save the data ? */
    if (TaskAnalysisListNb < THREAD_NUMBER)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskHandle;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      /* TaskAnalysisListNb incremented only at the end of the update of the list */
      TaskAnalysisListNb++;
      result = true;
    }
    else
    {
      PRINT_INFO("Error: Too many tasks added %d",
                 THREAD_NUMBER)
      result = false;
    }
    (void)rtosalMutexRelease(StackAnalysisMutexHandle);
  }

  return result;
}

/**
  * @brief  Using TaskName to refer the task, add the task and its stack size at creation in the task List
  * @param  TaskName - Task name to add
  * @param  stackSizeAtCreation - stack size at its creation
  * @retval bool - false: task not added in the list / true: task added in the list
  */
bool stackAnalysis_addStackSizeByName(
  uint8_t *TaskName,
  uint16_t stackSizeAtCreation)
{
  bool result;

  if (TaskName == NULL)
  {
    PRINT_INFO("Error: NULL TaskName passed")
    result = false;
  }
  else
  {
    (void)rtosalMutexAcquire(StackAnalysisMutexHandle, RTOSAL_WAIT_FOREVER);
    /* Enough space to save the data ? */
    if (TaskAnalysisListNb < THREAD_NUMBER)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskName;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      /* TaskAnalysisListNb incremented only at the end of the update of the list */
      TaskAnalysisListNb++;
      result = true;
    }
    else
    {
      PRINT_INFO("Error: Too many tasks added %d",
                 THREAD_NUMBER)
      result = false;
    }
    (void)rtosalMutexRelease(StackAnalysisMutexHandle);
  }

  return result;
}

/**
  * @brief  Trace a stack analysis
  * @param  -
  * @retval bool - false: trace not done (lack of memory) / true: trace done
  */
bool stackAnalysis_trace(void)
{
  static size_t GlobalstackSizeFree = TOTAL_HEAP_SIZE;

  bool result, found, heap_ok, stack_ok;
#if (SA_TRACE_ONLY_THE_CHANGE == 1)
  bool force = sa_next_force;
#endif /* SA_TRACE_ONLY_THE_CHANGE == 1 */

  sa_print_format_t print_format; /* format to use for the trace display */
  uint8_t indice;
  uint8_t stack_count_nok; /* number of tasks whose stack are nok */
  uint16_t usStackSizeAtCreation; /* task stack size at creation */
  uint16_t usStackSizeFree; /* task stack size still free */
  uint32_t ulTotalRunTime; /* total run time as defined in service uxTaskGetSystemState */
  uint32_t ulStatsAsPercentage; /* Percentage of the total run time used by a task */
  UBaseType_t previousTaskNumber;
  UBaseType_t uxArraySize; /* number of tasks defined in RTOS */
  SA_CHAR_t xTaskName[STACK_ANALYSIS_TASK_NAME_MAX]; /* task name to display */
  SA_CHAR_t cTaskStatus; /* Characters to display the task state */
  SA_CHAR_t *status[] = {"NOK", "OK"}; /* status of the heap task */
  size_t freeHeap1; /* current free heap size before allocation needed by stack analysis */
  size_t freeHeap2; /* free heap size after allocation done by stack analysis */
  TaskStatus_t *pxTaskStatusArray; /* pointer on all tasks status */
  TaskStatus_t *pxTaskStatusItem; /* pointer on one task item status */

  result = false;

  usStackSizeFree = 0U;
  ulTotalRunTime = 0U;
  uxArraySize = uxTaskGetNumberOfTasks();
  stack_count_nok = 0U;
  heap_ok = true;
  print_format = sa_next_print_format;

  /* Before allocation GetFreeHeapSize() */
  freeHeap1 = xPortGetFreeHeapSize();

  /* Allocate a TaskStatus_t structure for each task. */
  pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL)
  {
    /*
     * It is recommended that production systems call uxTaskGetSystemState()
     * directly to get access to raw stats data, rather than indirectly
     * through a call to vTaskList().
    */
    /* Generate raw status information about each task. */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
                                       uxArraySize,
                                       &ulTotalRunTime);

    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;

    /* After allocation GetFreeHeapSize() */
    freeHeap2 = xPortGetFreeHeapSize();

    if (print_format == STACK_ANALYSIS_PRINT)
    {
      PRINT_FORCE("<< Status  Begin >>")
      PRINT_FORCE("GlobalHeap : %d=>%d (init:%d) (%d SA usage)",
                  GlobalstackSizeFree,
                  freeHeap1,
                  TOTAL_HEAP_SIZE,
                  (freeHeap1 - freeHeap2))
      PRINT_FORCE("Task number:%lu", uxArraySize)
    }
    else
    {
      TRACE_VALID("@valid@:stack:size:status:start\n\r")
      /* Is total free heap still ok ? */
      if (freeHeap1
          < (((TOTAL_HEAP_SIZE) * SA_WARNING_IF_GLOBAL_HEAP_FREE_AT_X_PERCENT) / 100U))
      {
        heap_ok = false;
      }
      TRACE_VALID("@valid@:stack:size:%d/%d:%s:GlobalHeap\n\r",
                  freeHeap1,
                  (TOTAL_HEAP_SIZE),
                  status[((heap_ok == false) ? 0 : 1)])
    }
    GlobalstackSizeFree = freeHeap1;

    /* For each populated position in the pxTaskStatusArray array,
       format the raw data as human readable ASCII data */
    previousTaskNumber = 0U;

    /* For each element in the array provided by RTOS */
    for (UBaseType_t i = 0U; i < uxArraySize; i++)
    {
      usStackSizeAtCreation = 0U;

      /* Ordering Task according to their xTaskNumber */

      /* Start - Restart the search at the beginning of the array */
      pxTaskStatusItem = &pxTaskStatusArray[0];

      /* Find the lowest xTaskNumber in the list */
      for (UBaseType_t j = 0U; j < uxArraySize; j++)
      {
        /* Don't take into account task number that were already displayed */
        if ((pxTaskStatusArray[j].xTaskNumber > previousTaskNumber)
            || (previousTaskNumber == 0U)) /* To not miss thread with TaskNumber = 0 */
        {
          /* A good candidate */
          if (pxTaskStatusItem->xTaskNumber <= previousTaskNumber)
          {
            pxTaskStatusItem = &pxTaskStatusArray[j]; /* point on this first correct element
                                                         but maybe not the better possible */
          }
          else if (pxTaskStatusArray[j].xTaskNumber < pxTaskStatusItem->xTaskNumber)
          {
            pxTaskStatusItem = &pxTaskStatusArray[j]; /* point on this better element */
          }
          else
          {
            __NOP(); /* this element is not better than the previous one found - skip it */
          }
        }
        /* Continue to search until the end of the array */
      }
      /* Update previousTaskNumber */
      previousTaskNumber = pxTaskStatusItem->xTaskNumber;

      /* Task status format */
      switch (pxTaskStatusItem->eCurrentState)
      {
        case eRunning:
          cTaskStatus = TSK_RUNNING_CHAR;
          break;
        case eReady:
          cTaskStatus = TSK_READY_CHAR;
          break;
        case eBlocked:
          cTaskStatus = TSK_BLOCKED_CHAR;
          break;
        case eSuspended:
          cTaskStatus = TSK_SUSPENDED_CHAR;
          break;
        case eDeleted:
          cTaskStatus = TSK_DELETED_CHAR;
          break;
        case eInvalid:
          cTaskStatus = TSK_INVALID_CHAR;
          break;
        default:
          /* Should not get here, but it is included to prevent static checking errors. */
          cTaskStatus = TSK_UNKNOWN_CHAR;
          break;
      }

      /* Avoid to divide by zero */
      if (ulTotalRunTime > 0U)
      {
        /* What percentage of the total run time has the task used?
           This will always be rounded down to the nearest integer.
           ulTotalRunTimeDiv100 has already been divided by 100. */
        ulStatsAsPercentage = pxTaskStatusItem->ulRunTimeCounter / ulTotalRunTime;
      }
      else
      {
        ulStatsAsPercentage = 0U; /* TotalRunTime not managed */
      }

      /* Format the task name according to the size max defined */
      formatTaskName(xTaskName,
                     (uint32_t)STACK_ANALYSIS_TASK_NAME_MAX,
                     (const SA_CHAR_t *)pxTaskStatusItem->pcTaskName);

      /* Search the task stack information by handle */
      found = getStackSizeByHandle((const TaskHandle_t *)pxTaskStatusItem->xHandle,
                                   &usStackSizeAtCreation,
                                   &usStackSizeFree,
                                   &indice);

      /* If not found search the task stack information by name */
      if (found == false)
      {
        found = getStackSizeByName((const SA_CHAR_t *)pxTaskStatusItem->pcTaskName,
                                   &usStackSizeAtCreation,
                                   &usStackSizeFree,
                                   &indice);
      }

      /* Print the information collected */
#if (SA_TRACE_ONLY_THE_CHANGE == 1)
      if ((force == true)
          || (found == false)
          || ((uint32_t)(pxTaskStatusItem->usStackHighWaterMark)
              < (((uint32_t)usStackSizeAtCreation * SA_WARNING_IF_THREAD_FREE_STACK_AT_X_PERCENT) / 100U))
          || (pxTaskStatusItem->usStackHighWaterMark < usStackSizeFree))
#else /* SA_TRACE_ONLY_THE_CHANGE == 0 */
      /* trace must always be displayed */
#endif /* SA_TRACE_ONLY_THE_CHANGE == 1 */
      {
        if (print_format == STACK_ANALYSIS_PRINT)
        {
          if (ulStatsAsPercentage > 0UL)
          {
            PRINT_FORCE("%s n:%2lu FreeHeap:%4u=>%4u (init:%u) Prio:%lu State:%c Time:%lu  %lu%% ",
                        xTaskName,
                        pxTaskStatusItem->xTaskNumber,
                        usStackSizeFree,
                        pxTaskStatusItem->usStackHighWaterMark,
                        usStackSizeAtCreation,
                        pxTaskStatusItem->uxCurrentPriority,
                        cTaskStatus,
                        pxTaskStatusItem->ulRunTimeCounter,
                        ulStatsAsPercentage)
          }
          else
          {
            /* If the percentage is zero here then the task has
               consumed less than 1% of the total run time. */
#if ( configGENERATE_RUN_TIME_STATS == 1 )
            PRINT_FORCE("%s n:%2u FreeHeap:%4u=>%4u (init:%u) Prio:%u State:%c Time:<1%%",
                        xTaskName,
                        pxTaskStatusItem->xTaskNumber,
                        usStackSizeFree,
                        pxTaskStatusItem->usStackHighWaterMark,
                        usStackSizeAtCreation,
                        pxTaskStatusItem->uxCurrentPriority,
                        cTaskStatus)
#else
            PRINT_FORCE("%s n:%2lu FreeHeap:%4u=>%4u (init:%u) Prio:%lu State:%c",
                        xTaskName,
                        pxTaskStatusItem->xTaskNumber,
                        usStackSizeFree,
                        pxTaskStatusItem->usStackHighWaterMark,
                        usStackSizeAtCreation,
                        pxTaskStatusItem->uxCurrentPriority,
                        cTaskStatus)
#endif /* configGENERATE_RUN_TIME_STATS == 1 */
          }
        }
        else
        {
          /* specific validation trace - update stack nok/ok counter */
          if (pxTaskStatusItem->usStackHighWaterMark
              < (usStackSizeAtCreation * (uint16_t)SA_WARNING_IF_THREAD_FREE_STACK_AT_X_PERCENT / (uint16_t)100))
          {
            stack_count_nok++;
            stack_ok = false;
          }
          else
          {
            stack_ok = true;
          }
          TRACE_VALID("@valid@:stack:size:%d/%d:%s:%s\n\r",
                      pxTaskStatusItem->usStackHighWaterMark,
                      usStackSizeAtCreation,
                      status[((stack_ok == false) ? 0 : 1)],
                      xTaskName)
        }
        if (found == true)
        {
          setStackSize(indice,
                       pxTaskStatusItem->usStackHighWaterMark);
        }
      }
    }

    /* Finalize the trace */
    if (print_format == STACK_ANALYSIS_VALID)
    {
      if ((stack_count_nok == 0U) && (heap_ok == true))
      {
        TRACE_VALID("@valid@:stack:size:end:status OK\n\r")
      }
      else
      {
        if (heap_ok == false)
        {
          stack_count_nok ++;
        }
        TRACE_VALID("@valid@:stack:size:end:status NOK(*%u)\n\r", stack_count_nok)
      }
    }
    else
    {
      PRINT_FORCE("<< Status End >>")
    }

    /* The array is no longer needed, free the memory it consumes. */
    vPortFree(pxTaskStatusArray);

    result = true;
  }
  else
  {
    PRINT_ERR("NOT enough memory available - task nb: %lu", uxArraySize)
  }

  return (result);
}

#if (STACK_ANALYSIS_TIMER != 0U)
/**
  * @brief  Stack analysis thread
  * @note   Infinite loop Stack Analysis body
  *         In case a periodical display is activated,
  *         a task is needed to received the message of the timer and to display the task stack status
  *         callback is not used to do this because display is using freertos functions
  * @param  p_argument - unused
  * @retval -
  */
static void stack_analysis_thread(void *p_argument)
{
  UNUSED(p_argument);

  uint32_t msg_queue = 0U;

  (void)rtosalTimerStart(StackAnalysisTimerId, STACK_ANALYSIS_TIMER);

  for (;;)
  {
    /* Wait instruction */
    (void)rtosalMessageQueueGet(StackAnalysisQueueId, &msg_queue, RTOSAL_WAIT_FOREVER);
    (void)stackAnalysis_trace();
  }
}
#endif /* STACK_ANALYSIS_TIMER != 0U */

/**
  * @brief  Set stack analysis print format
  * @note   Stack Analysis print format setting
  * @param  print_format - new format
  * @retval bool - false: bad format / true: format updated
  */
bool stackAnalysis_print_format_set(sa_print_format_t print_format)
{
  bool result;

  if ((print_format == STACK_ANALYSIS_PRINT)
      || (print_format == STACK_ANALYSIS_VALID))
  {
    /* Input ok update print format for the next trace */
    sa_next_print_format = print_format;
    result = true;
  }
  else
  {
    result = false;
  }

  return (result);
}

/**
  * @brief  Stack Analysis init
  * @note   Must be called before to any other services - Protected if called multiple times
  * @param  -
  * @retval -
  */
void stackAnalysis_init(void)
{
  sa_next_print_format = STACK_ANALYSIS_PRINT; /* default value */
#if (SA_TRACE_ONLY_THE_CHANGE == 1)
  sa_next_force = false;
#endif /* SA_TRACE_ONLY_THE_CHANGE == 1 */

  /* Multi call protection */
  if (StackAnalysisMutexHandle == NULL)
  {
    /* Initialize Mutex to protect task list descriptor access */
    StackAnalysisMutexHandle = rtosalMutexNew(NULL);
  }

#if (STACK_ANALYSIS_TIMER != 0U)
  if (StackAnalysisTimerId == NULL)
  {
    StackAnalysisTimerId = rtosalTimerNew(NULL, (os_ptimer)stack_analysis_timer_cb, osTimerPeriodic, NULL);
  }
  if (StackAnalysisQueueId == NULL)
  {
    StackAnalysisQueueId = rtosalMessageQueueNew(NULL, 2U);
  }
#endif /* STACK_ANALYSIS_TIMER != 0U */
}

/**
  * @brief  Stack Analysis start
  * @note   Must be called after stackAnalysis_init and before any other service
  * @param  -
  * @retval -
  */
void stackAnalysis_start(void)
{
#if (STACK_ANALYSIS_TIMER != 0U)
  static osThreadId StackAnalysisTaskHandle;
#endif /* STACK_ANALYSIS_TIMER != 0 */

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(stackAnalysis_cmd_label, stackAnalysis_cmd, (uint8_t *)"stack analysis");
#endif /* USE_CMD_CONSOLE == 1 */

#if (STACK_ANALYSIS_TIMER != 0U)
  /* Create Stack Analysis thread  */
  StackAnalysisTaskHandle = rtosalThreadNew((const rtosal_char_t *)"StackAnalysisThread",
                                            (os_pthread)stack_analysis_thread,
                                            STACK_ANALYSIS_THREAD_PRIO, USED_STACK_ANALYSIS_THREAD_STACK_SIZE, NULL);
  if (StackAnalysisTaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_MAIN, 1, ERROR_FATAL);
  }
  else
  {
    (void)stackAnalysis_addStackSizeByHandle(StackAnalysisTaskHandle, USED_STACK_ANALYSIS_THREAD_STACK_SIZE);
  }
#endif /* STACK_ANALYSIS_TIMER != 0U */
}

#endif /* USE_STACK_ANALYSIS == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
