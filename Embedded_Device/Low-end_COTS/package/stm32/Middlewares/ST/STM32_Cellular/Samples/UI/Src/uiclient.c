/**
  ******************************************************************************
  * @file    uiclient.c
  * @author  MCD Application Team
  * @brief   UI Client Sample
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
#include "uiclient.h"

#if (USE_UI_CLIENT == 1)

/* FreeRTOS include */
#include "rtosal.h"

/* Error management */
#include "error_handler.h"

/* Datacache includes */
#include "dc_common.h"
#include "cellular_datacache.h"
#include "cellular_service_task.h"  /* CST_get_state() */

#if (USE_BUTTONS == 1)
#include "board_buttons.h"
#endif /* USE_BUTTONS == 1 */
#if (USE_DISPLAY == 1)
#include "board_display.h"
#include "bitmap.h"          /* Welcome bitmap */
#endif /* USE_DISPLAY == 1 */
#if (USE_LEDS == 1)
#include "board_leds.h"
#endif /* USE_LEDS == 1 */

#if (USE_RTC == 1)
#include "time_date.h"
#endif /* (USE_RTC == 1) */

#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
#include "dc_mems.h"
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */

#if ((USE_COM_ICC == 1) && (USE_ST33 == 1))
#include "com_icc.h"
#endif /* (USE_COM_ICC == 1) && (USE_ST33 == 1) */

/* Private defines -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Message exchange between callback and UIClt */
typedef uint8_t uiclient_msg_type_t;
#define UICLIENT_DATACACHE_MSG            (uiclient_msg_type_t)1    /* MSG is Datacache type */
#define UICLIENT_TIMER_MSG                (uiclient_msg_type_t)2    /* MSG is Timer     type */

typedef uint8_t uiclient_msg_id_t;
/* MSG id for Command   type */
/* MSG id for Datacache type */
#define UICLIENT_CELLULAR_INFO_DC         (uiclient_msg_id_t)1      /* MSG id is Datacache Celular info       */
#define UICLIENT_CELLULAR_DATA_INFO_DC    (uiclient_msg_id_t)2      /* MSG id is Datacache Cellular data info */
#define UICLIENT_MEMS_INFO_DC             (uiclient_msg_id_t)3      /* MSG id is Datacache Mems info          */
#if (USE_BUTTONS == 1)
#define UICLIENT_BUTTONS_INFO_DC          (uiclient_msg_id_t)4      /* MSG id is Datacache Buttons info       */
#endif /* USE_BUTTONS == 1 */
#if (USE_LEDS == 1)
/* MSG id for Timer     type */
#define UICLIENT_LED_TIMER                (uiclient_msg_id_t)1      /* MSG id is Timer     Led                */
#endif /* USE_LEDS == 1 */

typedef uint32_t uiclient_msg_t;

typedef uint8_t uiclient_screen_t;
#define UICLIENT_OFF_SCREEN            (uiclient_screen_t)0
#define UICLIENT_WELCOME_SCREEN        (uiclient_screen_t)1
#define UICLIENT_CELLULAR_INFO         (uiclient_screen_t)2
#define UICLIENT_CELLULAR_DATA_INFO    (uiclient_screen_t)3
#define UICLIENT_MEMS_INFO             (uiclient_screen_t)4

#define UICLIENT_NEXT_LINE             12U

#if (USE_LEDS == 1)
#define UICLIENT_LED_TIMER_MS          2000U /* in ms */
#endif /* USE_LEDS == 1 */

#if (USE_DISPLAY == 1)
#define UICLIENT_TEXT_TO_DISPLAY_MAX_LENGTH 40U
#endif /* USE_DISPLAY == 1 */

/* Private macros ------------------------------------------------------------*/
/* Use Cellular trace system */
#if (USE_TRACE_UI_CLIENT == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_UICLIENT, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_UICLIENT, DBL_LVL_P0, "UIClt: " format, ## args)
#define PRINT_DBG(format, args...) \
  TRACE_PRINT(DBG_CHAN_UICLIENT, DBL_LVL_P1, "UIClt: " format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...) (void)printf("" format "\n\r", ## args);
#define PRINT_INFO(format, args...)  (void)printf("UIClt: " format, ## args);
#define PRINT_DBG(format, args...)   (void)printf("UIClt: " format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */
#else /* USE_TRACE_UI_CLIENT == 0 */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_UICLIENT, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)   (void)printf("" format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_UI_CLIENT == 1U */

/* Description
uiclient_msg_t
{
  uiclient_msg_type_t type;
  uiclient_msg_id_t   id;
  uint16_t            data;
} */
/* Set / Get UIClt message */
#define SET_UICLIENT_MSG_TYPE(msg, type) ((msg) = ((((uint32_t)(msg)) & 0x00FFFFFFU) | ((type)<<24)))
#define SET_UICLIENT_MSG_ID(msg, id)     ((msg) = ((((uint32_t)(msg)) & 0xFF00FFFFU) | ((id)<<16)))
#define SET_UICLIENT_MSG_DATA(msg, data) ((msg) = ((((uint32_t)(msg)) & 0xFFFF0000U) | ((data))))
#define GET_UICLIENT_MSG_TYPE(msg)       ((uiclient_msg_type_t)((((uint32_t)(msg)) & 0xFF000000U)>>24))
#define GET_UICLIENT_MSG_ID(msg)         ((uiclient_msg_id_t)((((uint32_t)(msg)) & 0x00FF0000U)>>16))
#define GET_UICLIENT_MSG_DATA(msg)       ((uint16_t)(((uint32_t)(msg)) & 0xFFFF0000U))
#if (USE_BUTTONS == 1)
/* Data for BUTTONS */
#define SET_UICLIENT_DATA_BUTTON_ID(msg, button_id)   ((msg) = ((((uint32_t)(msg)) & 0xFFFF00FFU) | ((button_id)<<8)))
#define SET_UICLIENT_DATA_BUTTON_PRESS(msg, press)    ((msg) = ((((uint32_t)(msg)) & 0xFFFFFF00U) | \
                                                                (((press) == true) ? (1U) : (0U))))
#define GET_UICLIENT_DATA_BUTTON_ID(msg)              ((board_buttons_type_t)((((uint32_t)(msg)) & 0x0000FF00U)>>8))
#define GET_UICLIENT_DATA_BUTTON_PRESS(msg)           (((((uint32_t)(msg)) & 0x000000FFU) == 1U) ? true : false)
#endif /* USE_BUTTONS == 1 */
#define UICLIENT_MIN(a,b) (((a) < (b))  ? (a) : (b))

/* Private variables ---------------------------------------------------------*/
#if (USE_LEDS == 1)
static osTimerId uiclient_led_timer;    /* Timer used to blink Network led until Data is ready */
static bool uiclient_led_timer_running;
#endif /* USE_LEDS == 1 */
static osMessageQId uiclient_queue;     /* To communicate between datacache callback and UI client main thread */
#if (USE_DISPLAY == 1)
static uiclient_screen_t uiclient_screen_state;
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
static bool uiclient_modem_is_on;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
static uint8_t text_to_display [UICLIENT_TEXT_TO_DISPLAY_MAX_LENGTH];
static uint8_t tmp_string[20];
#endif /* USE_DISPLAY == 1 */

/* Example : */
/* static dc_cs_modem_state_t uiclient_modem_state; */
/* dc_event_id: DC_CELLULAR_INFO dc_cellular_info.modem_state
DC_MODEM_STATE_OFF           : modem not started
DC_MODEM_STATE_POWERED_ON    : modem powered on
DC_MODEM_STATE_SIM_CONNECTED : modem started with SIM connected
DC_MODEM_STATE_DATA_OK       : modem started with data available
*/
/* dc_event_id: DC_CELLULAR_NIFMAN_INFO dc_nifman_info.rt_state
DC_SERVICE_ON:            modem is attached
DC_SERVICE_SHUTTING_DOWN: modem shutdown in progress
*/

#if (USE_LEDS == 1)
static bool uiclient_dataready;
#endif /* USE_LEDS == 1 */

#if (USE_BUTTONS == 1)
#if ((USE_COM_ICC == 1) && (USE_ST33 == 1))
static int32_t uiclient_icc_st33_handle;
#endif /* (USE_COM_ICC == 1) && (USE_ST33 == 1) */
#endif /* USE_BUTTONS == 1 */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Callback used to treat datacache information */
static void uiclient_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);
#if (USE_LEDS == 1)
/* Callback called When Led timer is raised */
static void uiclient_led_timer_cb(void *p_argument);
#endif /* USE_LEDS == 1 */

/* Declare UI client thread */
static void uiclient_thread(void *p_argument);

#if (USE_DISPLAY == 1)
/* Format a line */
static void uiclient_format_line(uint8_t *p_string1, uint8_t length1, uint8_t *p_string2, uint8_t length2,
                                 uint8_t *p_string_res);
#endif /* USE_DISPLAY == 1 */

static bool uiclient_update_welcome(void);
/* Update Information */
static bool uiclient_update_cellular_data(bool display_init);
static bool uiclient_update_cellular_data_info(bool display_init);
static bool uiclient_update_mems_info(bool display_init);
static void uiclient_update_info(uiclient_screen_t info);

/* Update Led */
static void uiclient_update_led(void);

#if (USE_BUTTONS == 1)
/* Icc ST33 exchange */
static void uiclient_icc_st33_exchange(void);
#endif /* USE_BUTTONS == 1 */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Callback called when a value in datacache changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_private_gui_data - value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void uiclient_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  /* Event to know Modem state ? */
  if (dc_event_id == DC_CELLULAR_INFO)
  {
    SET_UICLIENT_MSG_TYPE(msg_queue, UICLIENT_DATACACHE_MSG);
    SET_UICLIENT_MSG_ID(msg_queue, UICLIENT_CELLULAR_INFO_DC);
  }
  else if (dc_event_id == DC_CELLULAR_DATA_INFO)
  {
    SET_UICLIENT_MSG_TYPE(msg_queue, UICLIENT_DATACACHE_MSG);
    SET_UICLIENT_MSG_ID(msg_queue, UICLIENT_CELLULAR_DATA_INFO_DC);
  }
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
  else if ((dc_event_id == DC_COM_PRESSURE) || (dc_event_id == DC_COM_HUMIDITY) || (dc_event_id == DC_COM_TEMPERATURE))
  {
    SET_UICLIENT_MSG_TYPE(msg_queue, UICLIENT_DATACACHE_MSG);
    SET_UICLIENT_MSG_ID(msg_queue, UICLIENT_MEMS_INFO_DC);
  }
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */
#if (USE_BUTTONS == 1)
  else if (dc_event_id == DC_BOARD_BUTTONS_USER)
  {
    dc_board_buttons_t board_buttons_data;
    /* Datacache must be read before a new push button could happen */
    (void)dc_com_read(&dc_com_db, DC_BOARD_BUTTONS_USER, (void *)&board_buttons_data, sizeof(dc_board_buttons_t));
    /* Update datacache structure */
    if (board_buttons_data.rt_state == DC_SERVICE_ON)
    {
      SET_UICLIENT_MSG_TYPE(msg_queue, UICLIENT_DATACACHE_MSG);
      SET_UICLIENT_MSG_ID(msg_queue, UICLIENT_BUTTONS_INFO_DC);
      SET_UICLIENT_DATA_BUTTON_ID(msg_queue, BOARD_BUTTONS_USER_TYPE);
      SET_UICLIENT_DATA_BUTTON_PRESS(msg_queue, (board_buttons_data.press_type));
    }
  }
#endif /* USE_BUTTONS == 1 */
  /* treat other dc_event_id */
  /*
  else if (dc_event_id == xxx)
  {
  }
  */
  else
  {
    __NOP();
  }
  if (msg_queue != 0U)
  {
    /* A message has to be send */
    status = rtosalMessageQueuePut(uiclient_queue, msg_queue, 0U);
    if (status != osOK)
    {
      PRINT_FORCE("UIClt: ERROR Datacache Msg Put Type:%d Id:%d - status:%d\n\r",
                  GET_UICLIENT_MSG_TYPE(msg_queue), GET_UICLIENT_MSG_ID(msg_queue), status)
    }
  }
}

#if (USE_LEDS == 1)
/**
  * @brief  Callback called when a value in datacache changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_argument - value provided at service subscription
  * @note   Unused parameter
  * @retval -
  */
static void uiclient_led_timer_cb(void *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue = 0U;
  rtosalStatus status;

  SET_UICLIENT_MSG_TYPE(msg_queue, UICLIENT_TIMER_MSG);
  SET_UICLIENT_MSG_ID(msg_queue, UICLIENT_LED_TIMER);
  status = rtosalMessageQueuePut(uiclient_queue, msg_queue, 0U);
  if (status != osOK)
  {
    PRINT_FORCE("UIClt: ERROR Led Msg Put - status: %d\n\r", status)
  }
}
#endif /* USE_LEDS == 1 */

#if (USE_DISPLAY == 1)
/**
  * @brief  Format a line taking into account number of characters available on a line
  *         Enough space will be added between string1 and string2 to have string2 end always at last possible position
  * @param  p_string1    - string to display at left
  * @param  length1      - string1 length
  * @param  p_string2    - string to display at right
  * @param  length2      - string2 length
  * @param  p_string_res - string result to display
  * @retval -
  */
static void uiclient_format_line(uint8_t *p_string1, uint8_t length1, uint8_t *p_string2, uint8_t length2,
                                 uint8_t *p_string_res)
{
  uint32_t nb_character = board_display_CharacterNbPerLine();
  uint8_t space[10];
  (void)memset(space, 0x20, 10U);
  (void)sprintf((CRC_CHAR_t *)p_string_res, "%.*s%.*s%.*s", (int16_t)length1, p_string1,
                (int16_t)(nb_character - (uint32_t)length1 - (uint32_t)length2), space, (int16_t)length2, p_string2);
}
#endif /* USE_DISPLAY == 1 */

/**
  * @brief  Update display and status according to welcome screen
  * @retval true/false - refresh has to be done or not
  */
static bool uiclient_update_welcome(void)
{
  bool result = false;
#if (USE_DISPLAY == 1)
  /* Welcome display only information */
  /* Display welcome screen ? */
  if (uiclient_screen_state == UICLIENT_OFF_SCREEN)
  {
    /* Display a welcome bitmap */
    uint32_t XPos = 0U;
    uint32_t YPos = 0U;
    /* Center welcome image as much as possible */
    if (WELCOME_BITMAP_HEIGHT < BSP_LCD_GetYSize())
    {
      YPos = (BSP_LCD_GetYSize() - WELCOME_BITMAP_HEIGHT) / 2U;
    }
    if (WELCOME_BITMAP_WIDTH < BSP_LCD_GetXSize())
    {
      XPos = (BSP_LCD_GetXSize() - WELCOME_BITMAP_WIDTH) / 2U;
    }
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    board_display_DrawRGBImage((uint16_t)XPos, (uint16_t)YPos, WELCOME_BITMAP_WIDTH, WELCOME_BITMAP_HEIGHT,
                               (uint8_t *)welcome_bitmap);
    /* Update screen state */
    uiclient_screen_state = UICLIENT_WELCOME_SCREEN;
  }
#endif /* USE_DISPLAY == 1 */
  return (result);
}

/**
  * @brief  Update display and status according to new cellular data received
  * @param  display_init - true/false display is initialized/not initialized
  * @retval true/false - refresh has to be done or not
  */
static bool uiclient_update_cellular_data(bool display_init)
{
  bool result;

#if (USE_DISPLAY == 1)
  uint32_t line = 0U;
  uint8_t *p_string1;
  uint8_t *p_string2;
  static uint8_t *sim_string[3]   = {((uint8_t *)"SIM:?"), \
                                     ((uint8_t *)"SIM  "), \
                                     ((uint8_t *)"eSIM ")
                                    };
  static uint8_t *state_string[7] = {((uint8_t *)"      Unknown"), \
                                     ((uint8_t *)" Initializing"), \
                                     ((uint8_t *)" NwkSearching"), \
                                     ((uint8_t *)"NwkAttachment"), \
                                     ((uint8_t *)"  NwkAttached"), \
                                     ((uint8_t *)"    DataReady"), \
                                     ((uint8_t *)"   FlightMode")
                                    };
  uint8_t *operator_string[1] = {((uint8_t *)"Operator:?")};

  /* Cellular data display only information */
  if (display_init == true)
  {
    /* Display welcome screen ? */
    if (uiclient_screen_state == UICLIENT_OFF_SCREEN)
    {
      /* Display a welcome bitmap */
      result = uiclient_update_welcome();
    }
    else
    {
      result = false;
      /* If comes from Welcome Screen - Erase full screen */
      if (uiclient_screen_state == UICLIENT_WELCOME_SCREEN)
      {
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        board_display_Clear(LCD_COLOR_BLACK);
        /* And go to Cellular Info Screen */
        uiclient_screen_state = UICLIENT_CELLULAR_INFO;
      }

      /* Update screen only if Cellular info screen */
      if (uiclient_screen_state == UICLIENT_CELLULAR_INFO)
      {
        uint32_t nb_character = board_display_CharacterNbPerLine();
        uint8_t space[10];
        dc_cellular_info_t dc_cellular_info;
        dc_sim_info_t      dc_sim_info;
        timedate_t time;

        /* Set font to default font */
        board_display_SetFont(0U);

        (void)memset(text_to_display, 0, UICLIENT_TEXT_TO_DISPLAY_MAX_LENGTH);
        (void)memset(space, 0x20, 10U);

#if (USE_RTC == 1)
        /* line: hh:mm month/day */
        /* Date and Time managed through Datacache */
        (void)timedate_get(&time, TIMEDATE_DATE_AND_TIME);

        (void)sprintf((CRC_CHAR_t *)text_to_display, "%02hhu:%02hhu %.*s %02hhu/%02hhu",
                      time.hour, time.min, (int16_t)(nb_character - 12U), space, time.month, time.mday);

        board_display_DisplayStringAt(1U, (uint16_t)line, text_to_display, LEFT_MODE);
        line += board_display_GetFontHeight();
#endif /* USE_RTC == 1 */

        /* Obtain all data from datacache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&dc_sim_info, sizeof(dc_sim_info));

        /* line:[eSIM|SIM |    ] */
        if (dc_sim_info.rt_state == DC_SERVICE_ON)
        {
          if (dc_sim_info.active_slot == DC_SIM_SLOT_MODEM_EMBEDDED_SIM)
          {
            p_string1 = sim_string[2];
          }
          else if (dc_sim_info.active_slot == DC_SIM_SLOT_MODEM_SOCKET)
          {
            p_string1 = sim_string[1];
          }
          else
          {
            p_string1 = sim_string[0];
          }
        }
        else
        {
          p_string1 = sim_string[0];
        }

        /* line: Stat:$status */
        uint16_t cst_state = CST_get_state();

        switch (cst_state)
        {
          case CST_BOOT_STATE:
          case CST_MODEM_INIT_STATE:
            p_string2 = state_string[1];
            break;
          case CST_MODEM_READY_STATE:
          case CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE:
            p_string2 = state_string[2];
            break;
          case CST_WAITING_FOR_NETWORK_STATUS_STATE:
          case CST_NETWORK_STATUS_OK_STATE:
            p_string2 = state_string[3];
            break;
          case CST_MODEM_REGISTERED_STATE:
          case CST_MODEM_PDN_ACTIVATING_STATE:
            p_string2 = state_string[4];
            break;
          case CST_MODEM_DATA_READY_STATE:
            p_string2 = state_string[5];
            break;
          case CST_MODEM_SIM_ONLY_STATE:
            p_string2 = state_string[5];
            break;
          default:
            p_string2 = state_string[0];
            PRINT_INFO("!!!Platform state Unknown: CST state:%d!!!\n\r", cst_state)
            break;
        }

        uiclient_format_line(p_string1, 5U, p_string2, 13U, (uint8_t *)&text_to_display);
        board_display_DisplayStringAt(1U, (uint16_t)line, text_to_display, LEFT_MODE);
        line += board_display_GetFontHeight();

        /* line: $mno_name $cs_signal_level_db(dB) */
        if ((dc_cellular_info.rt_state == DC_SERVICE_RUN) || (dc_cellular_info.rt_state == DC_SERVICE_ON))
        {
          uint8_t operator_name_start;
          uint32_t operator_name_length;

          operator_name_length = crs_strlen((const uint8_t *)(&dc_cellular_info.mno_name[0]));
          if (dc_cellular_info.mno_name[0] == (uint8_t)'"')
          {
            operator_name_start = 1U;
            operator_name_length = operator_name_length - 2U;
          }
          else
          {
            operator_name_start = 0U;
          }
          if (operator_name_length > 0U)
          {
            /* Copy operator name */
            p_string1 = &dc_cellular_info.mno_name[operator_name_start];
          }
          else
          {
            /* Copy operator name unknown*/
            p_string1 = operator_string[0];
            operator_name_length = 10U;
          }
          /* Add Signal level */
          (void)sprintf((CRC_CHAR_t *)&tmp_string[0], "%3lddB", dc_cellular_info.cs_signal_level_db);
          p_string2 = &tmp_string[0];
          uiclient_format_line(p_string1, (uint8_t)UICLIENT_MIN(operator_name_length, (nb_character - 5U - 1U)),
                               p_string2, 5U, (uint8_t *)&text_to_display);
        }
        else
        {
          /* Copy operator name unknown*/
          p_string1 = operator_string[0];
          /* Add Signal level */
          (void)sprintf((CRC_CHAR_t *)&tmp_string[0], "%3lddB", dc_cellular_info.cs_signal_level_db);
          p_string2 = &tmp_string[0];
          uiclient_format_line(p_string1, 10U, p_string2, 5U, (uint8_t *)&text_to_display);
        }

        board_display_DisplayStringAt(1U, (uint16_t)line, text_to_display, LEFT_MODE);
        /* Finalize the cellular info screen with cellular data info */
        /* line += board_display_GetFontHeight(); */
        (void)uiclient_update_cellular_data_info(display_init);
        /* Finalize the cellular info screen with mems info */
        /* line += board_display_GetFontHeight(); */
        (void)uiclient_update_mems_info(display_init);
        /* line += board_display_GetFontHeight(); */

        result = true;
      }
    }
  }
  else
  {
    result = false;
  }
  /* else nothing to do */
#else  /* USE_DISPLAY == 0 */
  UNUSED(display_init);
  result = false;
#endif /* USE_DISPLAY == 1 */

  return (result);
}

/**
  * @brief  Update display and status according to new cellular data info received
  * @param  display_init - true/false display is initialized/not initialized
  * @retval true/false - refresh has to be done or not
  */
static bool uiclient_update_cellular_data_info(bool display_init)
{
  bool result = false;
  dc_cellular_data_info_t dc_cellular_data_info;
#if (USE_DISPLAY == 1)
  uint16_t line;
  uint8_t *p_string1;
  uint8_t *p_string2;
  uint8_t *ip_string[2] = {((uint8_t *)"IP:?"), ((uint8_t *)"IP:")};
#endif /* USE_DISPLAY == 1 */

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&dc_cellular_data_info, sizeof(dc_cellular_data_info));

#if (USE_DISPLAY == 1)
  /* Update screen only if display is initialized and screen is Cellular info */
  if ((display_init == true) && (uiclient_screen_state == UICLIENT_CELLULAR_INFO))
  {
    /* Is IP known ? */
    if (dc_cellular_data_info.rt_state == DC_SERVICE_ON)
    {
      p_string1 = ip_string[1];
      (void)sprintf((CRC_CHAR_t *)&tmp_string[0], "%hhu.%hhu.%hhu.%.hhu",
                    (uint8_t)(COM_IP4_ADDR1(&dc_cellular_data_info.ip_addr)),
                    (uint8_t)(COM_IP4_ADDR2(&dc_cellular_data_info.ip_addr)),
                    (uint8_t)(COM_IP4_ADDR3(&dc_cellular_data_info.ip_addr)),
                    (uint8_t)(COM_IP4_ADDR4(&dc_cellular_data_info.ip_addr)));
      p_string2 = &tmp_string[0];
      uiclient_format_line(p_string1, 3U, p_string2, (uint8_t)crs_strlen((const uint8_t *)p_string2),
                           (uint8_t *)&text_to_display);
    }
    else
    {
      (void)memcpy(text_to_display, ip_string[0], 5);
    }
    /* line: Ip:$local_ip */
    line = 3U * (uint16_t)board_display_GetFontHeight();
    board_display_DisplayStringAt(1U, line, text_to_display, LEFT_MODE);

    result = true;
  }
#endif /* USE_DISPLAY == 1 */

#if (USE_LEDS == 1)
  /* Update data ready status whatever the display status and screen state */
  if (dc_cellular_data_info.rt_state == DC_SERVICE_ON)
  {
    uiclient_dataready = true;
  }
  else
  {
    uiclient_dataready = false;
  }
#endif /* USE_LEDS == 1 */

  return (result);
}

/**
  * @brief  Update display and status according to new mems info received
  * @param  display_init - true/false display is initialized/not initialized
  * @retval true/false - refresh has to be done or not
  */
static bool uiclient_update_mems_info(bool display_init)
{
  bool result = false;

#if (USE_DISPLAY == 1)
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
  uint16_t line;

  dc_pressure_info_t    pressure_info;
  dc_humidity_info_t    humidity_info;
  dc_temperature_info_t temperature_info;

  /* Update screen only if display is initialized and screen is Cellular info */
  if ((display_init == true) && (uiclient_screen_state == UICLIENT_CELLULAR_INFO))
  {
    (void)dc_com_read(&dc_com_db, DC_COM_HUMIDITY, (void *)&humidity_info, sizeof(humidity_info));
    (void)dc_com_read(&dc_com_db, DC_COM_TEMPERATURE, (void *)&temperature_info, sizeof(temperature_info));
    (void)dc_com_read(&dc_com_db, DC_COM_PRESSURE, (void *)&pressure_info, sizeof(pressure_info));

    (void)sprintf((CRC_CHAR_t *)text_to_display, "T:%4.1fC H:%4.1f P:%6.1fP",
                  temperature_info.temperature, humidity_info.humidity, pressure_info.pressure);
    line = 4U * (uint16_t)board_display_GetFontHeight();
    /* Too many information to display on same line: reduce Font */
    board_display_DecreaseFont();
    board_display_DisplayStringAt(1U, line, text_to_display, LEFT_MODE);
    /* Restore font to default font */
    board_display_SetFont(0U);

    result = true;
  }
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */
#endif /* USE_DISPLAY == 1 */

  return (result);
}

/**
  * @brief  Update information according to new one received
  * @param  info - information received
  * @retval -
  */
static void uiclient_update_info(uiclient_screen_t info)
{
  bool display_init_ok = false;
  bool refresh_to_do;

  /* Board display init to be done only if modem is on ? */
#if (USE_DISPLAY == 1)
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
  if (uiclient_modem_is_on == true)
  {
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
    display_init_ok = board_display_Init();
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
  }
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
#endif /* USE_DISPLAY == 1 */

  switch (info)
  {
#if (DISPLAY_WAIT_MODEM_IS_ON == 0U)
    case UICLIENT_WELCOME_SCREEN :
      if (display_init_ok == true)
      {
        refresh_to_do = uiclient_update_welcome();
      }
      else
      {
        refresh_to_do = false;
      }
      break;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 0U */
    case UICLIENT_CELLULAR_INFO :
      refresh_to_do = uiclient_update_cellular_data(display_init_ok);
      break;
    case UICLIENT_CELLULAR_DATA_INFO :
      refresh_to_do = uiclient_update_cellular_data_info(display_init_ok);
      break;
    case UICLIENT_MEMS_INFO :
      refresh_to_do = uiclient_update_mems_info(display_init_ok);
      break;
    default:
      refresh_to_do = false;
      break;
  }

  if (refresh_to_do == true)
  {
#if (USE_DISPLAY == 1)
    /* Refresh Display */
    board_display_Refresh();
#endif /* USE_DISPLAY == 1 */
  }
}

/**
  * @brief  Update led information
  * @retval -
  */
static void uiclient_update_led(void)
{
#if (USE_LEDS == 1)
  /* Data ready state ? */
  if (uiclient_dataready == false)
  {
    /* Data not ready */
    /* Blink quickly the led to show data is not ready */
    board_leds_on(DATAREADY_LED);
    (void)rtosalDelay(300U);
    board_leds_off(DATAREADY_LED);

    /* Restart led timer ? */
    if (uiclient_led_timer_running == false)
    {
      /* Data is lost, restart the timer */
      uiclient_led_timer_running = true;
      (void)rtosalTimerStart(uiclient_led_timer, UICLIENT_LED_TIMER_MS);
    }
  }
  else
  {
    /* Inform the first time data is now ready */
    if (uiclient_led_timer_running == true)
    {
      /* Stop the led timer */
      uiclient_led_timer_running = false;
      (void)rtosalTimerStop(uiclient_led_timer);

      /* Data is now ready */
      /* Blink led to show data is ready */
      for (uint8_t i = 0U; i < 3U; i++)
      {
        board_leds_on(DATAREADY_LED);
        (void)rtosalDelay(200U);
        board_leds_off(DATAREADY_LED);
        (void)rtosalDelay(200U);
      }
      /* Led off rather than led on for power consumption saving */

    }
  }
#endif /* USE_LEDS == 1 */
}

#if (USE_BUTTONS == 1)
/**
  * @brief  Example of Icc ST33 exchange
  * @retval -
  */
static void uiclient_icc_st33_exchange(void)
{
#if ((USE_COM_ICC == 1) && (USE_ST33 == 1))
#define UICLIENT_APDU_RES_MAX_LENGTH 5U /* in fact current implementation at low level is limited to 253 apdu bytes
                                               means 506 char bytes maximum */
  uint8_t apdu_test[34] = "00A404000BA000000095010000930001";
  uint8_t apdu_test_res[UICLIENT_APDU_RES_MAX_LENGTH] = "9000";
  static uint8_t apdu_rsp[UICLIENT_APDU_RES_MAX_LENGTH];
  uint8_t *apdu_cmd;               /* pointer on the apdu to send */
  uint8_t *apdu_rsp_expected;      /* pointer on the apdu response expected */
  int32_t apdu_cmd_len;            /* length  of the apdu to send */
  int32_t apdu_rsp_len;            /* length  of the buffer response */
  int32_t apdu_received_len;       /* length  of the apdu received */

  if (uiclient_icc_st33_handle == COM_HANDLE_INVALID_ID)
  {
    uiclient_icc_st33_handle = com_icc(COM_AF_UNSPEC, COM_SOCK_SEQPACKET, COM_PROTO_NDLC);
  }
  if (uiclient_icc_st33_handle != COM_HANDLE_INVALID_ID)
  {
    apdu_cmd = &apdu_test[0];
    apdu_rsp_expected = &apdu_test_res[0];

    apdu_cmd_len = (int32_t)crs_strlen(apdu_cmd);
    apdu_rsp_len = (int32_t)sizeof(apdu_rsp);

    apdu_received_len = com_icc_generic_access(uiclient_icc_st33_handle,
                                               (const com_char_t *)apdu_cmd, apdu_cmd_len,
                                               &apdu_rsp[0], apdu_rsp_len);
    if (apdu_received_len > 0)
    {
      int32_t apdu_rsp_expected_len = (int32_t)crs_strlen(apdu_rsp_expected);
      if ((apdu_received_len <= apdu_rsp_len)
          && (apdu_received_len == apdu_rsp_expected_len)
          && (memcmp(apdu_test_res, apdu_rsp, (uint32_t)apdu_received_len) == 0))
      {
        PRINT_INFO("eSE: Response Received OK: length:%ld string:%.*s\n\r", apdu_received_len,
                   (int16_t)apdu_received_len, apdu_rsp);
      }
      else
      {
        PRINT_INFO("eSE: Response Received NOK: length:%ld string:%.*s\n\r", apdu_received_len,
                   (int16_t)apdu_received_len, apdu_rsp);
      }
    }
    else
    {
      PRINT_INFO("eSE: Response Error: status:%ld\n\r", apdu_received_len);
    }
  }
  if (com_closeicc(uiclient_icc_st33_handle) == COM_ERR_OK)
  {
    uiclient_icc_st33_handle = COM_HANDLE_INVALID_ID;
    PRINT_INFO("Close ESE handle OK\n\r")
  }
#endif /* (USE_COM_ICC == 1) && (USE_ST33 == 1) */
}
#endif /* USE_BUTTONS == 1 */

/**
  * @brief  UIClient thread
  * @note   Infinite loop body
  * @param  p_argument - parameter osThread
  * @note   Unused parameter
  * @retval -
  */
static void uiclient_thread(void *p_argument)
{
  UNUSED(p_argument);
  uint32_t msg_queue; /* Msg received from the queue      */
  uint16_t msg_type;  /* Msg type received from the queue */
  uint16_t msg_id;    /* Msg id received from the queue   */

#if (USE_LEDS == 1)
  /* Led off DataReady */
  board_leds_off(DATAREADY_LED);

  /* Start Led timer */
  uiclient_led_timer_running = true;
  (void)rtosalTimerStart(uiclient_led_timer, UICLIENT_LED_TIMER_MS);
#endif /* USE_LEDS == 1 */

#if (USE_DISPLAY == 1)
#if (DISPLAY_WAIT_MODEM_IS_ON == 0U)
  uiclient_update_info(UICLIENT_WELCOME_SCREEN);
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 0U */
#endif /* USE_DISPLAY == 1 */

  for (;;)
  {
    msg_queue = 0U; /* Re-initialize msg_queue to impossible value */

    /* Wait a notification to do something */
    (void)rtosalMessageQueueGet(uiclient_queue, &msg_queue, RTOSAL_WAIT_FOREVER);
    /* Analyze message */
    if (msg_queue != 0U)
    {
      msg_type = GET_UICLIENT_MSG_TYPE(msg_queue);
      msg_id = GET_UICLIENT_MSG_ID(msg_queue);

      PRINT_DBG("UIClt: Msg received:%lx type:%d id:%d data:%d\n\r", msg_queue, msg_type, msg_id,
                GET_UICLIENT_MSG_DATA(msg_queue))
      switch (msg_type)
      {
        case UICLIENT_DATACACHE_MSG :
          if (msg_id == UICLIENT_CELLULAR_INFO_DC)
          {
            dc_cellular_info_t dc_cellular_info;

            (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
            if ((dc_cellular_info.rt_state == DC_SERVICE_RUN)
                && (dc_cellular_info.modem_state == DC_MODEM_STATE_POWERED_ON))
            {
#if (USE_DISPLAY == 1)
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
              uiclient_modem_is_on = true;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
#endif /* USE_DISPLAY == 1 */
              uiclient_update_info(UICLIENT_CELLULAR_INFO);
            }
            else if (dc_cellular_info.rt_state == DC_SERVICE_ON)
              /*     && (dc_cellular_info.modem_state == DC_MODEM_STATE_DATA_OK) */

            {
              uiclient_update_info(UICLIENT_CELLULAR_INFO);
            }
            else
            {
              /* Not treated */
              __NOP();
            }
          }
          else if (msg_id == UICLIENT_CELLULAR_DATA_INFO_DC)
          {
            uiclient_update_info(UICLIENT_CELLULAR_DATA_INFO);
            uiclient_update_led();
          }
          else if (msg_id == UICLIENT_MEMS_INFO_DC)
          {
            uiclient_update_info(UICLIENT_MEMS_INFO);
          }
#if (USE_BUTTONS == 1)
          else if (msg_id == UICLIENT_BUTTONS_INFO_DC)
          {
            PRINT_INFO("Buttons pressed:%d - long press:%d\r\n", GET_UICLIENT_DATA_BUTTON_ID(msg_queue),
                       GET_UICLIENT_DATA_BUTTON_PRESS(msg_queue))
            uiclient_icc_st33_exchange();
          }
#endif /* USE_BUTTONS == 1 */
          else /* Should not happen */
          {
            __NOP();
          }
          break;

        case UICLIENT_TIMER_MSG :
#if (USE_LEDS == 1)
          if (msg_id == UICLIENT_LED_TIMER)
          {
            uiclient_update_led();
          }
#else
          /* No other timer than LED one */
          __NOP();
#endif /* USE_LEDS == 1 */
          break;

        default :
          /* Should not happen */
          __NOP();
          break;
      }
    }
  }
}

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Initialization
  * @note   UIClient initialization - first function called
  * @param  -
  * @retval -
  */
void uiclient_init(void)
{
  /* UI Client static initialization */
#if (USE_LEDS == 1)
  uiclient_led_timer_running = false;
  uiclient_dataready = false;
#endif /* USE_LEDS == 1 */
#if (USE_DISPLAY == 1)
  uiclient_screen_state = UICLIENT_OFF_SCREEN;
#if (DISPLAY_WAIT_MODEM_IS_ON == 1U)
  uiclient_modem_is_on = false;
#endif /* DISPLAY_WAIT_MODEM_IS_ON == 1U */
#endif /* USE_DISPLAY == 1 */

#if (USE_BUTTONS == 1)
#if ((USE_COM_ICC == 1) && (USE_ST33 == 1))
  uiclient_icc_st33_handle = COM_HANDLE_INVALID_ID;
#endif /* (USE_COM_ICC == 1) && (USE_ST33 == 1) */
#endif /* USE_BUTTONS == 1 */

  /* UI Client queue creation */
#if ((USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1))
  uiclient_queue = rtosalMessageQueueNew(NULL, 20U);
#else
  uiclient_queue = rtosalMessageQueueNew(NULL, 10U);
#endif /* (USE_DC_MEMS == 1) || (USE_SIMU_MEMS == 1) */
  if (uiclient_queue == NULL)
  {
    ERROR_Handler(DBG_CHAN_UICLIENT, 1, ERROR_FATAL);
  }

#if (USE_LEDS == 1)
  /* UI Client led timer creation */
  uiclient_led_timer = rtosalTimerNew(NULL, (os_ptimer)uiclient_led_timer_cb, osTimerPeriodic, NULL);
  if (uiclient_led_timer == NULL)
  {
    ERROR_Handler(DBG_CHAN_UICLIENT, 2, ERROR_FATAL);
  }
#endif /* USE_LEDS == 1 */
}

/**
  * @brief  Start
  * @note   UIClient start - function called when platform is initialized
  * @param  -
  * @retval t-
  */
void uiclient_start(void)
{
  static osThreadId UIClient_TaskHandle;

  /* Cellular is now initialized - Registration to other components is now possible */

  /* Registration to datacache */
  (void)dc_com_register_gen_event_cb(&dc_com_db, uiclient_datacache_cb, (void *) NULL);

  /* Create UIClient thread  */
  UIClient_TaskHandle = rtosalThreadNew((const rtosal_char_t *)"UICltThread", (os_pthread)uiclient_thread,
                                        UICLIENT_THREAD_PRIO, USED_UICLIENT_THREAD_STACK_SIZE, NULL);
  if (UIClient_TaskHandle == NULL)
  {
    ERROR_Handler(DBG_CHAN_UICLIENT, 3, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(UIClient_TaskHandle, USED_UICLIENT_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }
}

#endif /* USE_UI_CLIENT == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
