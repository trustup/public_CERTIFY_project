/**
  ******************************************************************************
  * @file    spi_ndlc.c
  * @author  MCD Application Team
  * @brief   SPI for NDLC module
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
#include "spi_ndlc.h"

#if (USE_ST33 == 1)

#include "sys_spi.h"
#include "ndlc_commands.h"
#include "spi.h"
#include "gpio.h"

#include "trace_interface.h"

#if (USE_TRACE_TEST == 1)
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, format , ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, format , ## args)
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_TEST == 1 */

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef char SPI_NDLC_CHAR_t; /* used in stdio.h and string.h service call */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static STSpiDevHnd_t spidevhnd;
/* PIL: Temporary patch- Normal correction : use rsp buffer provided as parameter by Applications */
/* uint8_t ST33Data[50]; */
uint8_t ST33Data[260];

/* Global variables ----------------------------------------------------------*/
extern SPI_HandleTypeDef ST33_SPI_HANDLE;

/* Private function prototypes -----------------------------------------------*/
static void print_buffer_line(SPI_NDLC_CHAR_t *name, uint16_t length, uint8_t *data);
static void MX_LOC_SPI_Init(void);
static void MX_LOC_SPI_DeInit(SPI_HandleTypeDef *spiHandle);

/* Private functions ---------------------------------------------------------*/
static void print_buffer_line(SPI_NDLC_CHAR_t *name, uint16_t length, uint8_t *p_data)
{
  PRINT_INFO("%s", name);
  /*  PRINT_DBG("%s",name); */
  if ((p_data != NULL) && (length != 0U))
  {
    for (uint8_t i = 0U; i < length; i++)
    {
      if (p_data[i] < 0xFU)
      {
        PRINT_INFO("0");
        /*  PRINT_DBG("0"); */
      }
      PRINT_INFO("%x ", p_data[i]);
      /*  PRINT_DBG("%x ", p_data[i]); */
    }
  }
  else
  {
    PRINT_INFO("pointer or length null")
  }
  PRINT_INFO("\r\n");
}

static void MX_LOC_SPI_Init(void)
{
  ST33_SPI_HANDLE.Instance = ST33_SPI_INSTANCE;
  ST33_SPI_HANDLE.Init.Mode = SPI_MODE_MASTER;
  ST33_SPI_HANDLE.Init.Direction = SPI_DIRECTION_2LINES;
  ST33_SPI_HANDLE.Init.DataSize = SPI_DATASIZE_8BIT;
  ST33_SPI_HANDLE.Init.CLKPolarity = SPI_POLARITY_LOW;
  ST33_SPI_HANDLE.Init.CLKPhase = SPI_PHASE_1EDGE;
  ST33_SPI_HANDLE.Init.NSS = SPI_NSS_SOFT;
  ST33_SPI_HANDLE.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  ST33_SPI_HANDLE.Init.FirstBit = SPI_FIRSTBIT_MSB;
  ST33_SPI_HANDLE.Init.TIMode = SPI_TIMODE_DISABLE;
  ST33_SPI_HANDLE.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  ST33_SPI_HANDLE.Init.CRCPolynomial = 7;
  ST33_SPI_HANDLE.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  ST33_SPI_HANDLE.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&ST33_SPI_HANDLE) != HAL_OK)
  {
    Error_Handler();
  }

  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = ST33_SPI_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ST33_SPI_CS_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(ST33_SPI_CS_PORT, ST33_SPI_CS_PIN, GPIO_PIN_SET);
}


static void MX_LOC_SPI_DeInit(SPI_HandleTypeDef *spiHandle)
{
  (void)HAL_SPI_DeInit(spiHandle);
  HAL_SPI_MspDeInit(spiHandle);
}


/* Functions Definition ------------------------------------------------------*/
bool spi_ndlc_init(void *hspi, ndlc_device_t *p_handler)
{
  static uint16_t cs_pin = ST33_SPI_CS_PIN;
  bool result;

  if (hspi != NULL) /* hspi handle is preconfigured by the user */
  {
    p_handler->handlePtr = hspi;
    /* To be done Other parameter to be set */
  }
  else /* default spi handle is selected */
  {
    if (sys_spi_init() == true)
    {
      sys_spi_acquire(SYS_SPI_ST33_CONFIGURATION);
      MX_LOC_SPI_Init();
      sys_spi_release(SYS_SPI_ST33_CONFIGURATION);

      spidevhnd.cs_pin =    &cs_pin;
      spidevhnd.cs_port =   ST33_SPI_CS_PORT;
      spidevhnd.hspi =      &ST33_SPI_HANDLE;
      spidevhnd.power =     NULL;
      p_handler->handlePtr = &spidevhnd;
      p_handler->data = ST33Data;
    }
  }

  NDLC_Initialization(p_handler);

  if (p_handler->data == 0)
  {
    result = false;
  }
  else
  {
    result = true;
  }

  return (result);
}

bool spi_ndlc_deinit(ndlc_device_t *p_handler)
{
  /* if (p_handler->handlePtr == NULL) */
  /* { */
  sys_spi_acquire(SYS_SPI_ST33_CONFIGURATION);
  MX_LOC_SPI_DeInit(spidevhnd.hspi);
  sys_spi_release(SYS_SPI_ST33_CONFIGURATION);
  /* } */

  NDLC_DeInitialization(p_handler);

  return (true);
}

bool spi_ndlc_power(ndlc_device_t *p_handler, bool state_on)
{
  UNUSED(p_handler);
  bool result;

  /*
    if ( ((struct mbedSpiHnd*)dev->handlePtr)->power != 0)
    {
      return -1;
    }

    DigitalOut * powerPin =(DigitalOut * ) ((struct mbedSpiHnd*)dev->handlePtr)->power;

    switch(state)
    {
      case DEVICE_OFF:
        powerPin->write(0);
        break;

      case DEVICE_ON:
        powerPin->write(1);
        break;

      default:
        return -1;
    }
  */

  if (state_on == true)
  {
    result = sys_spi_power_on();
  }
  else
  {
    result = sys_spi_power_off();
  }

  return (result);
}

int32_t spi_ndlc_atr(ndlc_device_t *p_handler)
{
  int32_t result;
  /* spi_ndlc_power(dev, DEVICE_OFF); */
  /* wait_ms(50); */
  /* spi_ndlc_power(dev, DEVICE_ON); */
  /* wait_ms(50); */

  sys_spi_acquire(SYS_SPI_ST33_CONFIGURATION);
  MX_LOC_SPI_Init();

  result = NDLC_abort(p_handler, p_handler->data);

  sys_spi_release(SYS_SPI_ST33_CONFIGURATION);

  return (result);
}

int32_t spi_ndlc_transceive_apdu(ndlc_device_t *p_handler, uint8_t *buffer_tx, uint16_t buffer_tx_len,
                                 uint8_t *buffer_rx)
{
  int32_t result;

  sys_spi_acquire(SYS_SPI_ST33_CONFIGURATION);
  MX_LOC_SPI_Init();

  result = NDLC_apdu(p_handler, buffer_tx, buffer_tx_len, buffer_rx);

  sys_spi_release(SYS_SPI_ST33_CONFIGURATION);

  return (result);
}

int32_t spi_ndlc_abort_apdu(ndlc_device_t *p_handler)
{
  int32_t result;

  sys_spi_acquire(SYS_SPI_ST33_CONFIGURATION);
  MX_LOC_SPI_Init();

  result = NDLC_abort(p_handler, p_handler->data);
  /* print_buffer_line("abort rsp dump", dev->dataLen, dev->data); */

  sys_spi_release(SYS_SPI_ST33_CONFIGURATION);

  return (result);
}


int32_t spi_ndlc_send_receive_phy(void *p_handler, uint16_t length, uint8_t *tx_data, uint8_t *rx_data)
{
  STSpiDevHnd_t *stspidevhnd = (STSpiDevHnd_t *)p_handler;
  int32_t spi_result = SPI_NDLC_COMMUNICATION_ERROR;

  print_buffer_line((SPI_NDLC_CHAR_t *)">>", length, tx_data);

  if (stspidevhnd->cs_port != NULL)
  {
    HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);
    HAL_Delay(30);
    HAL_GPIO_WritePin(stspidevhnd->cs_port, *(stspidevhnd->cs_pin), GPIO_PIN_RESET);
  }
  if ((tx_data != NULL) || (rx_data != NULL))
  {
    if (HAL_SPI_TransmitReceive(stspidevhnd->hspi, tx_data, rx_data, length, 10) == HAL_OK)
    {
      spi_result = (int32_t)length;
    }
  }
  if (stspidevhnd->cs_port != NULL)
  {
    HAL_GPIO_WritePin(stspidevhnd->cs_port, *(stspidevhnd->cs_pin), GPIO_PIN_SET);
    HAL_Delay(30);
  }
  print_buffer_line((SPI_NDLC_CHAR_t *)"<<", length, rx_data);

  return (spi_result);
}

#endif /* USE_ST33 == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

