/**
  ******************************************************************************
  * @file    time_date.h
  * @author  MCD Application Team
  * @brief   Header for time_date.c module
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
#ifndef TIME_DATE_H
#define TIME_DATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* set or get command type */
typedef uint8_t timedate_cmd_t;
#define TIMEDATE_TIME          (timedate_cmd_t)0U               /* set or get time only     */
#define TIMEDATE_DATE          (timedate_cmd_t)1U               /* set or get date only     */
#define TIMEDATE_DATE_AND_TIME (timedate_cmd_t)2U               /* set or get time and date */

/* timedate status */
typedef uint8_t timedate_status_t;
#define TIMEDATE_STATUS_OK          ((timedate_status_t)0U)     /* date and time valid      */
#define TIMEDATE_STATUS_INVALID     ((timedate_status_t)1U)     /* date and time invalid    */
#define TIMEDATE_STATUS_FAIL        ((timedate_status_t)2U)     /* function FAIL            */

/* time date structure */
typedef struct
{
  uint8_t sec;    /*!< seconds 0-61 */
  uint8_t min;    /*!< minutes 0-59 */
  uint8_t hour;   /*!< hours 0-23 */
  uint8_t mday;   /*!< day of the month  1-31   (based on RTC HAL values)*/
  uint8_t month;  /*!< month since january 1-12 (based on RTC HAL values) */
  uint16_t year;   /*!< year since 1970 */
  uint8_t wday;   /*!< day since monday 1-7 (based on RTC HAL values) */
  uint8_t yday;   /*!< days since January 1 */
  uint32_t isdst;  /*!< daylight saving time 0-365 */
} timedate_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
timedate_status_t timedate_set_from_http(uint8_t *dateStr);
timedate_status_t timedate_set(timedate_t *time, timedate_cmd_t set_cmd);
timedate_status_t timedate_get(timedate_t *time, timedate_cmd_t set_cmd);

#if (!USE_DEFAULT_SETUP == 1)
void timedate_setup_handler(void);
void timedate_setup_dump(void);
#endif /* (!USE_DEFAULT_SETUP == 1) */

#ifdef __cplusplus
}
#endif

#endif /* TIME_DATE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
