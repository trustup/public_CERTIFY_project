/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
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
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* MISRAC messages linked to HAL include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
#include "stm32l462e_cell01.h"
/*cstat +MISRAC2012-* */

#include "main.h"
#include "plf_modem_config.h"
#include "usart.h" /* for huartX */

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/
#define SPI3_MOSI_PIN     GPIO_PIN_12
#define SPI3_MISO_PIN     GPIO_PIN_11
#define SPI3_PORT         GPIOC
#define SPI3_SCK_PIN      GPIO_PIN_10
#define SPI3_ALT_FUNCTION GPIO_AF6_SPI3
#define SPI3_GPIO_CLOCK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define SPI3_GPIO_CLOCK_DISABLE()     __HAL_RCC_GPIOC_CLK_DISABLE()
#define SPI3_CLOCK_ENABLE()           __HAL_RCC_SPI3_CLK_ENABLE()
#define SPI3_CLOCK_DISABLE()          __HAL_RCC_SPI3_CLK_DISABLE()
#define SPI3_FORCE_RESET()            __HAL_RCC_SPI3_FORCE_RESET()
#define SPI3_FORCE_RELEASE_RESET()    __HAL_RCC_SPI3_RELEASE_RESET()

/* LCD and ST33 share same SPI */
/* LCD configuration */
#define LCD_SPI_HANDLE                  hspi3
#define LCD_SPI_INSTANCE                SPI3
#define LCD_SPI_BAUDRATEPRESCALER       SPI_BAUDRATEPRESCALER_4
#define LCD_SPI_MOSI_PIN                SPI3_MOSI_PIN
#define LCD_SPI_MISO_PIN                0
#define LCD_SPI_PORT                    SPI3_PORT
#define LCD_SPI_CS_PIN                  GPIO_PIN_2
#define LCD_SPI_CS_PORT                 GPIOB
#define LCD_SPI_ALT_FUNCTION            SPI3_ALT_FUNCTION
#define LCD_SPI_SCK_PIN                 SPI3_SCK_PIN
#define LCD_SPI_GPIO_CLOCK_ENABLE()     SPI3_GPIO_CLOCK_ENABLE()
#define LCD_SPI_GPIO_CLOCK_DISABLE()    SPI3_GPIO_CLOCK_DISABLE()
#define LCD_SPI_CLOCK_ENABLE()          SPI3_CLOCK_ENABLE()
#define LCD_SPI_CLOCK_DISABLE()         SPI3_CLOCK_DISABLE()
#define LCD_SPI_FORCE_RESET()           SPI3_FORCE_RESET()
#define LCD_SPI_FORCE_RELEASE_RESET()   SPI3_FORCE_RELEASE_RESET()

/* ST33 configuration */
#define ST33_SPI_HANDLE                 hspi3
#define ST33_SPI_INSTANCE               SPI3
#define ST33_SPI_BAUDRATEPRESCALER      SPI_BAUDRATEPRESCALER_8
#define ST33_SPI_MOSI_PIN               SPI3_MOSI_PIN
#define ST33_SPI_MISO_PIN               SPI3_MISO_PIN
#define ST33_SPI_MOSI_MISO_SCK_PORT     SPI3_PORT
#define ST33_SPI_CS_PIN                 ST33_CS_Pin
#define ST33_SPI_CS_PORT                ST33_CS_GPIO_Port
#define ST33_SPI_ALT_FUNCTION           SPI3_ALT_FUNCTION
#define ST33_SPI_SCK_PIN                SPI3_SCK_PIN

/* MODEM configuration */
#define USE_DISCO_L462   (1) /* DISCO L462 */
#define USE_NUCLEO_64    (0) /* NUCLEO standard */

#define NDLC_SPI_INTERFACE (1)

#if defined(DISCO_L462)
#define HW_SETUP       (USE_DISCO_L462)
#if !defined USE_DISPLAY
#define USE_DISPLAY    (1)  /* 0: not activated, 1: activated */
#define DISPLAY_WAIT_MODEM_IS_ON (1U) /* Must be activated */
#endif /* !defined USE_DISPLAY */
#if !defined USE_ST33
#define USE_ST33       (1)  /* 0: not activated, 1: activated */
#endif /* !defined USE_ST33 */
#define NDLC_INTERFACE NDLC_SPI_INTERFACE
#endif /* DISCO_L462 */

#if defined(NUCLEO_64)
#define HW_SETUP       (USE_NUCLEO_64)
#define USE_DISPLAY    (0) /* DISPLAY NOT AVAILABLE */
#define DISPLAY_WAIT_MODEM_IS_ON (0U) /* No display */
#define USE_ST33       (0) /* ST33 NOT AVAILABLE */
#define NDLC_INTERFACE
#endif /* NUCLEO_64 */

#if (HW_SETUP == USE_DISCO_L462)
#define MODEM_UART_HANDLE       huart3
#define MODEM_UART_INSTANCE     USART3
#define MODEM_UART_AUTOBAUD     (0)
#define MODEM_UART_IRQN         USART3_IRQn
#else /* HW_SETUP == USE_NUCLEO_64 */
#define MODEM_UART_HANDLE       huart2
#define MODEM_UART_INSTANCE     USART2
#define MODEM_UART_AUTOBAUD     (0)
#define MODEM_UART_IRQN         USART2_IRQn
#endif /* HW_SETUP == USE_DISCO_L462 */

#define MODEM_UART_BAUDRATE     (CONFIG_MODEM_UART_BAUDRATE)
#define MODEM_UART_WORDLENGTH   UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS     UART_STOPBITS_1
#define MODEM_UART_PARITY       UART_PARITY_NONE
#define MODEM_UART_MODE         UART_MODE_TX_RX

#if (CONFIG_MODEM_UART_RTS_CTS == 1)
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_RTS_CTS
#else
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_NONE
#endif /* (CONFIG_MODEM_UART_RTS_CTS == 1) */

#if (HW_SETUP == USE_DISCO_L462)
#define MODEM_TX_GPIO_PORT      ((GPIO_TypeDef *)MDM_UART_TX_GPIO_Port)
#define MODEM_TX_PIN            MDM_UART_TX_Pin
#define MODEM_RX_GPIO_PORT      ((GPIO_TypeDef *)MDM_UART_RX_GPIO_Port)
#define MODEM_RX_PIN            MDM_UART_RX_Pin
#define MODEM_CTS_GPIO_PORT     ((GPIO_TypeDef *)MDM_UART_CTS_GPIO_Port)
#define MODEM_CTS_PIN           MDM_UART_CTS_Pin
#define MODEM_RTS_GPIO_PORT     ((GPIO_TypeDef *)MDM_UART_RTS_GPIO_Port)
#define MODEM_RTS_PIN           MDM_UART_RTS_Pin

/* ---- MODEM other pins configuration ---- */
/* output */
#define MODEM_RST_GPIO_PORT             MDM_RST_OUT_GPIO_Port
#define MODEM_RST_PIN                   MDM_RST_OUT_Pin
#define MODEM_PWR_EN_GPIO_PORT          MDM_PWR_EN_OUT_GPIO_Port
#define MODEM_PWR_EN_PIN                MDM_PWR_EN_OUT_Pin
#define MODEM_DTR_GPIO_PORT             MDM_DTR_OUT_GPIO_Port
#define MODEM_DTR_PIN                   MDM_DTR_OUT_Pin
/* input */
#define MODEM_RING_GPIO_PORT    ((GPIO_TypeDef *)MDM_RING_GPIO_Port)
#define MODEM_RING_PIN          MDM_RING_Pin
#define MODEM_RING_IRQN         MDM_RING_EXTI_IRQn

#else /* HW_SETUP == USE_NUCLEO_64 */
#define MODEM_RST_GPIO_PORT       ((GPIO_TypeDef *)MDM_RST_OUT_GPIO_Port) /* not connected on EVK */
#define MODEM_RST_PIN             MDM_RST_OUT_Pin /* not connected on EVK */
#define MODEM_PWR_EN_GPIO_PORT    ((GPIO_TypeDef *)ARD_A5_GPIO_Port)
#define MODEM_PWR_EN_PIN          ARD_A5_Pin

#define MODEM_DTR_GPIO_PORT       ((GPIO_TypeDef *)GPIOB)
#define MODEM_DTR_PIN             GPIO_PIN_5

#define MODEM_RING_GPIO_PORT    ((GPIO_TypeDef *)ARD_D7_GPIO_Port)
#define MODEM_RING_PIN          ARD_D7_Pin
#define MODEM_RING_IRQN         EXTI9_5_IRQn

#define MODEM_CTS_GPIO_PORT     ((GPIO_TypeDef *)ARD_A0_GPIO_Port)
#define MODEM_CTS_PIN           ARD_A0_Pin
#define MODEM_RTS_GPIO_PORT     ((GPIO_TypeDef *)ARD_A1_GPIO_Port)
#define MODEM_RTS_PIN           ARD_A1_Pin

#define MODEM_RX_GPIO_PORT      ((GPIO_TypeDef *)ARD_D0_GPIO_Port)
#define MODEM_RX_PIN            ARD_D0_Pin
#define MODEM_TX_GPIO_PORT      ((GPIO_TypeDef *)ARD_D1_GPIO_Port)
#define MODEM_TX_PIN            ARD_D1_Pin
#endif /* HW_SETUP == USE_DISCO_L462 */

#define PPPOS_LINK_UART_HANDLE   NULL
#define PPPOS_LINK_UART_INSTANCE NULL

#define BUTTON_USER_PIN            USER_BUTTON_Pin

/* Resource BUTTON definition */
#define NO_BUTTON            (0xFF) /* value to use when Button is NOT defined or mapped
                                       or to do nothing when Button interruption is received */

/* change to NO_BUTTON in order to do nothing  */
#define USER_BUTTON          BUTTON_USER     /* Defined and mapped on BUTTON_USER */
#define UP_BUTTON            NO_BUTTON       /* NOT defined or mapped             */
#define DOWN_BUTTON          NO_BUTTON       /* NOT defined or mapped             */
#define RIGHT_BUTTON         NO_BUTTON       /* NOT defined or mapped             */
#define LEFT_BUTTON          NO_BUTTON       /* NOT defined or mapped             */
#define SEL_BUTTON           NO_BUTTON       /* NOT defined or mapped             */
#define BUTTONS_NB           (1U)

/* Resource LED definition */
#define NO_LED               ((uint8_t)0xFF)   /* value to use when Led is NOT defined or mapped
                                                  or to do nothing when Led init/on/off services are called */

#define BOARD_LEDS_1         ((uint8_t)BSP_LED1)      /* Defined and mapped */
#define BOARD_LEDS_2         ((uint8_t)BSP_LED2)      /* Defined and mapped */
#define BOARD_LEDS_3         ((uint8_t)BSP_LED3)      /* Defined and mapped */

/* Some Samples prefer to use Color Leds terminology */
#define GREEN_LED            ((uint8_t)BSP_LED_GREEN) /* Defined and mapped */
#define RED_LED              ((uint8_t)BSP_LED_RED)   /* Defined and mapped */
#define BLUE_LED             ((uint8_t)BSP_LED_BLUE)  /* Defined and mapped */

/* Some Samples prefer to use Naming Leds terminology */
#define DATAREADY_LED        ((uint8_t)BSP_LED_GREEN) /* Defined and mapped */
#define CLOUD_LED            ((uint8_t)BSP_LED_RED)   /* Defined and mapped */
#define OTHER_LED            ((uint8_t)BSP_LED_BLUE)  /* Defined and mapped */

#define LEDS_NB              (3U)


/* Flash configuration   */
#define FLASH_LAST_PAGE_ADDR     ((uint32_t)0x0807f800) /* Base @ of Page 255, 2 Kbytes */
#define FLASH_LAST_PAGE_NUMBER     255
#define FLASH_BANK_NUMBER          FLASH_BANK_1

/* DEBUG INTERFACE CONFIGURATION */

/* use test board trace UART - DEFAULT */
#if (HW_SETUP == USE_DISCO_L462)
#define TRACE_INTERFACE_UART_HANDLE     huart1
#define TRACE_INTERFACE_INSTANCE        USART1

#define COM_INTERFACE_UART_HANDLE      huart2
#define COM_INTERFACE_INSTANCE        ((USART_TypeDef *)USART2)
#define COM_INTERFACE_UART_IRQ         USART2_IRQn
#define COM_INTERFACE_UART_INIT        MX_USART2_UART_Init(); \
  HAL_NVIC_EnableIRQ(COM_INTERFACE_UART_IRQ);

#else /* HW_SETUP == USE_NUCLEO_64 */
#define TRACE_INTERFACE_UART_HANDLE     huart1
#define TRACE_INTERFACE_INSTANCE        USART1
#endif /* HW_SETUP == USE_DISCO_L462 */


#define D_C_DISP_PIN GPIO_PIN_11
#define D_C_DISP_GPIO_PORT GPIOC
#define RST_DISP_PIN GPIO_PIN_1
#define RST_DISP_GPIO_PORT GPIOH
#define CS_DISP_PIN GPIO_PIN_2
#define CS_DISP_GPIO_PORT GPIOB
/* #define ST33_NSS_PIN GPIO_PIN_4 */
/* #define ST33_NSS_GPIO_PORT GPIOA */

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
