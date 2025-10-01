/**
  ******************************************************************************
  * @file    stm32l462e_cell01.h
  * @author  MCD Application Team
  * @brief   STM32L462E_CELL01 board support package
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L462E_CELL01_H
#define __STM32L462E_CELL01_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "plf_hw_config.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL01
  * @{
  */

/** @addtogroup STM32L462E_CELL01_LOW_LEVEL
  * @{
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Exported_Types LOW LEVEL Exported Types
  * @{
  */
typedef enum
{
  BSP_LED1 = 0x0,
  BSP_LED2 = 0x1,
  BSP_LED3 = 0x2,
  BSP_LEDn,
  BSP_LED_GREEN = BSP_LED1,
  BSP_LED_RED   = BSP_LED2,
  BSP_LED_BLUE  = BSP_LED3,
} Led_TypeDef;


typedef enum
{
  BUTTON_USER = 0
} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;


/**
  * @}
  */

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Exported_Constants LOW LEVEL Exported Constants
  * @{
  */

/**
  * @brief  Define for STM32L462E_CELL01 board
  */
#if !defined (USE_STM32L462E_CELL01)
#define USE_STM32L462E_CELL01
#endif /* !defined (USE_STM32L462E_CELL01) */

#define LEDn                             ((uint8_t)3)

#define LED1_PIN                         GPIO_PIN_6
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define LED1_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOC_CLK_DISABLE()

#define LED2_PIN                         GPIO_PIN_15
#define LED2_GPIO_PORT                   GPIOB
#define LED2_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED2_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()

#define LED3_PIN                         GPIO_PIN_14
#define LED3_GPIO_PORT                   GPIOB
#define LED3_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED3_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()


#define LEDx_GPIO_CLK_ENABLE(__INDEX__)   do{ if((__INDEX__) == BSP_LED1) { LED1_GPIO_CLK_ENABLE();   } else \
                                              if((__INDEX__) == BSP_LED2) { LED2_GPIO_CLK_ENABLE();   } else \
                                              if((__INDEX__) == BSP_LED3) { LED3_GPIO_CLK_ENABLE(); } } while(0)

#define LEDx_GPIO_CLK_DISABLE(__INDEX__)  do{ if((__INDEX__) == BSP_LED1) { LED1_GPIO_CLK_DISABLE();   } else \
                                                if((__INDEX__) == BSP_LED2) { LED2_GPIO_CLK_DISABLE();   } else \
                                                if((__INDEX__) == BSP_LED3) { LED3_GPIO_CLK_DISABLE(); } } while(0)

/* Only one User/Wakeup button */
#define BUTTONn                             ((uint8_t)1)

/**
  * @brief Wakeup push-button
  */
#define USER_BUTTON_PIN                   GPIO_PIN_13
#define USER_BUTTON_GPIO_PORT             GPIOC
#define USER_BUTTON_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOC_CLK_ENABLE()
#define USER_BUTTON_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOC_CLK_DISABLE()
#define USER_BUTTON_EXTI_IRQn             EXTI15_10_IRQn

/* User can use this section to tailor I2Cx instance used and associated resources */
/* Definition for I2Cx resources */

#define DISCOVERY_I2Cx                             I2C1
#define DISCOVERY_I2Cx_CLK_ENABLE()                __HAL_RCC_I2C1_CLK_ENABLE()
#define DISCOVERY_I2Cx_CLK_DISABLE()               __HAL_RCC_I2C1_CLK_DISABLE()
#define DISCOVERY_I2Cx_SCL_SDA_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define DISCOVERY_I2Cx_SCL_SDA_GPIO_CLK_DISABLE()  __HAL_RCC_GPIOB_CLK_DISABLE()

#define DISCOVERY_I2Cx_FORCE_RESET()               __HAL_RCC_I2C1_FORCE_RESET()
#define DISCOVERY_I2Cx_RELEASE_RESET()             __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define DISCOVERY_I2Cx_SCL_PIN                     ARD_D15_Pin
#define DISCOVERY_I2Cx_SDA_PIN                     ARD_D14_Pin
#define DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT           GPIOB
#define DISCOVERY_I2Cx_SCL_SDA_AF                  GPIO_AF4_I2C1

/* I2C interrupt requests */
#define DISCOVERY_I2Cx_EV_IRQn                     I2C1_EV_IRQn
#define DISCOVERY_I2Cx_ER_IRQn                     I2C1_ER_IRQn

/* I2C clock speed configuration (in Hz)
  WARNING:
   Make sure that this define is not already declared in other files.
   It can be used in parallel by other modules. */

#ifndef DISCOVERY_I2C_SPEED
#define DISCOVERY_I2C_SPEED                             100000
#endif /* DISCOVERY_I2C_SPEED */


#ifndef DISCOVERY_I2Cx_TIMING
#define DISCOVERY_I2Cx_TIMING                     ((uint32_t)0x00702681)
#endif /* DISCOVERY_I2Cx_TIMING */


/* I2C Sensors address */
/* LPS22HB (Pressure) I2C Address */
#define LPS22HB_I2C_ADDRESS  (uint8_t)0xBA
/* HTS221 (Humidity) I2C Address */
#define HTS221_I2C_ADDRESS   (uint8_t)0xBE


#ifdef USE_LPS22HB_TEMP
/* LPS22HB Sensor hardware I2C address */
#define TSENSOR_I2C_ADDRESS     LPS22HB_I2C_ADDRESS
#else /* USE_HTS221_TEMP */
/* HTS221 Sensor hardware I2C address */
#define TSENSOR_I2C_ADDRESS     HTS221_I2C_ADDRESS
#endif /* USE_LPS22HB_TEMP */


/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup STM32L462E_CELL01_LOW_LEVEL_Exported_Functions LOW LEVEL Exported Functions
  * @{
  */
uint32_t         BSP_GetVersion(void);
void             BSP_LED_Init(Led_TypeDef Led);
void             BSP_LED_DeInit(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);
void             BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
void             BSP_PB_DeInit(Button_TypeDef Button);
uint32_t         BSP_PB_GetState(Button_TypeDef Button);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* __STM32L462E_CELL01_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
