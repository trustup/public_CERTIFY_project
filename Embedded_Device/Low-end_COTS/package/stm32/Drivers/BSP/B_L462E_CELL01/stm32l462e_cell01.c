/**
  ******************************************************************************
  * @file    stm32l462e_cell01.c
  * @author  MCD Application Team
  * @brief   STM32L462E-CELL01 board support package
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell01.h"

/** @defgroup BSP BSP
  * @{
  */

/** @defgroup STM32L462E_CELL01 STM32L462E_CELL01
  * @{
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL LOW LEVEL
  * @{
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Private_Defines LOW LEVEL Private Def
  * @{
  */
/**
  * @brief STM32L475E IOT01 BSP Driver version number
   */
#define __STM32L462E_CELL01_BSP_VERSION_MAIN   (0x01) /*!< [31:24] main version */
#define __STM32L462E_CELL01_BSP_VERSION_SUB1   (0x01) /*!< [23:16] sub1 version */
#define __STM32L462E_CELL01_BSP_VERSION_SUB2   (0x02) /*!< [15:8]  sub2 version */
#define __STM32L462E_CELL01_BSP_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32L462E_CELL01_BSP_VERSION        ((__STM32L462E_CELL01_BSP_VERSION_MAIN << 24)\
                                                |(__STM32L462E_CELL01_BSP_VERSION_SUB1 << 16)\
                                                |(__STM32L462E_CELL01_BSP_VERSION_SUB2 << 8 )\
                                                |(__STM32L462E_CELL01_BSP_VERSION_RC))
/**
  * @}
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Private_Variables LOW LEVEL Variables
  * @{
  */

const uint32_t GPIO_PIN[BSP_LEDn] = {LED1_PIN, LED2_PIN, LED3_PIN};

GPIO_TypeDef *GPIO_PORT[BSP_LEDn] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT};

GPIO_TypeDef *BUTTON_PORT[BUTTONn] = {USER_BUTTON_GPIO_PORT};

const uint16_t BUTTON_PIN[BUTTONn] = {USER_BUTTON_PIN};

const uint16_t BUTTON_IRQn[BUTTONn] = {USER_BUTTON_EXTI_IRQn};

extern I2C_HandleTypeDef hi2c1;

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

/**
  * @}
  */
/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Private_FunctionPrototypes LOW LEVEL Private Function Prototypes
  * @{
  */


static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                           uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                            uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2Cx_IsDeviceReady(I2C_HandleTypeDef *i2c_handler, uint16_t DevAddress, uint32_t Trials);
static void              I2Cx_Error(I2C_HandleTypeDef *i2c_handler, uint8_t Addr);

/* Sensors IO functions */
void     SENSOR_IO_Init(void);
void     SENSOR_IO_DeInit(void);
void     SENSOR_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
uint8_t  SENSOR_IO_Read(uint8_t Addr, uint8_t Reg);
uint16_t SENSOR_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length);
void     SENSOR_IO_WriteMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length);
HAL_StatusTypeDef SENSOR_IO_IsDeviceReady(uint16_t DevAddress, uint32_t Trials);
void     SENSOR_IO_Delay(uint32_t Delay);

/* LCD IO functions */
void    LCD_IO_Init(void);
void    LCD_IO_DeInit(void);
void    LCD_IO_WriteCommand(uint8_t Cmd);
void    LCD_IO_WriteData(uint8_t Value);
void    LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size);
void    LCD_Delay(uint32_t delay);

void    LCD_BUS_Init(void);
void    LCD_BUS_DeInit(void);

/**
  * @}
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Private_Functions LOW LEVEL Private Functions
  * @{
  */

/**
  * @brief  This method returns the STM32L475E IOT01 BSP Driver revision
  * @retval version: 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t BSP_GetVersion(void)
{
  return __STM32L462E_CELL01_BSP_VERSION;
}

/**
  * @brief  Configures LEDs.
  * @param  Led: LED to be configured.
  *         This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
  */
void BSP_LED_Init(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  LEDx_GPIO_CLK_ENABLE(Led);
  /* Configure the GPIO_LED pin */
  gpio_init_structure.Pin   = GPIO_PIN[Led];
  gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull  = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(GPIO_PORT[Led], &gpio_init_structure);
  /* By default, turn off LED */
  HAL_GPIO_WritePin(GPIO_PORT[Led], gpio_init_structure.Pin, GPIO_PIN_RESET);
}

/**
  * @brief  DeInit LEDs.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
  */
void BSP_LED_DeInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* DeInit the GPIO_LED pin */
  gpio_init_structure.Pin = GPIO_PIN[Led];

  /* Turn off LED */
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
  HAL_GPIO_DeInit(GPIO_PORT[Led], gpio_init_structure.Pin);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: LED to be set on
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: LED to be set off
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: LED to be toggled
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Configures button GPIO and EXTI Line.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @param  ButtonMode: Button mode
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_MODE_GPIO: Button will be used as simple IO
  *            @arg  BUTTON_MODE_EXTI: Button will be connected to EXTI line
  *                                    with interrupt generation capability
  * @retval None
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode)
{
  GPIO_InitTypeDef gpio_init_structure;

  USER_BUTTON_GPIO_CLK_ENABLE();

  if (ButtonMode == BUTTON_MODE_GPIO)
  {
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);
  }

  if (ButtonMode == BUTTON_MODE_EXTI)
  {
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);

    HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  }
}

/**
  * @brief  Push Button DeInit.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @note PB DeInit does not disable the GPIO clock
  * @retval None
  */
void BSP_PB_DeInit(Button_TypeDef Button)
{
  GPIO_InitTypeDef gpio_init_structure;

  gpio_init_structure.Pin = BUTTON_PIN[Button];
  HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  HAL_GPIO_DeInit(BUTTON_PORT[Button], gpio_init_structure.Pin);
}


/**
  * @brief  Returns the selected button state.
  * @param  Button: Button to be checked
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @retval The Button GPIO pin value (GPIO_PIN_RESET = button pressed)
  */
uint32_t BSP_PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}


/*******************************************************************************
                            BUS OPERATIONS
  *******************************************************************************/

/**
  * @brief  Reads multiple data.
  * @param  i2c_handler : I2C handler
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  MemAddress: memory address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                           uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Read(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if (status != HAL_OK)
  {
    /* I2C error occurred */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}


/**
  * @brief  Writes a value in a register of the device through BUS in using DMA mode.
  * @param  i2c_handler : I2C handler
  * @param  Addr: Device address on BUS Bus.
  * @param  Reg: The target register address to write
  * @param  MemAddress: memory address
  * @param  Buffer: The target register value to be written
  * @param  Length: buffer size to be written
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                            uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Write(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if (status != HAL_OK)
  {
    /* Re-Initiaize the I2C Bus */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}

/**
  * @brief  Checks if target device is ready for communication.
  * @note   This function is used with Memory devices
  * @param  i2c_handler : I2C handler
  * @param  DevAddress: Target device address
  * @param  Trials: Number of trials
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_IsDeviceReady(I2C_HandleTypeDef *i2c_handler, uint16_t DevAddress, uint32_t Trials)
{
  return (HAL_I2C_IsDeviceReady(i2c_handler, DevAddress, Trials, 1000));
}

/**
  * @brief  Manages error callback by re-initializing I2C.
  * @param  i2c_handler : I2C handler
  * @param  Addr: I2C Address
  * @retval None
  */
static void I2Cx_Error(I2C_HandleTypeDef *i2c_handler, uint8_t Addr)
{
  /* De-initialize the I2C communication bus */
  HAL_I2C_DeInit(i2c_handler);
}



/**
  * @}
  */

/*******************************************************************************
                            LINK OPERATIONS
  *******************************************************************************/
/******************************** LINK Sensors ********************************/

/**
  * @brief  Initializes Sensors low level.
  * @retval None
  */
void SENSOR_IO_Init(void)
{
  __NOP();
}

/**
  * @brief  DeInitializes Sensors low level.
  * @retval None
  */
void SENSOR_IO_DeInit(void)
{
  __NOP();
}

/**
  * @brief  Writes a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
void SENSOR_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  I2Cx_WriteMultiple(&hi2c1, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&Value, 1);
}

/**
  * @brief  Reads a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @retval Data to be read
  */
uint8_t SENSOR_IO_Read(uint8_t Addr, uint8_t Reg)
{
  uint8_t read_value = 0;

  I2Cx_ReadMultiple(&hi2c1, Addr, Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&read_value, 1);

  return read_value;
}

/**
  * @brief  Reads multiple data with I2C communication
  *         channel from Sensor.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval HAL status
  */
uint16_t SENSOR_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
  return I2Cx_ReadMultiple(&hi2c1, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

/**
  * @brief  Writes multiple data with I2C communication
  *         channel from MCU to Sensor.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval None
  */
void SENSOR_IO_WriteMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
  I2Cx_WriteMultiple(&hi2c1, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

/**
  * @brief  Checks if target device is ready for communication.
  * @note   This function is used with Memory devices
  * @param  DevAddress: Target device address
  * @param  Trials: Number of trials
  * @retval HAL status
  */
HAL_StatusTypeDef SENSOR_IO_IsDeviceReady(uint16_t DevAddress, uint32_t Trials)
{
  return (I2Cx_IsDeviceReady(&hi2c1, DevAddress, Trials));
}

/**
  * @brief  Delay function used in Sensor low level driver.
  * @param  Delay: Delay in ms
  * @retval None
  */
void SENSOR_IO_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}


/**
  * @brief  Initializes lcd low level.
  * @retval None
  */

void LCD_IO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_DISP_GPIO_PORT, RST_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin : D_C_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = D_C_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin : RST_DISP_PIN */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  GPIO_InitStruct.Pin = RST_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(RST_DISP_GPIO_PORT, &GPIO_InitStruct);

  /* Configure GPIO pin : CS_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = CS_DISP_PIN;
  /* GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; */  /* Already done in previous line */
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(CS_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ST33_CS_GPIO_Port, ST33_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RST_DISP_GPIO_PORT, RST_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(RST_DISP_GPIO_PORT, RST_DISP_PIN, GPIO_PIN_SET);
}

void LCD_IO_DeInit(void)
{
  if (HAL_SPI_GetState(&LCD_SPI_HANDLE) != HAL_SPI_STATE_RESET)
  {
    /* SPI Deinit */
    HAL_SPI_DeInit(&LCD_SPI_HANDLE);
    HAL_GPIO_DeInit(GPIOC, (ST33_MOSI_Pin | ST33_MISO_Pin | ST33_SCK_Pin));
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();

    HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);

    /* Disable SPIx clock  */
    __HAL_RCC_GPIOC_CLK_DISABLE();
  }
}

void LCD_BUS_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  if ((HAL_SPI_GetState(&LCD_SPI_HANDLE) == HAL_SPI_STATE_RESET)
      || (HAL_SPI_GetState(&LCD_SPI_HANDLE) == HAL_SPI_STATE_READY))
  {
    LCD_SPI_HANDLE.Instance = LCD_SPI_INSTANCE;
    LCD_SPI_HANDLE.Init.Mode = SPI_MODE_MASTER;
    LCD_SPI_HANDLE.Init.Direction = SPI_DIRECTION_2LINES;
    LCD_SPI_HANDLE.Init.DataSize = SPI_DATASIZE_8BIT;
    LCD_SPI_HANDLE.Init.CLKPolarity = SPI_POLARITY_LOW;
    LCD_SPI_HANDLE.Init.CLKPhase = SPI_PHASE_1EDGE;
    LCD_SPI_HANDLE.Init.NSS = SPI_NSS_SOFT;
    LCD_SPI_HANDLE.Init.BaudRatePrescaler = LCD_SPI_BAUDRATEPRESCALER;
    LCD_SPI_HANDLE.Init.FirstBit = SPI_FIRSTBIT_MSB;
    LCD_SPI_HANDLE.Init.TIMode = SPI_TIMODE_DISABLE;
    LCD_SPI_HANDLE.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    LCD_SPI_HANDLE.Init.CRCPolynomial = 7;
    LCD_SPI_HANDLE.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    LCD_SPI_HANDLE.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    /* enable SPI GPIO clock */
    LCD_SPI_GPIO_CLOCK_ENABLE();
    /* enable MSP SPI CLK */
    LCD_SPI_CLOCK_ENABLE();

    GPIO_InitStruct.Pin = LCD_SPI_MOSI_PIN | LCD_SPI_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = LCD_SPI_ALT_FUNCTION;
    HAL_GPIO_Init(LCD_SPI_PORT, &GPIO_InitStruct);
    HAL_SPI_Init(&LCD_SPI_HANDLE);
    HAL_GPIO_WritePin(LCD_SPI_CS_PORT, LCD_SPI_CS_PIN, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = D_C_DISP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    /* GPIO_InitStruct.Pull = GPIO_NOPULL; */ /* Already done in previous line */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_RESET);
  }
}

void LCD_BUS_DeInit(void)
{
  if (HAL_SPI_GetState(&LCD_SPI_HANDLE) != HAL_SPI_STATE_RESET)
  {
    /* SPI Deinit */
    HAL_SPI_DeInit(&LCD_SPI_HANDLE);
    HAL_GPIO_DeInit(LCD_SPI_PORT, (LCD_SPI_MOSI_PIN | LCD_SPI_SCK_PIN));
    HAL_GPIO_DeInit(D_C_DISP_GPIO_PORT, D_C_DISP_PIN);

    LCD_SPI_FORCE_RESET();
    LCD_SPI_FORCE_RELEASE_RESET();
    HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);
    /* Disable SPIx clock  */
    LCD_SPI_CLOCK_DISABLE();
  }
}

void LCD_IO_WriteCommand(uint8_t Cmd)
{
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_SPI_Transmit(&LCD_SPI_HANDLE, &Cmd, 1, 5);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

void LCD_IO_WriteData(uint8_t Value)
{
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_SPI_Transmit(&LCD_SPI_HANDLE, &Value, 1, 5);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_SPI_Transmit(&LCD_SPI_HANDLE, pData, Size, 5 * Size);
  HAL_Delay(1);
  HAL_GPIO_WritePin(D_C_DISP_GPIO_PORT, D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_DISP_GPIO_PORT, CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

void LCD_Delay(uint32_t delay)
{
  HAL_Delay(delay);
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
