/**
  ******************************************************************************
  * @file    time_date.c
  * @author  MCD Application Team
  * @brief   This file contains date and time utilities
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

/* module used only if USE_RTC activated */
#if (USE_RTC == 1)
#include "rtc.h"
#include "time_date.h"
#include "cellular_runtime_custom.h"
#include "cellular_runtime_standard.h"

#if (!USE_DEFAULT_SETUP == 1)
#include "menu_utils.h"
#endif /* (!USE_DEFAULT_SETUP == 1) */

/* Private macros ------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define TIMEDATE_SETUP_LABEL         "Time Date Menu"
#define TIMEDATE_DEFAULT_PARAMA_NB   1U
#define TIMEDATE_STRING_SIZE        30U
#define TIMEDATE_DOW_LEN             4U
#define TIMEDATE_MONTH_LEN           3U

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static timedate_status_t timedate_status = TIMEDATE_STATUS_INVALID;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  get day of year number
  * @param  Date     (in) date to set
  * @retval day of year
  */
static uint16_t timedate_get_yday(RTC_DateTypeDef *Date)
{
  /* number of days by month */
  static const uint8_t month_day[13] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
  uint16_t i;
  uint16_t yday;

  yday = 0U;

  /* add number of day for each finisher month */
  for (i = 1U; i < Date->Month; i++)
  {
    yday += month_day[i];
  }

  /* add number of day of the current month */
  yday += Date->Date;

  /* add a day if it is a lead year after february */
  if (((Date->Year % 4U) == 0U) && (Date->Month > 1U))
  {
    yday++;
  }

  return yday;
}

/**
  * @brief  setting date and time from http response
  * @param  dateStr     (in) http date
  * @retval return value
  */

timedate_status_t timedate_set_from_http(uint8_t *dateStr)
{
  timedate_t  timedate_data;
  /* http Day strings */
  static const uint8_t *dow_string[7] =    {(uint8_t *)"Mon,", (uint8_t *)"Tue,", (uint8_t *)"Wed,", (uint8_t *)"Thu,",
                                            (uint8_t *)"Fri,", (uint8_t *)"Sat,", (uint8_t *)"Sun,"
                                           };
  /* http Month strings */
  static const uint8_t *month_string[12] = {(uint8_t *)"Jan", (uint8_t *)"Feb", (uint8_t *)"Mar", (uint8_t *)"Apr",
                                            (uint8_t *)"May", (uint8_t *)"Jun",
                                            (uint8_t *)"Jul", (uint8_t *)"Aug", (uint8_t *)"Sep", (uint8_t *)"Oct",
                                            (uint8_t *)"Nov", (uint8_t *)"Dec"
                                           };
  /* RTC week day values */
  static const uint8_t dow_value[7]  =
  {
    RTC_WEEKDAY_MONDAY,
    RTC_WEEKDAY_TUESDAY,
    RTC_WEEKDAY_WEDNESDAY,
    RTC_WEEKDAY_THURSDAY,
    RTC_WEEKDAY_FRIDAY,
    RTC_WEEKDAY_SATURDAY,
    RTC_WEEKDAY_SUNDAY
  };

  /* current offset in http response */
  uint16_t offset;
  timedate_status_t res;
  uint16_t i;

  timedate_data.wday = RTC_WEEKDAY_SUNDAY;
  timedate_data.month = 1;
  offset = 0U;

  /*======*/
  /* day  */
  /*======*/
  for (i = 0U; i < 7U; i++)
  {
    if (memcmp((CRC_CHAR_t *)&dateStr[offset], (const CRC_CHAR_t *)dow_string[i], TIMEDATE_DOW_LEN) == 0)
    {
      timedate_data.wday = dow_value[i];
      break;
    }
  }

  if (i == 7U)
  {
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += TIMEDATE_DOW_LEN + 1U;

  /*======*/
  /* mday */
  /*======*/
  timedate_data.mday = (uint8_t)crs_atoi(&dateStr[offset]);

  /* look at the end of mday in the http string */
  for (i = 0U ; i < 4U ; i++)
  {
    if (dateStr[offset + i] == (uint8_t)' ')
    {
      break;
    }
  }

  if (i == 4U)
  {
    /* mday not valid in http string */
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += i + 1U;

  /*=======*/
  /* month */
  /*=======*/
  for (i = 0U; i < 12U; i++)
  {
    if (memcmp((CRC_CHAR_t *)&dateStr[offset], (const CRC_CHAR_t *)month_string[i], TIMEDATE_MONTH_LEN) == 0)
    {
      timedate_data.month = (uint8_t)(i + 1U);
      break;
    }
  }
  if (i == 12U)
  {
    /* month not valid in http string */
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += TIMEDATE_MONTH_LEN + 1U;

  /*======*/
  /* year */
  /*======*/
  timedate_data.year = (uint16_t)crs_atoi(&dateStr[offset]);
  for (i = 0U; i < 6U; i++)
  {
    if (dateStr[offset + i] == (uint8_t)' ')
    {
      break;
    }
  }

  if (i == 6U)
  {
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += i + 1U;

  /*======*/
  /* hour */
  /*======*/
  timedate_data.hour = (uint8_t)crs_atoi(&dateStr[offset]);
  for (i = 0U; i < 4U; i++)
  {
    if (dateStr[offset + i] == (uint8_t)':')
    {
      break;
    }
  }

  if (i == 4U)
  {
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += i + 1U;

  /*=========*/
  /* minutes */
  /*=========*/
  timedate_data.min = (uint8_t)crs_atoi(&dateStr[offset]);
  for (i = 0U; i < 4U; i++)
  {
    if (dateStr[offset + i] == (uint8_t)':')
    {
      break;
    }
  }

  if (i == 4U)
  {
    res = TIMEDATE_STATUS_FAIL;
    goto end;
  }

  offset += i + 1U;

  /*=========*/
  /* seconds */
  /*=========*/
  timedate_data.sec = (uint8_t)crs_atoi(&dateStr[offset]);

  res = timedate_set(&timedate_data, TIMEDATE_DATE_AND_TIME);
  timedate_status = res;

end:
  return res;
}

/**
  * @brief  setting date and time
  * @param  time     (in) date and time to set
  * @param  set_cmd  (in) command to apply
  * @retval return value
  */
timedate_status_t timedate_set(timedate_t *time, timedate_cmd_t set_cmd)
{
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  timedate_status_t ret;

  ret = TIMEDATE_STATUS_OK;
  if (set_cmd <= TIMEDATE_DATE_AND_TIME)
  {
    /* Set Time */
    if (set_cmd != TIMEDATE_DATE)
    {
      sTime.Hours   = (uint8_t)time->hour;
      sTime.Minutes = (uint8_t)time->min;
      sTime.Seconds = (uint8_t)time->sec;

      sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
      sTime.StoreOperation = RTC_STOREOPERATION_RESET;

      if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
      {
        ret = TIMEDATE_STATUS_FAIL;
      }
    }

    /* Set Date */
    if ((set_cmd != TIMEDATE_TIME) && (ret == TIMEDATE_STATUS_OK))
    {
      if ((time->wday > 7U) || (time->wday == 0U)
          || (time->year < 2000U) || (time->year > 2255U))
      {
        ret = TIMEDATE_STATUS_FAIL;
      }
      else
      {
        sDate.WeekDay = (uint8_t)time->wday;
        sDate.Date    = (uint8_t)time->mday;
        sDate.Month   = (uint8_t)time->month;
        sDate.Year    = (uint8_t)(time->year - 2000U);

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        {
          ret = TIMEDATE_STATUS_FAIL;
        }
      }
    }
  }
  else
  {
    ret = TIMEDATE_STATUS_FAIL;
  }

  if ((ret == TIMEDATE_STATUS_OK) && (set_cmd == TIMEDATE_DATE_AND_TIME))
  {
    timedate_status = TIMEDATE_STATUS_OK;
  }
  return ret;
}


/**
  * @brief  get system date and/or time
  * @param  time         (out) date and/or time to get
  * @param  set_cmd      (in) type of time parameter (date and/or time)
  * @retval bool  return status
  */
timedate_status_t timedate_get(timedate_t *time, timedate_cmd_t set_cmd)
{
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;
  timedate_status_t ret;

  if (set_cmd <= TIMEDATE_DATE_AND_TIME)
  {
    /* WARNING : if HAL_RTC_GetTime is called it must be called before HAL_RTC_GetDate */
    /* HAL_RTC_GetTime return always HAL_OK */
    (void)HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
    if (set_cmd != TIMEDATE_DATE)
    {
      /* Get the RTC current Time */
      time->hour  = stimestructureget.Hours;
      time->min   = stimestructureget.Minutes;
      time->sec   = stimestructureget.Seconds;
      time->isdst = 0U;  /* not managed */
    }

    /* WARNING : HAL_RTC_GetDate must be called after HAL_RTC_GetTime even if date get is not necessary */
    (void)HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
    if (set_cmd != TIMEDATE_TIME)
    {
      /* Get the RTC current Date */
      time->wday  = sdatestructureget.WeekDay;
      time->mday  = sdatestructureget.Date;
      time->month = sdatestructureget.Month;
      time->year  = (uint16_t)sdatestructureget.Year + 2000U;
      time->yday  = (uint8_t)timedate_get_yday(&sdatestructureget);
    }
    ret = timedate_status;
  }
  else
  {
    ret = TIMEDATE_STATUS_FAIL;
  }

  return ret;
}

#if (!USE_DEFAULT_SETUP == 1)

void timedate_setup_handler(void)
{
  static uint8_t timedate_input_sring[TIMEDATE_STRING_SIZE];
  PRINT_SETUP("\n\rDate and Time setup\n\r")

  menu_utils_get_string((uint8_t *)"Date GMT <Day, dd Month yyyy hh:mm:ss> (ex: Mon, 11 Dec 2017 17:22:05) ",
                        timedate_input_sring, TIMEDATE_STRING_SIZE);
  (void)timedate_set_from_http((uint8_t *)timedate_input_sring);
}

/**
  * @brief  date and date dump
  * @param  none
  * @retval none
  */


void timedate_setup_dump(void)
{
  static uint8_t *timedate_day_of_week_sring[8] =
  {
    (uint8_t *)"",
    (uint8_t *)"Mon",
    (uint8_t *)"Tue",
    (uint8_t *)"Wed",
    (uint8_t *)"Thu",
    (uint8_t *)"Fri",
    (uint8_t *)"Sat",
    (uint8_t *)"Sun",
  };

  timedate_t  timedate_data;
  (void)timedate_get(&timedate_data, TIMEDATE_DATE_AND_TIME);

  PRINT_SETUP("Date: %s %02d/%02d/%d - %02d:%02d:%02d\n\r",
              timedate_day_of_week_sring[timedate_data.wday],
              timedate_data.mday,
              timedate_data.month,
              timedate_data.year,
              timedate_data.hour,
              timedate_data.min,
              timedate_data.sec)

}
#endif /* (!USE_DEFAULT_SETUP == 1) */

#endif /* (USE_RTC == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
