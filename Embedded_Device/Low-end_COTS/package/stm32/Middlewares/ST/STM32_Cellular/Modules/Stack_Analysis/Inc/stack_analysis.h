/**
  ******************************************************************************
  * @file    stack_analysis.h
  * @author  MCD Application Team
  * @brief   Header for stack_analysis.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STACK_ANALYSIS_H
#define STACK_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_STACK_ANALYSIS == 1)

#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Task name length : used for the trace convenience */
#define STACK_ANALYSIS_TASK_NAME_MAX 15

/* if SA_TRACE_ONLY_THE_CHANGE == 1, then trace only:
   threads whose heap has changed since last display
   threads not referenced with addStackSizeByHandle or addStackSizeByName
   threads whose free heap is less than SA_WARNING_IF_THREAD_FREE_STACK_AT_X_PERCENT of the starting value
   Note: global heap is always displayed */
#define SA_TRACE_ONLY_THE_CHANGE           0 /* 0: Trace only the change not activated (all heap displayed)
                                                1: Trace only the change activated */

/* Possible filtering of the stack analysis
   Trace only thread if free heap is less than :
   SA_WARNING_IF_THREAD_FREE_STACK_AT_X_PERCENT of the starting value
 */
#define SA_WARNING_IF_THREAD_FREE_STACK_AT_X_PERCENT 15U
#define SA_WARNING_IF_GLOBAL_HEAP_FREE_AT_X_PERCENT  10U


/* trace stack analysis mode */
typedef uint8_t sa_print_format_t;
#define STACK_ANALYSIS_PRINT    (sa_print_format_t)(1U)     /* normal print result */
#define STACK_ANALYSIS_VALID    (sa_print_format_t)(2U)     /* validation test     */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/*** Update Stack Analysis data when a new task is created ********************/
/* After each TaskCreate, call addStackSizeByHandle or addStackSizeByName
  to provide the stackSizeAtCreation to StackAnalysis */

/**
  * @brief  Using TaskHandle to refer the task, add the task and its stack size at creation in the task List
  * @param  TaskHandle - Task handle to add
  * @param  stackSizeAtCreation - stack size at its creation
  * @retval bool - false: task not added in the list / true: task added in the list
  */
bool stackAnalysis_addStackSizeByHandle(
  void *TaskHandle,
  uint16_t stackSizeAtCreation);

/**
  * @brief  Using TaskName to refer the task, add the task and its stack size at creation in the task List
  * @param  TaskName - Task name to add
  * @param  stackSizeAtCreation - stack size at its creation
  * @retval bool - false: task not added in the list / true: task added in the list
  */
bool stackAnalysis_addStackSizeByName(
  uint8_t *TaskName,
  uint16_t stackSizeAtCreation);

/*** Trace interface **********************************************************/
/**
  * @brief  Set stack analysis print format
  * @note   Stack Analysis print format setting
  * @param  print_format - new format
  * @retval bool - false: bad format / true: format updated
  */
bool stackAnalysis_print_format_set(sa_print_format_t print_format);

/**
  * @brief  Trace a stack analysis
  * @param  -
  * @retval bool - false: trace not done (lack of memory) / true: trace done
  */
bool stackAnalysis_trace(void);


/*** Component Initialization/Start *******************************************/
/*** Internal use only - Not an Application Interface *************************/

/**
  * @brief  Stack Analysis init
  * @note   Must be called before to any other services - Protected if called multiple times
  * @param  -
  * @retval -
  */
void stackAnalysis_init(void);

/**
  * @brief  Stack Analysis start
  * @note   Must be called after stackAnalysis_init and before any other service
  * @param  -
  * @retval -
  */
void stackAnalysis_start(void);


#endif /* USE_STACK_ANALYSIS == 1 */

#ifdef __cplusplus
}
#endif

#endif /* STACK_ANALYSIS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
