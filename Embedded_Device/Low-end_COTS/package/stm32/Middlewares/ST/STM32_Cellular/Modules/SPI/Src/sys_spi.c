/**
  ******************************************************************************
  * @file    sys_spi.c
  * @author  MCD Application Team
  * @brief   System SPI module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "sys_spi.h"

#if ((USE_DISPLAY == 1) || (USE_ST33 == 1))

#include <string.h>
#include "cellular_service_os.h"
#include "cellular_runtime_standard.h"
#include "rtosal.h"

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static bool sys_spi_powered; /* true/false SPI is powered or not powered */
static sys_spi_configuration_t sys_spi_actual_configuration; /* SPI configuration */
static osMutexId SysSPIMutexHandle = NULL; /* Mutex to protect SPI configuration change */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static bool sys_spi_at_command(uint8_t *p_at_cmd);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Send the at command to request to Modem to Power ON/OFF the SPI
  * @param  p_at_cmd - pointer on at command buffer to send
  * @retval bool - true/false command executed with success or not
  */
static bool sys_spi_at_command(uint8_t *p_at_cmd)
{
  bool result = false;
  uint32_t at_timeout = 5000U;
  CS_direct_cmd_tx_t direct_cmd_tx;

  direct_cmd_tx.cmd_size    = (uint16_t)crs_strlen(p_at_cmd);
  direct_cmd_tx.cmd_timeout = at_timeout;
  (void)memcpy(&direct_cmd_tx.cmd_str[0], p_at_cmd, direct_cmd_tx.cmd_size);

  if (osCDS_direct_cmd(&direct_cmd_tx, NULL) == CELLULAR_OK)
  {
    result = true;
  }

  return (result);
}

static void sys_spi_configure(sys_spi_configuration_t sys_spi_conf)
{
  switch (sys_spi_conf)
  {
    case SYS_SPI_DISPLAY_CONFIGURATION:
      /* LCD_BUS_Init() is in charge to configure SPI3 to LCD */
      /* Already done in BSP_LCD_Start, BSP_LCD_Refresh */
      /* Missing in BSP_LCD_Clear ? */
      break;

    case SYS_SPI_ST33_CONFIGURATION:
      /* MX_LOC_SPI_Init() is in charge to configure SPI3 to ST33 */
      /* Already done in spi_ndlc_init */
      break;

    case SYS_SPI_NO_CONFIGURATION:
    default:
      break;
  }

  /* Update actual configuration */
  sys_spi_actual_configuration = sys_spi_conf;
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Power ON the SPI to access to Display or ST33
  * @param  -
  * @retval bool - true/false SPI Power ON OK/NOK
  */
bool sys_spi_power_on(void)
{
  bool result = true;

  /* PIL: add command in AT */
  if (sys_spi_powered == false)
  {
    /* sys_spi_at_command("at%setacfg=pm.conf.sleep_mode,disable"); */
    result = sys_spi_at_command(((uint8_t *)"at%ldocmd=\"on\",2"));
    if (result == true)
    {
      sys_spi_powered = true;
    }
    /* else : Nothing to do result set to false */
  }
  /* SPI already Power ON - Nothing to do */

  return (result);
}

/**
  * @brief  Power OFF the SPI to improve power consumption
  * @param  -
  * @retval bool - true/false SPI Power OFF OK/NOK
  */
bool sys_spi_power_off(void)
{
  bool result = true;

  /* PIL: add command in AT  */
  if (sys_spi_powered == true)
  {
    /* sys_spi_at_command("at%setacfg=pm.conf.sleep_mode,enable"); */
    result = sys_spi_at_command(((uint8_t *)"at%ldocmd=\"off\",2"));
    if (result == true)
    {
      sys_spi_powered = false;
    }
  }
  /* SPI already Power OFF - Nothing to do */

  return (result);
}

/**
  * @brief  SPI acquire
  * @param  sys_spi_conf - requested configuration
  * @retval -
  */
void sys_spi_acquire(sys_spi_configuration_t sys_spi_conf)
{
  (void)rtosalMutexAcquire(SysSPIMutexHandle, RTOSAL_WAIT_FOREVER);
  if (sys_spi_actual_configuration != sys_spi_conf)
  {
    /* Change configuration to the one requested */
    sys_spi_configure(sys_spi_conf);
  }
}

/**
  * @brief  SPI release
  * @param  sys_spi_conf - requested configuration
  * @retval -
  */
void sys_spi_release(sys_spi_configuration_t sys_spi_conf)
{
  if (sys_spi_actual_configuration == sys_spi_conf)
  {
    (void)rtosalMutexRelease(SysSPIMutexHandle);
  }
}

/**
  * @brief  SPI Initialization
  * @param  -
  * @retval bool - true/false SPI initialization OK/NOK
  */
bool sys_spi_init(void)
{
  bool result = true; /* if sSPI already initialized return init ok */

  /* Protection against multi-entrance */
  if (SysSPIMutexHandle == NULL)
  {
    /* SPI is not powered */
    sys_spi_powered = false;
    /* SPI is not configured */
    sys_spi_actual_configuration = SYS_SPI_NO_CONFIGURATION;

    /* Initialize Mutex to protect sys spi configuration change */
    SysSPIMutexHandle = rtosalMutexNew(NULL);
    if (SysSPIMutexHandle == NULL)
    {
      result = false;
    }
  }
  return (result);
}

#endif /* (USE_DISPLAY == 1) || (USE_ST33 == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

