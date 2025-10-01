/**
  ******************************************************************************
  * @file    dc_generic.h
  * @author  MCD Application Team
  * @brief   Header for dc_generic.c module
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

#ifndef DC_GENERIC_H
#define DC_GENERIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_DC_GENERIC == 1)

#include <math.h>
#include "dc_common.h"

/* Exported macros -----------------------------------------------------------*/
#define DC_BYTE_TABLE_SIZE 80U
/* Exported types ------------------------------------------------------------*/

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  uint8_t               value;
} dc_generic_bool_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  uint8_t               value;
} dc_generic_uint8_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  uint32_t               value;
} dc_generic_uint32_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  float_t                value;
} dc_generic_float_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  uint8_t                table[DC_BYTE_TABLE_SIZE];
} dc_generic_byte_table_t;



/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern dc_com_res_id_t  DC_GENERIC_BOOL_1      ;
extern dc_com_res_id_t  DC_GENERIC_BOOL_2      ;
extern dc_com_res_id_t  DC_GENERIC_BOOL_3      ;
extern dc_com_res_id_t  DC_GENERIC_UINT8_1     ;
extern dc_com_res_id_t  DC_GENERIC_UINT8_2     ;
extern dc_com_res_id_t  DC_GENERIC_UINT32_1    ;
extern dc_com_res_id_t  DC_GENERIC_UINT32_2    ;
extern dc_com_res_id_t  DC_GENERIC_FLOAT_1     ;
extern dc_com_res_id_t  DC_GENERIC_FLOAT_2     ;
extern dc_com_res_id_t  DC_GENERIC_BYTE_TABLE  ;
extern void dc_gen_uartTransmit(uint8_t *ptr, uint16_t len);

extern uint8_t *dc_generic_UartBusyFlag;

/* Exported functions ------------------------------------------------------- */
void dc_generic_start(void);
void dc_generic_init(void);

#ifdef __cplusplus
}
#endif

#endif /* (USE_DC_GENERIC == 1) */

#endif /* __DC_GENERIC_H */

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/
