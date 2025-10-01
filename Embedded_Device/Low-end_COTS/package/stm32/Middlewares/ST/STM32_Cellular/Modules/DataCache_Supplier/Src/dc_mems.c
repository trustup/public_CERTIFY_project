/**
  ******************************************************************************
  * @file    dc_mems.c
  * @author  MCD Application Team
  * @brief   This file contains management of mems data for
  *          Nucleo expansion board X-NUCLEO-IKS01A2
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

#include <string.h>

#include "plf_config.h"
#include "rtosal.h"
#include "dc_mems.h"

#include "dc_common.h"
#include "error_handler.h"
#include "cellular_runtime_custom.h"
#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

#if (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1)

#if (USE_DC_MEMS == 1)
/* MISRAC messages linked to BSP include are ignored */
/*cstat -MISRAC2012-* */
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_accelero.h"
#include "x_nucleo_iks01a2_gyro.h"
#include "x_nucleo_iks01a2_magneto.h"
#include "x_nucleo_iks01a2_humidity.h"
#include "x_nucleo_iks01a2_temperature.h"
#include "x_nucleo_iks01a2_pressure.h"
#elif defined (USE_STM32L475E_IOT01) /* USE B-L475E-IOT1 MEMS */
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_psensor.h"
#elif defined (USE_STM32L462E_CELL01) /* USE B-L462E-CELL1 MEMS */
#include "stm32l462e_cell01.h"
#include "stm32l462e_cell01_hsensor.h"
#include "stm32l462e_cell01_psensor.h"
#include "stm32l462e_cell01_tsensor.h"
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
#include "STWIN.h"
#include "STWIN_motion_sensors.h"
#include "STWIN_motion_sensors_ex.h"
#include "STWIN_env_sensors.h"
#include "STWIN_env_sensors_ex.h"
#endif   /* USE_STM32L496G_DISCO */
/*cstat +MISRAC2012-* */
#endif /* USE_DC_MEMS */


/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_MAIN, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)   (void)printf("" format "\n\r", ## args);
#endif   /* (USE_PRINTF == 0U) */

#if (USE_TRACE_DCMEMS == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "MEMS:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "MEMS:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "MEMS ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void)printf("MEMS:" format "\n\r", ## args);
#define PRINT_DBG(format, args...)   (void)printf("MEMS:" format "\n\r", ## args);
#define PRINT_ERR(format, args...)   (void)printf("MEMS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_DCMEMS */

/* Private defines -----------------------------------------------------------*/
#define MEMS_POLLING_PERIOD    10000U

#define SIMULATED_PRESSURE_INCREMENT    (float_t)1
#define SIMULATED_PRESSURE_INITIAL      (float_t)1000
#define SIMULATED_PRESSURE_DELTA        (float_t)7
#define SIMULATED_PRESSURE_MAX          (float_t)(SIMULATED_PRESSURE_INITIAL+SIMULATED_PRESSURE_DELTA)
#define SIMULATED_PRESSURE_MIN          (float_t)(SIMULATED_PRESSURE_INITIAL-SIMULATED_PRESSURE_DELTA)

#define SIMULATED_HUMIDITY_INCREMENT    (float_t)1
#define SIMULATED_HUMIDITY_INITIAL      (float_t)85
#define SIMULATED_HUMIDITY_DELTA        (float_t)10
#define SIMULATED_HUMIDITY_MAX          (SIMULATED_HUMIDITY_INITIAL+SIMULATED_HUMIDITY_DELTA)
#define SIMULATED_HUMIDITY_MIN          (SIMULATED_HUMIDITY_INITIAL-SIMULATED_HUMIDITY_DELTA)

#define SIMULATED_TEMPERATURE_INCREMENT (float_t)0.8
#define SIMULATED_TEMPERATURE_INITIAL   (float_t)40
#define SIMULATED_TEMPERATURE_DELTA     (float_t)7
#define SIMULATED_TEMPERATURE_MAX       (SIMULATED_TEMPERATURE_INITIAL+SIMULATED_TEMPERATURE_DELTA)
#define SIMULATED_TEMPERATURE_MIN       (SIMULATED_TEMPERATURE_INITIAL-SIMULATED_TEMPERATURE_DELTA)


/* Private variables ---------------------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
static uint8_t *mems_cmd_label = (uint8_t *)"mems";
static uint32_t mems_state;
#endif  /* USE_CMD_CONSOLE */

#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO)    /* USE X-NUCLEO-IKS01A2 MEMS */
static void *ACCELERO_handle = NULL;
static void *GYRO_handle = NULL;
static void *MAGNETO_handle = NULL;
static void *HUMIDITY_handle = NULL;
static void *TEMPERATURE_handle = NULL;
static void *PRESSURE_handle = NULL;
#elif defined (USE_STM32L475E_IOT01)          /* USE B-L475E-IOT1 MEMS */
static int16_t ACC_Value[3];                  /*!< Acceleration Value */
static float_t GYR_Value[3];                  /*!< Gyroscope Value */
static int16_t MAG_Value[3];                  /*!< Magnetometer Value */
static int8_t mems_init_status;               /* init status of mems */
#define FLAG_MEMS_NONE        ((uint8_t) 0x00U)
#define FLAG_MEMS_ACC         ((uint8_t) 0x01U)
#define FLAG_MEMS_GYRO        ((uint8_t) 0x02U)
#define FLAG_MEMS_MAGN        ((uint8_t) 0x04U)
#define FLAG_MEMS_HUMIDTY     ((uint8_t) 0x08U)
#define FLAG_MEMS_TEMPERATURE ((uint8_t) 0x10U)
#define FLAG_MEMS_PRESSURE    ((uint8_t) 0x20U)
#elif defined (USE_STM32L462E_CELL01)         /* USE STM32L462E_CELL01 */
static int8_t mems_init_status;               /* init status of mems */
#define FLAG_MEMS_HUMIDTY     ((uint8_t) 0x08U)
#define FLAG_MEMS_TEMPERATURE ((uint8_t) 0x10U)
#define FLAG_MEMS_PRESSURE    ((uint8_t) 0x20U)
#elif defined (USE_STEVAL_STWIN)              /* USE STEVAL-STWINKT1 */
static int8_t mems_init_status;               /* init status of mems */
#define FLAG_MEMS_NONE        ((uint8_t) 0x00U)
#define FLAG_MEMS_ACC         ((uint8_t) 0x01U)
#define FLAG_MEMS_GYRO        ((uint8_t) 0x02U)
#define FLAG_MEMS_MAGN        ((uint8_t) 0x04U)
#define FLAG_MEMS_HUMIDITY    ((uint8_t) 0x08U)
#define FLAG_MEMS_TEMPERATURE ((uint8_t) 0x10U)
#define FLAG_MEMS_PRESSURE    ((uint8_t) 0x20U)
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */

static dc_pressure_info_t          dc_pressure_info;
static dc_humidity_info_t          dc_humidity_info;
static dc_temperature_info_t       dc_temperature_info;
static uint32_t       mems_polling_period;


/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t  DC_COM_PRESSURE      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t  DC_COM_HUMIDITY      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t  DC_COM_TEMPERATURE   = DC_COM_INVALID_ENTRY;
dc_com_res_id_t  DC_COM_ACCELEROMETER = DC_COM_INVALID_ENTRY;
dc_com_res_id_t  DC_COM_GYROSCOPE     = DC_COM_INVALID_ENTRY;
dc_com_res_id_t  DC_COM_MAGNETOMETER  = DC_COM_INVALID_ENTRY;

/* Private function prototypes -----------------------------------------------*/
static void StartMemsDclibTask(void *argument);
static void mems_init_sensors(void);
static void mems_get_acc_datas(void);
static void mems_get_gyro_datas(void);
static void mems_get_magn_datas(void);
static uint8_t mems_get_pressure_datas(void);
static uint8_t mems_get_humidity_datas(void);
static uint8_t mems_get_temperature_datas(void);
#if (USE_CMD_CONSOLE == 1)
static void mems_cmd_help(void);
static cmd_status_t mems_cmd(uint8_t *cmd_line_p);
#endif /* USE_CMD_CONSOLE */

/* Functions Definition ------------------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  help command
  * @param  none
  * @retval none
  */
static void mems_cmd_help(void)
{
  CMD_print_help(mems_cmd_label);
  PRINT_FORCE("%s help", mems_cmd_label);
  PRINT_FORCE("%s state (state of mems software component)", mems_cmd_label);
  PRINT_FORCE("%s disable (disable mems process)", mems_cmd_label);
  PRINT_FORCE("%s enable (enable mems process)", mems_cmd_label);
  PRINT_FORCE("%s period [<ms>] (set/get mems process period)", mems_cmd_label);
  PRINT_FORCE("%s pressure (get current pressure value)", mems_cmd_label);
  PRINT_FORCE("%s pressure <ppp>  (set pressure value and disable mems process)", mems_cmd_label);
  PRINT_FORCE("%s temperature (get current temperature value)", mems_cmd_label);
  PRINT_FORCE("%s temperature <ttt> (set temperature value and disable mems process)", mems_cmd_label);
  PRINT_FORCE("%s humidity (get current humidity value)", mems_cmd_label);
  PRINT_FORCE("%s humidity <hhh> (set humidity value and disable mems process)", mems_cmd_label);
}

/**
  * @brief  mems command handler
  * @param  cmd_line_p     command line
  * @retval status on command
  */
static cmd_status_t mems_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  uint8_t    *cmd_p;
  dc_pressure_info_t        pressure_info;
  dc_humidity_info_t        humidity_info;
  dc_temperature_info_t     temperature_info;
  float_t HUMIDITY_Value;    /*!< Humidity Value */
  float_t PRESSURE_Value;    /*!< Pressure Value */
  float_t TEMPERATURE_Value; /*!< Temperature Value */
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p == NULL)
  {
    PRINT_FORCE("bad command\n\r");
    mems_cmd_help();
  }
  else if (memcmp((CRC_CHAR_t *)cmd_p, (CRC_CHAR_t *)mems_cmd_label, crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < 10U ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }
    if (argc == 0U)
    {
      mems_cmd_help();
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      mems_cmd_help();
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "state", crs_strlen(argv_p[0])) == 0)
    {
      if (mems_state == 1U)
      {
        PRINT_FORCE("mems disabled");
      }
      else
      {
        PRINT_FORCE("mems enabled");
        PRINT_FORCE("polling period: %ld", mems_polling_period);
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "enable", crs_strlen(argv_p[0])) == 0)
    {
      mems_state = 0U;
      PRINT_FORCE("mems enabled");
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "disable", crs_strlen(argv_p[0])) == 0)
    {
      mems_state = 1U;
      PRINT_FORCE("mems disabled");
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "period", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        mems_polling_period = (uint32_t)crs_atoi(argv_p[1]);
      }
      else
      {
        PRINT_FORCE("mems polling period: %ld", mems_polling_period);
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "pressure", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        PRESSURE_Value = (float_t)crs_atoi(argv_p[1]);

        pressure_info.rt_state         =  DC_SERVICE_ON;
        pressure_info.pressure         =  PRESSURE_Value;
        mems_state = 1U;
        (void)dc_com_write(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));
      }
      (void)dc_com_read(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));
      if (pressure_info.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("pressure: %f", pressure_info.pressure);
      }
      else
      {
        PRINT_FORCE("%s invalid", argv_p[0]);
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "humidity", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        HUMIDITY_Value = (float_t)crs_atoi(argv_p[1]);
        humidity_info.rt_state         =  DC_SERVICE_ON;
        humidity_info.humidity         =  HUMIDITY_Value;
        mems_state = 1U;
        (void)dc_com_write(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
      }
      (void)dc_com_read(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
      if (humidity_info.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("humidity: %f", humidity_info.humidity);
      }
      else
      {
        PRINT_FORCE("%s invalid", argv_p[0]);
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "temperature", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        TEMPERATURE_Value = (float_t)crs_atoi(argv_p[1]);
        temperature_info.rt_state         =  DC_SERVICE_ON;
        temperature_info.temperature      =  TEMPERATURE_Value;
        mems_state = 1U;
        (void)dc_com_write(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
      }
      (void)dc_com_read(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
      if (temperature_info.rt_state ==  DC_SERVICE_ON)
      {
        PRINT_FORCE("temperature: %f", temperature_info.temperature);
      }
      else
      {
        PRINT_FORCE("%s invalid", argv_p[0]);
      }
    }
    else
    {
      PRINT_FORCE("%s bad command %s\n\r", cmd_p, argv_p[0]);
      mems_cmd_help();
    }
  }
  else
  {
    /* Nothing to do */
    __NOP();
  }
  return cmd_status;
}
#endif /* USE_CMD_CONSOLE */

/**
  * @brief  StartDefaultTask function
  * @param  argument     task argument (unused)
  * @retval none
  */
static void StartMemsDclibTask(void *argument)
{
#if (USE_SIMU_MEMS == 1)
  static float_t pressure_increment    = SIMULATED_PRESSURE_INCREMENT;
  static float_t humidity_increment    = SIMULATED_HUMIDITY_INCREMENT;
  static float_t temperature_increment = SIMULATED_TEMPERATURE_INCREMENT;

  static float_t simulated_pressure_value    = SIMULATED_PRESSURE_INITIAL;
  static float_t simulated_temperature_value = SIMULATED_TEMPERATURE_INITIAL;
  static float_t simulated_humidity_value    = SIMULATED_HUMIDITY_INITIAL;
#endif /* USE_SIMU_MEMS */

  mems_init_sensors();

  /* USER CODE BEGIN StartDefaultTask */
  (void)rtosalDelay(1000U);

  /* Infinite loop */
  for (;;)
  {
#if (USE_CMD_CONSOLE == 1)
    if (mems_state == 0U)
#endif /* USE_CMD_CONSOLE */
    {
      /* get mems data */
      mems_get_acc_datas();
      mems_get_gyro_datas();
      mems_get_magn_datas();

      if (mems_get_pressure_datas() != 1U)
      {
#if (USE_SIMU_MEMS == 1)
        simulated_pressure_value += pressure_increment;
        if (dc_pressure_info.pressure >= (float_t)SIMULATED_PRESSURE_MAX)
        {
          pressure_increment = (float_t) - SIMULATED_PRESSURE_INCREMENT;
        }
        if (dc_pressure_info.pressure <= (float_t)SIMULATED_PRESSURE_MIN)
        {
          pressure_increment = (float_t)SIMULATED_PRESSURE_INCREMENT;
        }

        dc_pressure_info.rt_state =  DC_SERVICE_ON;
        dc_pressure_info.pressure = simulated_pressure_value;
        PRINT_DBG("### Simulated PRESSURE_Value = %f\n\r", dc_pressure_info.pressure)
        (void)dc_com_write(&dc_com_db, DC_COM_PRESSURE, (void *)&dc_pressure_info, sizeof(dc_pressure_info_t));
#endif /* USE_SIMU_MEMS */
      }

      if (mems_get_humidity_datas() != 1U)
      {
#if (USE_SIMU_MEMS == 1)
        simulated_humidity_value += humidity_increment;
        {
          humidity_increment = -SIMULATED_HUMIDITY_INCREMENT;
        }
        if (dc_humidity_info.humidity <= SIMULATED_HUMIDITY_MIN)
        {
          humidity_increment  = SIMULATED_HUMIDITY_INCREMENT;
        }

        dc_humidity_info.rt_state =  DC_SERVICE_ON;
        dc_humidity_info.humidity = simulated_humidity_value;
        PRINT_DBG("### Simulated HUMIDITY Value = %f\n\r", dc_humidity_info.humidity)
        (void)dc_com_write(&dc_com_db, DC_COM_HUMIDITY, (void *)&dc_humidity_info, sizeof(dc_humidity_info_t));
#endif /* USE_SIMU_MEMS */
      }

      if (mems_get_temperature_datas() != 1U)
      {
#if (USE_SIMU_MEMS == 1)
        simulated_temperature_value += temperature_increment;
        if (dc_temperature_info.temperature >= SIMULATED_TEMPERATURE_MAX)
        {
          temperature_increment = -SIMULATED_TEMPERATURE_INCREMENT;
        }

        if (dc_temperature_info.temperature <= SIMULATED_TEMPERATURE_MIN)
        {
          temperature_increment = SIMULATED_TEMPERATURE_INCREMENT;
        }

        dc_temperature_info.rt_state =  DC_SERVICE_ON;
        dc_temperature_info.temperature = simulated_temperature_value;
        PRINT_DBG("### Simulated TEMPERATURE Value = %f\n\r", dc_temperature_info.temperature)
        (void)dc_com_write(&dc_com_db, DC_COM_TEMPERATURE, (void *)&dc_temperature_info, sizeof(dc_temperature_info_t));
#endif /* USE_SIMU_MEMS */
      }
    }
    (void)rtosalDelay(mems_polling_period);
  }
}

/**
  * @brief  dc mems  init
  * @param  none
  * @retval none
  */

void dc_mems_init(void)
{
  static dc_accelerometer_info_t     dc_accelerometer_info;
  static dc_gyroscope_info_t         dc_gyroscope_info;
  static dc_magnetometer_info_t      dc_magnetometer_info;

#if (USE_CMD_CONSOLE == 1)
  mems_state = 0U;
#endif /* USE_CMD_CONSOLE */

  mems_polling_period = MEMS_POLLING_PERIOD;
  (void)memset((void *)&dc_pressure_info,      0, sizeof(dc_pressure_info_t));
  (void)memset((void *)&dc_humidity_info,      0, sizeof(dc_humidity_info_t));
  (void)memset((void *)&dc_temperature_info,   0, sizeof(dc_temperature_info_t));
  (void)memset((void *)&dc_accelerometer_info, 0, sizeof(dc_accelerometer_info_t));
  (void)memset((void *)&dc_gyroscope_info,     0, sizeof(dc_gyroscope_info_t));
  (void)memset((void *)&dc_magnetometer_info,  0, sizeof(dc_magnetometer_info_t));

  DC_COM_PRESSURE       = dc_com_register_serv(&dc_com_db, (void *)&dc_pressure_info,
                                               (uint16_t)sizeof(dc_pressure_info_t));
  DC_COM_HUMIDITY       = dc_com_register_serv(&dc_com_db, (void *)&dc_humidity_info,
                                               (uint16_t)sizeof(dc_humidity_info_t));
  DC_COM_TEMPERATURE    = dc_com_register_serv(&dc_com_db, (void *)&dc_temperature_info,
                                               (uint16_t)sizeof(dc_temperature_info_t));
  DC_COM_ACCELEROMETER  = dc_com_register_serv(&dc_com_db, (void *)&dc_accelerometer_info,
                                               (uint16_t)sizeof(dc_accelerometer_info_t));
  DC_COM_GYROSCOPE      = dc_com_register_serv(&dc_com_db, (void *)&dc_gyroscope_info,
                                               (uint16_t)sizeof(dc_gyroscope_info_t));
  DC_COM_MAGNETOMETER   = dc_com_register_serv(&dc_com_db, (void *)&dc_magnetometer_info,
                                               (uint16_t)sizeof(dc_magnetometer_info_t));
}

/**
  * @brief  dc mems  start
  * @param  none
  * @retval none
  */

void dc_mems_start(void)
{
  static osThreadId memsDclibTaskTaskId;

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(mems_cmd_label, mems_cmd, (uint8_t *)"mems management");
#endif /* USE_CMD_CONSOLE */

  memsDclibTaskTaskId = rtosalThreadNew((const rtosal_char_t *)"memsDclibTask", (os_pthread)StartMemsDclibTask,
                                        DC_MEMS_THREAD_PRIO, USED_DC_MEMS_THREAD_STACK_SIZE, NULL);
  if (memsDclibTaskTaskId == NULL)
  {
    ERROR_Handler(DBG_CHAN_MAIN, 11, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(memsDclibTaskTaskId, USED_DC_MEMS_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

static void mems_init_sensors(void)
{
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  /* Try to use automatic discovery. By default use LSM6DSL on board */
  (void)BSP_ACCELERO_Init(ACCELERO_SENSORS_AUTO, &ACCELERO_handle);
  /* Try to use automatic discovery. By default use LSM6DSL on board */
  (void)BSP_GYRO_Init(GYRO_SENSORS_AUTO, &GYRO_handle);
  /* Try to use automatic discovery. By default use LSM303AGR on board */
  (void)BSP_MAGNETO_Init(MAGNETO_SENSORS_AUTO, &MAGNETO_handle);
  /* Try to use automatic discovery. By default use HTS221 on board */
  (void)BSP_HUMIDITY_Init(HUMIDITY_SENSORS_AUTO, &HUMIDITY_handle);
  /* Try to use automatic discovery. By default use HTS221 on board */
  (void)BSP_TEMPERATURE_Init(TEMPERATURE_SENSORS_AUTO, &TEMPERATURE_handle);
  /* Try to use automatic discovery. By default use LPS22HB on board */
  (void)BSP_PRESSURE_Init(PRESSURE_SENSORS_AUTO, &PRESSURE_handle);

  (void)BSP_ACCELERO_Sensor_Enable(ACCELERO_handle);
  (void)BSP_GYRO_Sensor_Enable(GYRO_handle);
  (void)BSP_MAGNETO_Sensor_Enable(MAGNETO_handle);
  (void)BSP_HUMIDITY_Sensor_Enable(HUMIDITY_handle);
  (void)BSP_TEMPERATURE_Sensor_Enable(TEMPERATURE_handle);
  (void)BSP_PRESSURE_Sensor_Enable(PRESSURE_handle);
#elif defined (USE_STM32L475E_IOT01) /* USE B-L475E-IOT1 MEMS */

  mems_init_status = FLAG_MEMS_NONE;

  if (ACCELERO_OK == BSP_ACCELERO_Init())
  {
    mems_init_status |= FLAG_MEMS_ACC;
  }

  if (GYRO_OK == BSP_GYRO_Init())
  {
    mems_init_status |= FLAG_MEMS_GYRO;
  }

  if (MAGNETO_OK == BSP_MAGNETO_Init())
  {
    mems_init_status |= FLAG_MEMS_MAGN;
  }

  if (HSENSOR_OK == BSP_HSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_HUMIDTY;
  }

  if (TSENSOR_OK == BSP_TSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_TEMPERATURE;
  }

  if (PSENSOR_OK == BSP_PSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_PRESSURE;
  }
#elif defined (USE_STM32L462E_CELL01) /* USE B-L462E-CELL1 MEMS */
  if (TSENSOR_OK == BSP_TSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_TEMPERATURE;
  }
  if (HSENSOR_OK == BSP_HSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_HUMIDTY;
  }
  if (PSENSOR_OK == BSP_PSENSOR_Init())
  {
    mems_init_status |= FLAG_MEMS_PRESSURE;
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  mems_init_status = FLAG_MEMS_NONE;

  /* Humidity */
  if (BSP_ENV_SENSOR_Init(HUMIDITY_INSTANCE, ENV_HUMIDITY) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_HUMIDITY;
    BSP_ENV_SENSOR_Enable(HUMIDITY_INSTANCE, ENV_HUMIDITY);
  };

  /* Pressure */
  if (BSP_ENV_SENSOR_Init(PRESSURE_INSTANCE, ENV_PRESSURE) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_PRESSURE;
    BSP_ENV_SENSOR_Enable(PRESSURE_INSTANCE, ENV_PRESSURE);
  }

  /* Temperature */
  if (BSP_ENV_SENSOR_Init(TEMPERATURE_INSTANCE, ENV_TEMPERATURE) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_TEMPERATURE;
    BSP_ENV_SENSOR_Enable(TEMPERATURE_INSTANCE, ENV_TEMPERATURE);
  }

  /* Accelero */
  if (BSP_MOTION_SENSOR_Init(ACCELERO_INSTANCE, MOTION_ACCELERO) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_ACC;
    BSP_MOTION_SENSOR_Enable(ACCELERO_INSTANCE, MOTION_ACCELERO);
    BSP_MOTION_SENSOR_SetOutputDataRate(ACCELERO_INSTANCE, MOTION_ACCELERO, 400);
  }

  /* Magneto */
  if (BSP_MOTION_SENSOR_Init(MAGNETO_INSTANCE, MOTION_MAGNETO) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_MAGN;
    BSP_MOTION_SENSOR_Enable(MAGNETO_INSTANCE, MOTION_MAGNETO);
    BSP_MOTION_SENSOR_SetOutputDataRate(MAGNETO_INSTANCE, MOTION_MAGNETO, 10);
  }

  /* Gyro */
  if (BSP_MOTION_SENSOR_Init(GYRO_INSTANCE, MOTION_GYRO) == BSP_ERROR_NONE)
  {
    mems_init_status |= FLAG_MEMS_GYRO;
    BSP_MOTION_SENSOR_Enable(GYRO_INSTANCE, MOTION_GYRO);
    BSP_MOTION_SENSOR_SetOutputDataRate(GYRO_INSTANCE, MOTION_GYRO, 833);
  }
#endif /* USE_STM32L496G_DISCO */
#endif /* USE_DC_MEMS */
}

/**
  * @brief  accelerometer data managememnt
  * @param  none
  * @retval none
  */
static void mems_get_acc_datas(void)
{
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  dc_accelerometer_info_t   accelerometer_info;
  static SensorAxes_t ACC_Value;                /*!< Acceleration Value */
  uint8_t status = 0U;

  if ((BSP_ACCELERO_IsInitialized(ACCELERO_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_ACCELERO_Get_Axes(ACCELERO_handle, &ACC_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### ACC_Value: X=%ld - Y=%ld - Z=%ld\n\r", ACC_Value.AXIS_X, ACC_Value.AXIS_Y, ACC_Value.AXIS_Z)

    accelerometer_info.rt_state              =  DC_SERVICE_ON;
    accelerometer_info.accelerometer.AXIS_X  =  ACC_Value.AXIS_X;
    accelerometer_info.accelerometer.AXIS_Y  =  ACC_Value.AXIS_Y;
    accelerometer_info.accelerometer.AXIS_Z  =  ACC_Value.AXIS_Z;
    (void)dc_com_write(&dc_com_db, DC_COM_ACCELEROMETER, (void *)&accelerometer_info, sizeof(accelerometer_info));
  }
#elif defined (USE_STM32L475E_IOT01) /* USE B-L475E-IOT1 MEMS */
  dc_accelerometer_info_t   accelerometer_info;
  if (mems_init_status & FLAG_MEMS_ACC)
  {
    BSP_ACCELERO_AccGetXYZ(ACC_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### ACC_Value: X=%d - Y=%d - Z=%d\n\r", ACC_Value[0], ACC_Value[1], ACC_Value[2])

    accelerometer_info.rt_state              =  DC_SERVICE_ON;
    accelerometer_info.accelerometer.AXIS_X  =  ACC_Value[0];
    accelerometer_info.accelerometer.AXIS_Y  =  ACC_Value[1];
    accelerometer_info.accelerometer.AXIS_Z  =  ACC_Value[2];
    dc_com_write(&dc_com_db, DC_COM_ACCELEROMETER, (void *)&accelerometer_info, sizeof(accelerometer_info));
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static BSP_MOTION_SENSOR_Axes_t ACC_Value;    /*!< Acceleration Value */
  dc_accelerometer_info_t accelerometer_info;

  if (mems_init_status & FLAG_MEMS_ACC)
  {
    (void)BSP_MOTION_SENSOR_GetAxes(ACCELERO_INSTANCE, MOTION_ACCELERO, &ACC_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### ACC_Value: X=%ld - Y=%ld - Z=%ld\n\r", ACC_Value.x, ACC_Value.y, ACC_Value.z)

    accelerometer_info.rt_state              =  DC_SERVICE_ON;
    accelerometer_info.accelerometer.AXIS_X  =  ACC_Value.x;
    accelerometer_info.accelerometer.AXIS_Y  =  ACC_Value.y;
    accelerometer_info.accelerometer.AXIS_Z  =  ACC_Value.z;
    (void)dc_com_write(&dc_com_db, DC_COM_ACCELEROMETER, (void *)&accelerometer_info, sizeof(accelerometer_info));
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
}

/**
  * @brief  gyroscope data managememnt
  * @param  none
  * @retval none
  */
static void mems_get_gyro_datas(void)
{
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  dc_gyroscope_info_t       gyroscope_info;
  static SensorAxes_t GYR_Value;                /*!< Gyroscope Value */
  uint8_t status = 0U;

  if ((BSP_GYRO_IsInitialized(GYRO_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_GYRO_Get_Axes(GYRO_handle, &GYR_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### GYR_Value: X=%ld - Y=%ld - Z=%ld\n\r", GYR_Value.AXIS_X, GYR_Value.AXIS_Y, GYR_Value.AXIS_Z)

    gyroscope_info.rt_state          =  DC_SERVICE_ON;
    gyroscope_info.gyroscope.AXIS_X  =  GYR_Value.AXIS_X;
    gyroscope_info.gyroscope.AXIS_Y  =  GYR_Value.AXIS_Y;
    gyroscope_info.gyroscope.AXIS_Z  =  GYR_Value.AXIS_Z;
    (void)dc_com_write(&dc_com_db, DC_COM_GYROSCOPE, (void *)&gyroscope_info, sizeof(gyroscope_info));
  }
#elif defined (USE_STM32L475E_IOT01) /* USE B-L475E-IOT1 MEMS */
  dc_gyroscope_info_t       gyroscope_info;
  if (mems_init_status & FLAG_MEMS_GYRO)
  {
    BSP_GYRO_GetXYZ(GYR_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### GYR_Value: X=%ld - Y=%ld - Z=%ld\n\r",
              (int32_t) GYR_Value[0],
              (int32_t) GYR_Value[1],
              (int32_t) GYR_Value[2]);

    gyroscope_info.rt_state          =  DC_SERVICE_ON;
    gyroscope_info.gyroscope.AXIS_X  = (int32_t) GYR_Value[0];
    gyroscope_info.gyroscope.AXIS_Y  = (int32_t) GYR_Value[1];
    gyroscope_info.gyroscope.AXIS_Z  = (int32_t) GYR_Value[2];
    dc_com_write(&dc_com_db, DC_COM_GYROSCOPE, (void *)&gyroscope_info, sizeof(gyroscope_info));
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static BSP_MOTION_SENSOR_Axes_t GYR_Value;    /*!< Gyroscope Value */
  dc_gyroscope_info_t gyroscope_info;

  if (mems_init_status & FLAG_MEMS_GYRO)
  {
    (void)BSP_MOTION_SENSOR_GetAxes(GYRO_INSTANCE, MOTION_GYRO, &GYR_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### GYR_Value: X=%ld - Y=%ld - Z=%ld\n\r", GYR_Value.x, GYR_Value.y, GYR_Value.z)

    gyroscope_info.rt_state          =  DC_SERVICE_ON;
    gyroscope_info.gyroscope.AXIS_X  =  GYR_Value.x;
    gyroscope_info.gyroscope.AXIS_Y  =  GYR_Value.y;
    gyroscope_info.gyroscope.AXIS_Z  =  GYR_Value.z;
    (void)dc_com_write(&dc_com_db, DC_COM_GYROSCOPE, (void *)&gyroscope_info, sizeof(gyroscope_info));
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
}

/**
  * @brief  magnetometer data managememnt
  * @param  none
  * @retval none
  */
static void mems_get_magn_datas(void)
{
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  dc_magnetometer_info_t    magnetometer_info;
  static SensorAxes_t MAG_Value;                /*!< Magnetometer Value */
  uint8_t status = 0U;

  if ((BSP_MAGNETO_IsInitialized(MAGNETO_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_MAGNETO_Get_Axes(MAGNETO_handle, &MAG_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### MAG_Value: X=%ld - Y=%ld - Z=%ld\n\r", MAG_Value.AXIS_X, MAG_Value.AXIS_Y, MAG_Value.AXIS_Z)

    magnetometer_info.rt_state             =  DC_SERVICE_ON;
    magnetometer_info.magnetometer.AXIS_X  =  MAG_Value.AXIS_X;
    magnetometer_info.magnetometer.AXIS_Y  =  MAG_Value.AXIS_Y;
    magnetometer_info.magnetometer.AXIS_Z  =  MAG_Value.AXIS_Z;
    (void)dc_com_write(&dc_com_db, DC_COM_MAGNETOMETER, (void *)&magnetometer_info, sizeof(magnetometer_info));
  }
#elif defined (USE_STM32L475E_IOT01) /* USE B-L475E-IOT1 MEMS */
  dc_magnetometer_info_t    magnetometer_info;
  if (mems_init_status & FLAG_MEMS_MAGN)
  {
    BSP_MAGNETO_GetXYZ(MAG_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### MAG_Value: X=%d - Y=%d - Z=%d\n\r", MAG_Value[0], MAG_Value[1], MAG_Value[2])

    magnetometer_info.rt_state             =  DC_SERVICE_ON;
    magnetometer_info.magnetometer.AXIS_X  =  MAG_Value[0];
    magnetometer_info.magnetometer.AXIS_Y  =  MAG_Value[1];
    magnetometer_info.magnetometer.AXIS_Z  =  MAG_Value[2];
    dc_com_write(&dc_com_db, DC_COM_MAGNETOMETER, (void *)&magnetometer_info, sizeof(magnetometer_info));
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static BSP_MOTION_SENSOR_Axes_t MAG_Value;    /*!< Magnetometer Value */
  dc_magnetometer_info_t magnetometer_info;

  if (mems_init_status & FLAG_MEMS_MAGN)
  {
    (void)BSP_MOTION_SENSOR_GetAxes(MAGNETO_INSTANCE, MOTION_MAGNETO, &MAG_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### MAG_Value: X=%ld - Y=%ld - Z=%ld\n\r", MAG_Value.x, MAG_Value.y, MAG_Value.z)

    magnetometer_info.rt_state             =  DC_SERVICE_ON;
    magnetometer_info.magnetometer.AXIS_X  =  MAG_Value.x;
    magnetometer_info.magnetometer.AXIS_Y  =  MAG_Value.y;
    magnetometer_info.magnetometer.AXIS_Z  =  MAG_Value.z;
    (void)dc_com_write(&dc_com_db, DC_COM_MAGNETOMETER, (void *)&magnetometer_info, sizeof(magnetometer_info));
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
}

/**
  * @brief  pressure data managememnt
  * @param  none
  * @retval   function result (0:OK)
  */
static uint8_t mems_get_pressure_datas(void)
{
  uint8_t success = 0U;
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  static float_t PRESSURE_Value; /*!< Pressure Value */
  dc_pressure_info_t        pressure_info;
  uint8_t status = 0U;

  if ((BSP_PRESSURE_IsInitialized(PRESSURE_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_PRESSURE_Get_Press(PRESSURE_handle, &PRESSURE_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### PRESSURE_Value = %f\n\r", PRESSURE_Value)

    pressure_info.rt_state         =  DC_SERVICE_ON;
    pressure_info.pressure         =  PRESSURE_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));
    success = 1U;
  }
#elif defined (USE_STM32L475E_IOT01)|| defined (USE_STM32L462E_CELL01) /* USE B-L475E-IOT1 MEMS */
  static float_t PRESSURE_Value; /*!< Pressure Value */
  dc_pressure_info_t        pressure_info;
  if (mems_init_status & FLAG_MEMS_PRESSURE)
  {
    PRESSURE_Value = BSP_PSENSOR_ReadPressure();

    /* DEBUG sensor values */
    PRINT_DBG("### PRESSURE_Value = %f\n\r", PRESSURE_Value)

    pressure_info.rt_state         =  DC_SERVICE_ON;
    pressure_info.pressure         =  PRESSURE_Value;
    dc_com_write(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));
    success = 1U;
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static float_t PRESSURE_Value;    /*!< Pressure Value */
  dc_pressure_info_t pressure_info;

  if (mems_init_status & FLAG_MEMS_PRESSURE)
  {
    (void)BSP_ENV_SENSOR_GetValue(PRESSURE_INSTANCE, ENV_PRESSURE, &PRESSURE_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### PRESSURE_Value = %f\n\r", PRESSURE_Value)

    pressure_info.rt_state         =  DC_SERVICE_ON;
    pressure_info.pressure      =  PRESSURE_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));
    success = 1U;
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
  return (success);
}

/**
  * @brief  humidity data managememnt
  * @param  none
  * @retval   function result (0:OK)
  */
static uint8_t mems_get_humidity_datas(void)
{
  uint8_t success = 0U;
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  static float_t HUMIDITY_Value;    /*!< Humidity Value */
  dc_humidity_info_t        humidity_info;
  uint8_t status = 0U;
  if ((BSP_HUMIDITY_IsInitialized(HUMIDITY_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_HUMIDITY_Get_Hum(HUMIDITY_handle, &HUMIDITY_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### HUMIDITY_Value = %f\n\r", HUMIDITY_Value)

    humidity_info.rt_state         =  DC_SERVICE_ON;
    humidity_info.humidity         =  HUMIDITY_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
    success = 1U;
  }
#elif defined (USE_STM32L475E_IOT01) || defined (USE_STM32L462E_CELL01) /* USE B-L475E-IOT1 MEMS */
  static float_t HUMIDITY_Value;    /*!< Humidity Value */
  dc_humidity_info_t        humidity_info;
  if (mems_init_status & FLAG_MEMS_HUMIDTY)
  {
    HUMIDITY_Value = BSP_HSENSOR_ReadHumidity();

    /* DEBUG sensor values */
    PRINT_DBG("### HUMIDITY_Value = %f\n\r", HUMIDITY_Value)

    humidity_info.rt_state         =  DC_SERVICE_ON;
    humidity_info.humidity         =  HUMIDITY_Value;
    dc_com_write(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
    success = 1U;
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static float_t HUMIDITY_Value;    /*!< Humidity Value */
  dc_humidity_info_t humidity_info;

  if (mems_init_status & FLAG_MEMS_HUMIDITY)
  {
    (void)BSP_ENV_SENSOR_GetValue(HUMIDITY_INSTANCE, ENV_HUMIDITY, &HUMIDITY_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### HUMIDITY_Value = %f\n\r", HUMIDITY_Value)

    humidity_info.rt_state         =  DC_SERVICE_ON;
    humidity_info.humidity      =  HUMIDITY_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
    success = 1U;
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
  return (success);
}

/**
  * @brief  temperature data managememnt
  * @param  none
  * @retval   function result (0:OK)
  */
static uint8_t mems_get_temperature_datas(void)
{
  uint8_t success;
  success = 0U;
#if (USE_DC_MEMS == 1)
#if defined (USE_STM32L496G_DISCO) /* USE X-NUCLEO-IKS01A2 MEMS */
  static float_t TEMPERATURE_Value;    /*!< Temperature Value */
  dc_temperature_info_t     temperature_info;
  uint8_t status = 0U;

  if ((BSP_TEMPERATURE_IsInitialized(TEMPERATURE_handle, &status) == COMPONENT_OK) && (status == 1U))
  {
    (void)BSP_TEMPERATURE_Get_Temp(TEMPERATURE_handle, &TEMPERATURE_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### TEMPERATURE_Value = %f\n\r", TEMPERATURE_Value)

    temperature_info.rt_state         =  DC_SERVICE_ON;
    temperature_info.temperature      =  TEMPERATURE_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
    success = 1U;
  }
#elif defined (USE_STM32L475E_IOT01) || defined (USE_STM32L462E_CELL01)/* USE B-L475E-IOT1 MEMS */
  static float_t TEMPERATURE_Value;    /*!< Temperature Value */
  dc_temperature_info_t     temperature_info;
  if (mems_init_status & FLAG_MEMS_TEMPERATURE)
  {
    TEMPERATURE_Value = BSP_TSENSOR_ReadTemp();

    /* DEBUG sensor values */
    PRINT_DBG("### TEMPERATURE_Value = %f\n\r", TEMPERATURE_Value)

    temperature_info.rt_state         =  DC_SERVICE_ON;
    temperature_info.temperature      =  TEMPERATURE_Value;
    dc_com_write(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
    success = 1U;
  }
#elif defined (USE_STEVAL_STWIN) /* USE STEVAL-STWINKT1 */
  static float_t TEMPERATURE_Value;    /*!< Temperature Value */
  dc_temperature_info_t temperature_info;

  if (mems_init_status & FLAG_MEMS_TEMPERATURE)
  {
    (void)BSP_ENV_SENSOR_GetValue(TEMPERATURE_INSTANCE, ENV_TEMPERATURE, &TEMPERATURE_Value);

    /* DEBUG sensor values */
    PRINT_DBG("### TEMPERATURE_Value = %f\n\r", TEMPERATURE_Value)

    temperature_info.rt_state         =  DC_SERVICE_ON;
    temperature_info.temperature      =  TEMPERATURE_Value;
    (void)dc_com_write(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
    success = 1U;
  }
#endif /* USE_STM32L475E_IOT01 */
#endif /* USE_DC_MEMS */
  return (success);
}

#endif  /*USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

