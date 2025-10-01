/**
  ******************************************************************************
  * @file    mbedtls_entropy.c
  * @author  MCD Application Team
  * @brief   This file provides code for the entropy collector.
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

#include "plf_config.h"

#if (USE_MBEDTLS == 1)

#include "main.h"        /* Include the HAL interface */
#include "net_conf.h"    /* include network config */
#include "mbedtls_entropy.h"

/**
  * @brief  allow to get rng raw for mbedtls
  * @param  data        (IN)  - input data to generate random number
  * @param  output      (OUT) - output random data
  * @param  len         (IN)  - output data size
  * @retval return code
  */

int32_t mbedtls_rng_raw(void *data,
                        uint8_t *output, size_t len)
{
  HAL_StatusTypeDef status;
  uint32_t random_number = 0;
  int32_t ret;
  ret = 0;
  for (uint32_t i = 0U; i < ((len + sizeof(uint32_t) - 1U) / sizeof(uint32_t)); i++)
  {
    /* Data shall contain the pointer to the selected hrng instance */
    status = HAL_RNG_GenerateRandomNumber((RNG_HandleTypeDef *)data, &random_number);
    if (HAL_OK == status)
    {
      uint32_t jmax = (((uint32_t)len - (i * sizeof(uint32_t))) >= sizeof(uint32_t)) ?
                      sizeof(uint32_t) : (len % sizeof(uint32_t));
      for (uint32_t j = 0U; j < jmax; j++)
      {
        output[(i * sizeof(uint32_t)) + j] = ((uint8_t *) &random_number)[j];
      }
    }
    else
    {
      /* error */
      ret = -1;
      break;
    }
  }

  return ret;
}

#endif  /* (USE_MBEDTLS == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
