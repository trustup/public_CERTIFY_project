/**
  ******************************************************************************
  * @file    freertos.c
  * @author  MCD Application Team
  * @brief   Default task : System init
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
#include <stdlib.h>

#include "plf_config.h"
#include "rtosal.h"

#include "error_handler.h"
#include "cellular_mngt.h"


#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#endif /* (USE_PRINTF == 0U) */

#if (USE_BUTTONS == 1)
#include "board_buttons.h"
#endif /* USE_BUTTONS == 1 */

#if (USE_LEDS == 1)
#include "board_leds.h"
#endif /* USE_LEDS == 1 */


#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif /* (USE_CMD_CONSOLE == 1) */

#if (!USE_DEFAULT_SETUP == 1)
#include "time_date.h"
#include "app_select.h"
#include "setup.h"
#if (USE_MODEM_VOUCHER == 1)
#include "voucher.h"
#endif /* (USE_MODEM_VOUCHER == 1) */
#endif /* (!USE_DEFAULT_SETUP == 1) */

#if (USE_CUSTOM_CLIENT == 1)
#include "custom_client.h"
#endif /* (USE_CUSTOM_CLIENT == 1) */

#if (USE_UI_CLIENT == 1)
#include "uiclient.h"
#endif /* (USE_UI_CLIENT == 1) */

#if (USE_ECHO_CLIENT == 1)
#include "echoclient.h"
#endif /* (USE_ECHO_CLIENT == 1) */

#if (USE_HTTP_CLIENT == 1)
#include "httpclient.h"
#endif /* (USE_HTTP_CLIENT == 1) */

#if (USE_PING_CLIENT == 1)
#include "ping_client.h"
#endif /* (USE_PING_CLIENT == 1) */

#if (USE_COM_CLIENT == 1)
#include "comclient.h"
#endif /* (USE_COM_CLIENT == 1) */

#if (USE_MQTT_CLIENT == 1)
#include "mqttclient.h"
#endif /* (USE_MQTT_CLIENT == 1) */

#if (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1)
#include "dc_mems.h"
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if (USE_DC_GENERIC == 1)
#include "dc_generic.h"
#endif /* (USE_DC_GENERIC == 1)  */

/* Private defines -----------------------------------------------------------*/

#if (!USE_DEFAULT_SETUP == 1)
#define MODEM_POWER_ON_LABEL "Modem power on (without application)"
#endif /* (!USE_DEFAULT_SETUP == 1) */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_TEST == 1U)
#if (USE_PRINTF == 0U)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "RTOS:" (format) "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "RTOS ERROR:" (format) "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_DBG(format, args...)   (void)printf("RTOS:" format "\n\r", ## args);
#define PRINT_ERR(format, args...)   (void)printf("RTOS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_DBG(...)      __NOP(); /* Nothing to do */
#define PRINT_ERR(...)      __NOP(); /* Nothing to do */
#endif /* USE_TRACE_TEST */

/* Private variables ---------------------------------------------------------*/
#if (!USE_DEFAULT_SETUP == 1)
static bool boot_modem_only = false;
#endif /* (!USE_DEFAULT_SETUP == 1) */

/* Private typedef -----------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void applications_init(void);
static void applications_start(void);
static void utilities_init(void);
static void utilities_start(void);

#if (!USE_DEFAULT_SETUP == 1)
static bool set_boot_modem_only(void);
#endif /* (!USE_DEFAULT_SETUP == 1) */

/* Global variables ----------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
void StartDefaultTask(void *p_argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Initialize the applications
  * @param  -
  * @retval -
  */
static void applications_init(void)
{
#if (USE_CUSTOM_CLIENT == 1)
  custom_client_init();
#endif /* (USE_CUSTOM_CLIENT == 1) */

#if (USE_UI_CLIENT == 1)
  uiclient_init();
#endif /* (USE_UI_CLIENT == 1)   */

#if (USE_ECHO_CLIENT == 1)
  echoclient_init();
#endif /* (USE_ECHO_CLIENT == 1) */

#if (USE_HTTP_CLIENT == 1)
  httpclient_init();
#endif /* (USE_HTTP_CLIENT == 1) */

#if (USE_PING_CLIENT == 1)
  pingclient_init();
#endif /* (USE_PING_CLIENT == 1) */

#if (USE_COM_CLIENT == 1)
  comclient_init();
#endif /* (USE_COM_CLIENT == 1)  */

#if (USE_MQTT_CLIENT == 1)
  mqttclient_init();
#endif /* (USE_MQTT_CLIENT == 1) */
}

/**
  * @brief  Initialize the utilities
  * @param  -
  * @retval -
  */
static void utilities_init(void)
{
  /* call to stackAnalysis_init() must be done earlier */
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
  dc_mems_init();
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if (USE_DC_GENERIC == 1)
  dc_generic_init();
#endif /* (USE_DC_GENERIC == 1) */
}

/**
  * @brief  Start the applications
  * @param  -
  * @retval -
  */
static void applications_start(void)
{
#if (USE_CUSTOM_CLIENT == 1)
  custom_client_start();
#endif /* (USE_CUSTOM_CLIENT == 1) */

#if (USE_UI_CLIENT == 1)
  uiclient_start();
#endif /* (USE_UI_CLIENT == 1)   */

#if (USE_ECHO_CLIENT == 1)
  echoclient_start();
#endif /* (USE_ECHO_CLIENT == 1) */

#if (USE_HTTP_CLIENT == 1)
  httpclient_start();
#endif /* (USE_HTTP_CLIENT == 1) */

#if (USE_PING_CLIENT == 1)
  pingclient_start();
#endif /* (USE_PING_CLIENT == 1) */

#if (USE_COM_CLIENT == 1)
  comclient_start();
#endif /* (USE_COM_CLIENT == 1)  */

#if (USE_MQTT_CLIENT == 1)
  mqttclient_start();
#endif /* (USE_MQTT_CLIENT == 1) */
}

/**
  * @brief  set boot modem only config
  * @param  -
  * @retval -
  */
#if (!USE_DEFAULT_SETUP == 1)
static bool set_boot_modem_only(void)
{
  boot_modem_only = true;
  return true;
}
#endif  /* !USE_DEFAULT_SETUP == 1 */
/**
  * @brief  Start the utilities
  * @param  -
  * @retval -
  */
static void utilities_start(void)
{
#if (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1)
  dc_mems_start();
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if (USE_DC_GENERIC == 1)
  dc_generic_start();
#endif /* (USE_DC_GENERIC == 1) */
}

/* Functions Definition ------------------------------------------------------*/

/* Init FreeRTOS */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  static osThreadId defaultTaskHandle;

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* definition and creation of DebounceTimer */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  defaultTaskHandle = rtosalThreadNew((const rtosal_char_t *)"StartDefaultThread", (os_pthread)StartDefaultTask,
                                      CTRL_THREAD_PRIO, USED_DEFAULT_THREAD_STACK_SIZE, NULL);
  if (defaultTaskHandle == NULL)
  {
#if (USE_PRINTF == 0U)
    /* Error Handler may use trace print */
    traceIF_init();
#endif /* (USE_PRINTF == 0U)  */
    ERROR_Handler(DBG_CHAN_MAIN, 0, ERROR_FATAL);
  }
#if (USE_STACK_ANALYSIS == 1)
  stackAnalysis_init();
  (void)stackAnalysis_addStackSizeByHandle(defaultTaskHandle, (uint16_t)USED_DEFAULT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* definition and creation of testTask */

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}


/* StartDefaultTask function */
void StartDefaultTask(void *p_argument)
{
  UNUSED(p_argument);

  /* RandomNumberGenerator */
  srand(rtosalGetSysTimerCount());

#if (USE_BUTTONS == 1)
  /* Board Buttons initialization */
  (void)board_buttons_init();
#endif /* USE_BUTTONS == 1 */

#if (USE_LEDS == 1)
  /* Board Leds initialization    */
  (void)board_leds_init();
#endif /* USE_LEDS == 1 */

#if (USE_PRINTF == 0U)
  /* Error Handler in the modules below may use trace print */
  /* Recall traceIF_init() in case MX_FREERTOS_Init is not used or is redefined */
  traceIF_init();
#endif /* (USE_PRINTF == 0U)  */

#if (USE_STACK_ANALYSIS == 1)
  /* Recall stackAnalysis_init() in case MX_FREERTOS_Init is not used or is redefined */
  stackAnalysis_init();
  /* check values in task.c tskIDLE_STACK_SIZE */
  (void)stackAnalysis_addStackSizeByHandle(xTaskGetIdleTaskHandle(),
                                           (uint16_t)configMINIMAL_STACK_SIZE);
  /* check values in FreeRTOSConfig.h */
  (void)stackAnalysis_addStackSizeByHandle(xTimerGetTimerDaemonTaskHandle(),
                                           (uint16_t)configTIMER_TASK_STACK_DEPTH);
#endif /* USE_STACK_ANALYSIS == 1 */

#if (USE_CMD_CONSOLE == 1)
  CMD_init();
#endif /* (USE_CMD_CONSOLE == 1) */

#if (!USE_DEFAULT_SETUP == 1)
  /* If menu is used, menu variables must be initialized before the rest of software */
  app_select_init();
  setup_init();
#endif /* (!USE_DEFAULT_SETUP == 1) */

  /* Cellular components statical init */
#if (USE_NETWORK_LIBRARY == 1)
  cellular_net_init();
#else
  cellular_init();
#endif /* (USE_NETWORK_LIBRARY == 1) */

  /* Application components statical init  */
  applications_init();

  /* Other optional components statical init */
  utilities_init();

#if (!USE_DEFAULT_SETUP == 1)
  /* add boot menu choice to boot modem only */
  (void)app_select_record((uint8_t *)MODEM_POWER_ON_LABEL, set_boot_modem_only);
#if (USE_MODEM_VOUCHER == 1)
  voucher_init();
#endif /* (USE_MODEM_VOUCHER == 1) */

  app_select_start();

  if (boot_modem_only == true)
  {
#if (USE_PRINTF == 0U)
    traceIF_start();
#endif /* (USE_PRINTF == 0U) */
    cellular_modem_start();
  }
  else
#endif /* (!USE_DEFAULT_SETUP == 1) */
  {
#if (!USE_DEFAULT_SETUP == 1)
    setup_apply();
#endif /* (!USE_DEFAULT_SETUP == 1) */

#if (USE_BUTTONS == 1)
    /* Board Buttons start */
    (void)board_buttons_start();
#endif /* USE_BUTTONS == 1 */

#if (USE_LEDS == 1)
    /* Board Leds start    */
    (void)board_leds_start();
#endif /* USE_LEDS == 1 */

    /* Cellular components start */
#if (USE_NETWORK_LIBRARY == 1)
    cellular_net_start();
#else
    cellular_start();
#endif /* (USE_NETWORK_LIBRARY == 1) */

    /* Application components start */
    applications_start();

    /* Utilities components start */
    utilities_start();
  }

#if (USE_CMD_CONSOLE == 1)
  CMD_start();
#endif /* (USE_CMD_CONSOLE == 1) */

#if (USE_STACK_ANALYSIS == 1)
  /* If stack analysis is activated check stack overflow after all init/start */
  (void)stackAnalysis_trace();
#endif /* USE_STACK_ANALYSIS == 1 */
  /* Platform Initialization and Thread Start are done */
  /* No more need of this thread wait a little before to free the memory */
  (void)rtosalDelay(2000U);
  vTaskDelete(NULL);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
