/**
  ******************************************************************************
  * @file    board_buttons.c
  * @author  MCD Application Team
  * @brief   Implements functions for user buttons actions
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
#include "board_buttons.h"

#if (USE_BUTTONS == 1)
#include "rtosal.h"
#include "dc_common.h"
#include "stm32l4xx_hal.h" /* HAL_GetTick() */
/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define BOARD_BUTTONS_MSG_QUEUE_SIZE    (2U * BUTTONS_NB) /* msg queue size max:
                                                            (one press, one release)event per buttons */
#define BOARD_BUTTONS_MAX_NB             6U /* USER, UP, DOWN, LEFT, RIGHT, SEL */
/* Private macros ------------------------------------------------------------*/
/* Description
board_buttons_msg_t
{
  board_buttons_type_t type;        uint8_t
  bool                 press;       uint8_t
  bool                 press_type;  uint8_t
} */
/* Set / Get Socket message */
#define BOARD_BUTTONS_SET_TYPE(msg, type)               ((msg) = ((msg)&0xFFFFFF00U) | ((uint8_t)(type)))
#define BOARD_BUTTONS_SET_PRESS(msg, press)             ((msg) = ((msg)&0xFF00FFFFU) | (((uint8_t)(press))<<16U))
#define BOARD_BUTTONS_SET_PRESS_TYPE(msg, press_type)   ((msg) = ((msg)&0x00FFFFFFU) | (((uint8_t)(press_type))<<24U))
#define BOARD_BUTTONS_GET_TYPE(msg)                     ((board_buttons_type_t)((msg)&0x000000FFU))
#define BOARD_BUTTONS_GET_PRESS(msg)                    (((((msg)&0x00FF0000U)>>16U) == 1U) ? true : false)
#define BOARD_BUTTONS_GET_PRESS_TYPE(msg)               (((((msg)&0xFF000000U)>>24U) == 1U) ? true : false)

#define BOARD_BUTTONS_DELTA_TICK(start, end)            (((start) < (end)) ? ((end) - (start)) : \
                                                         ((0xFFFFFFFFU - (start)) + (end)))
#define BOARD_BUTTONS_SHORT_PRESS_TIMEOUT_MS            500U /* If button press > 500ms then it is 
                                                                a Long Press and not a Short press */

/* Functions Definition ------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static bool board_buttons_press_status[BOARD_BUTTONS_MAX_NB]; /* button status :
                                                                 false: button not pressed - wait press
                                                                 true: button pressed      - wait release */
static uint32_t board_buttons_press_time[BOARD_BUTTONS_MAX_NB]; /* Save tick counters when button has been pressed */
static dc_board_buttons_t board_buttons_dc_data[BOARD_BUTTONS_MAX_NB]; /* datacache data - one per buttons to manage */
static osMessageQId       board_buttons_msg_queue;

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t    DC_BOARD_BUTTONS_USER;  /* Datacache id for Button User      */
dc_com_res_id_t    DC_BOARD_BUTTONS_UP;    /* Datacache id for Button Up        */
dc_com_res_id_t    DC_BOARD_BUTTONS_DOWN;  /* Datacache id for Button Down      */
dc_com_res_id_t    DC_BOARD_BUTTONS_LEFT;  /* Datacache id for Button Left      */
dc_com_res_id_t    DC_BOARD_BUTTONS_RIGHT; /* Datacache id for Button Right     */
dc_com_res_id_t    DC_BOARD_BUTTONS_SEL;   /* Datacache id for Button Selection */

/* Private function prototypes -----------------------------------------------*/
static void board_buttons_thread(void *p_argument);
static board_buttons_type_t board_buttons_conversion(uint8_t button);
static void board_buttons_dc_register(board_buttons_type_t board_buttons);

/* Private Functions Definition ----------------------------------------------*/
static board_buttons_type_t board_buttons_conversion(uint8_t button)
{
  board_buttons_type_t result = BOARD_BUTTONS_MAX_TYPE;

  switch (button)
  {
    case BUTTON_USER:
      result = BOARD_BUTTONS_USER_TYPE;
      break;
    /* Change in below line result value if board support other buttons */
    case BOARD_BUTTONS_UP_TYPE:
    case BOARD_BUTTONS_DOWN_TYPE:
    case BOARD_BUTTONS_LEFT_TYPE:
    case BOARD_BUTTONS_RIGHT_TYPE:
    case BOARD_BUTTONS_SEL_TYPE:
    default:
      /* Nothing to do : result already set to BOARD_BUTTONS_MAX_TYPE / DC_COM_INVALID_ENTRY */
      __NOP();
      break;
  }
  return (result);
}

static void board_buttons_dc_register(board_buttons_type_t board_buttons)
{
  dc_com_res_id_t dc_id_res;

  switch (board_buttons)
  {
    case BOARD_BUTTONS_USER_TYPE:
      DC_BOARD_BUTTONS_USER = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_dc_data[board_buttons],
                                                   (uint16_t)sizeof(dc_board_buttons_t));
      dc_id_res = DC_BOARD_BUTTONS_USER;
      break;
    case BOARD_BUTTONS_UP_TYPE:
      dc_id_res = DC_BOARD_BUTTONS_UP;
      break;
    case BOARD_BUTTONS_DOWN_TYPE:
      dc_id_res = DC_BOARD_BUTTONS_DOWN;
      break;
    case BOARD_BUTTONS_LEFT_TYPE:
      dc_id_res = DC_BOARD_BUTTONS_LEFT;
      break;
    case BOARD_BUTTONS_RIGHT_TYPE:
      dc_id_res = DC_BOARD_BUTTONS_RIGHT;
      break;
    case BOARD_BUTTONS_SEL_TYPE:
      dc_id_res = DC_BOARD_BUTTONS_SEL;
      break;
    default:
      dc_id_res = DC_COM_INVALID_ENTRY;
      break;
  }
  if (dc_id_res != DC_COM_INVALID_ENTRY)
  {
    /* Button[board_buttons] is now active */
    board_buttons_dc_data[board_buttons].rt_state = DC_SERVICE_ON;
    board_buttons_dc_data[board_buttons].press_type = false;
  }
}

/* Functions Definition ------------------------------------------------------*/
/* Board buttons interface must be independent of the BSP interface so use of uint8_t and not BSP TypeDef */
void board_buttons_pressed(uint8_t button)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_type_t button_type;
  uint32_t tick_pressed = HAL_GetTick();
  button_type = board_buttons_conversion(button);

  /* Filter multiple press event - if possible */
  if ((button_type < BOARD_BUTTONS_MAX_TYPE) && (board_buttons_press_status[button_type] == false))
  {
    uint32_t msg_queue = 0U;

    /* Format msg_queue to send */
    /* Set Button type value */
    BOARD_BUTTONS_SET_TYPE(msg_queue, button_type);
    /* Set Press/Release indication */
    BOARD_BUTTONS_SET_PRESS(msg_queue, true); /* Button is pressed */
    /* Set Press type indication : not significant */
    /* BOARD_BUTTONS_SET_PRESS_TYPE(msg_queue, false); */ /* nothing to do: msg_queue already set to 0 */

    /* Send the msg_queue */
    if (rtosalMessageQueuePut(board_buttons_msg_queue, (uint32_t)msg_queue, 0U) == osOK)
    {
      board_buttons_press_time[button_type]   = tick_pressed;
      board_buttons_press_status[button_type] = true;
    }
  }
}

void board_buttons_released(uint8_t button)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_type_t button_type;
  uint32_t tick_released = HAL_GetTick();
  button_type = board_buttons_conversion(button);

  /* Filter multiple press event - if possible */
  if ((button_type < BOARD_BUTTONS_MAX_TYPE) && (board_buttons_press_status[button_type] == true))
  {
    uint32_t msg_queue = 0U;

    /* Format msg_queue to send */
    /* Is it a Short Press or a Long Press ? */
    if (BOARD_BUTTONS_DELTA_TICK((board_buttons_press_time[button_type]), (tick_released))
        > BOARD_BUTTONS_SHORT_PRESS_TIMEOUT_MS)
    {
      /* Long key press */
      BOARD_BUTTONS_SET_PRESS_TYPE(msg_queue, true); /* Long Key Press */
    }
    /* else Short Key Press: nothing to do msg_queue already set to 0 */
    /* BOARD_BUTTONS_SET_PRESS_TYPE(msg_queue, false); */ /* Short Key Press */

    /* Button is released */
    /* BOARD_BUTTONS_SET_PRESS(msg_queue, false); */ /* nothing to do: msg_queue already set to 0 */

    /* Set Button type value */
    BOARD_BUTTONS_SET_TYPE(msg_queue, button_type);

    /* Send the msg_queue */
    if (rtosalMessageQueuePut(board_buttons_msg_queue, (uint32_t)msg_queue, 0U) == osOK)
    {
      /* Do not release the button yet, board buttons thread must treat the press button indicaion
         before to allow a new press buttons */
      /* board_buttons_status[button] = false; */
      __NOP();
    }
    else
    {
      /* Release has not been put in the queue, release the status of the button to not block the button */
      board_buttons_press_status[button_type] = false;
    }
  }
}

/**
  * @brief  board buttons thread
  * @param  p_argument (UNUSED)
  * @retval -
  */
static void board_buttons_thread(void *p_argument)
{
  UNUSED(p_argument);
  board_buttons_type_t button_type;
  bool press;
  bool press_type;
  dc_com_res_id_t dc_id;
  uint32_t msg_queue;

  for (;;)
  {
    msg_queue = 0xFFFFFFFFU; /* Impossible value to receive */
    (void)rtosalMessageQueueGet(board_buttons_msg_queue, &msg_queue, RTOSAL_WAIT_FOREVER);
    if (msg_queue != 0xFFFFFFFFU)
    {
      /* Retrieve data */
      button_type = BOARD_BUTTONS_GET_TYPE(msg_queue);
      press = BOARD_BUTTONS_GET_PRESS(msg_queue);
      press_type = BOARD_BUTTONS_GET_PRESS_TYPE(msg_queue);

      switch (button_type)
      {
        case BOARD_BUTTONS_USER_TYPE:
          dc_id = DC_BOARD_BUTTONS_USER;
          break;
        case BOARD_BUTTONS_UP_TYPE:
          dc_id = DC_BOARD_BUTTONS_UP;
          break;
        case BOARD_BUTTONS_DOWN_TYPE:
          dc_id = DC_BOARD_BUTTONS_DOWN;
          break;
        case BOARD_BUTTONS_LEFT_TYPE:
          dc_id = DC_BOARD_BUTTONS_LEFT;
          break;
        case BOARD_BUTTONS_RIGHT_TYPE:
          dc_id = DC_BOARD_BUTTONS_RIGHT;
          break;
        case BOARD_BUTTONS_SEL_TYPE:
          dc_id = DC_BOARD_BUTTONS_SEL;
          break;
        default:
          dc_id = DC_COM_INVALID_ENTRY;
          break;
      }

      if ((dc_id != DC_COM_INVALID_ENTRY) && (button_type < BOARD_BUTTONS_MAX_NB))
      {
        if (press == true) /* Button is pressed */
        {
          /* Nothing to do */
        }
        else /* Button is released */
        {
          dc_board_buttons_t board_buttons_data;
          /* Update datacache structure */
          board_buttons_data.rt_state = DC_SERVICE_ON;
          board_buttons_data.press_type = press_type;
          (void)dc_com_write(&dc_com_db, dc_id, (void *)&board_buttons_data, sizeof(dc_board_buttons_t));
          /* New press button is possible */
          board_buttons_press_status[button_type] = false;
        }
      }
    }
  }
}
/**
  * @brief  Component initialisation
  * @param  -
  * @retval true/false - init OK/NOK
  */
bool board_buttons_init(void)
{
  bool result = true;

  /* Initialize all Buttons to INVALID */
  /* Data Cache entries Button User set to INVALID */
  DC_BOARD_BUTTONS_USER = DC_COM_INVALID_ENTRY;
  /* Data Cache entries Button Up set to INVALID */
  DC_BOARD_BUTTONS_UP = DC_COM_INVALID_ENTRY;
  /* Data Cache entries Button Down set to INVALID */
  DC_BOARD_BUTTONS_DOWN = DC_COM_INVALID_ENTRY;
  /* Data Cache entries Button Left set to INVALID */
  DC_BOARD_BUTTONS_LEFT = DC_COM_INVALID_ENTRY;
  /* Data Cache entries Button Right set to INVALID */
  DC_BOARD_BUTTONS_RIGHT = DC_COM_INVALID_ENTRY;
  /* Data Cache entries Button Selection set to INVALID */
  DC_BOARD_BUTTONS_SEL = DC_COM_INVALID_ENTRY;
  for (uint8_t i = 0U; i < BOARD_BUTTONS_MAX_TYPE; i++)
  {
    /* Board buttons press status st to false */
    board_buttons_press_status[i] = false;
    board_buttons_dc_data[i].rt_state = DC_SERVICE_UNAVAIL;
    board_buttons_dc_data[i].press_type = false;
  }

  /* Queue creation */
  board_buttons_msg_queue = rtosalMessageQueueNew(NULL, BOARD_BUTTONS_MSG_QUEUE_SIZE);

  if (board_buttons_msg_queue == NULL)
  {
    result = false;
  }

  /* Now that all internal structures are initialized, initilazize Buttons BSP part */
  /* Initialize all Buttons managed by the target here */
#if (USER_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)USER_BUTTON, BUTTON_MODE_EXTI);
#endif /* USER_BUTTON != NO_BUTTON */
#if (UP_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)UP_BUTTON, BUTTON_MODE_EXTI);
#endif /* UP_BUTTON != NO_BUTTON */
#if (DOWN_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)DOWN_BUTTON, BUTTON_MODE_EXTI);
#endif /* DOWN_BUTTON != NO_BUTTON */
#if (RIGHT_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)RIGHT_BUTTON, BUTTON_MODE_EXTI);
#endif /* RIGHT_BUTTON != NO_BUTTON */
#if (LEFT_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)LEFT_USER, BUTTON_MODE_EXTI);
#endif /* LEFT_BUTTON != NO_BUTTON */
#if (SEL_BUTTON != NO_BUTTON)
  BSP_PB_Init((Button_TypeDef)SEL_USER, BUTTON_MODE_EXTI);
#endif /* SEL_BUTTON != NO_BUTTON */

  return (result);
}


/**
  * @brief  Component start
  * @param  -
  * @retval true/false start OK/NOK
  */
bool board_buttons_start(void)
{
  bool result = true;
  static osThreadId boardButtonsTaskHandle;

  /* Service registration to datacache */
  /* Initialize Control Button USER Data Cache entries */
  for (board_buttons_type_t i = BOARD_BUTTONS_USER_TYPE; i < BOARD_BUTTONS_MAX_TYPE; i++)
  {
    board_buttons_dc_register(i);
  }

  /* Task creation */
  boardButtonsTaskHandle = rtosalThreadNew((const rtosal_char_t *)"BoardButtonsThread",
                                           (os_pthread)board_buttons_thread,
                                           BOARD_BUTTONS_THREAD_PRIO, USED_BOARD_BUTTONS_THREAD_STACK_SIZE, NULL);

  if (boardButtonsTaskHandle == NULL)
  {
    result = false;
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(boardButtonsTaskHandle, USED_BOARD_BUTTONS_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }

  return (result);
}

#endif /* USE_BUTTONS == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
