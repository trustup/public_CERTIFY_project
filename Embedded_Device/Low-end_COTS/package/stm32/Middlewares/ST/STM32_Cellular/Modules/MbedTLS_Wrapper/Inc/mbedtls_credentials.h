/**
  ******************************************************************************
  * @file    mbedtls_credentials.h
  * @author  MCD Application Team
  * @brief   mbedtls credentials management
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#ifndef MBEDTLS_CREDENTIALS_H
#define MBEDTLS_CREDENTIALS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define USER_CONF_C2C_SOAPC_MAX_LENGTH  16
#define USER_CONF_C2C_USERID_MAX_LENGTH 16
#define USER_CONF_C2C_PSW_MAX_LENGTH    16

#define USER_CONF_WIFI_SSID_MAX_LENGTH  32
#define USER_CONF_WIFI_PSK_MAX_LENGTH   64

#define USER_CONF_DEVICE_NAME_LENGTH    300   /**< Must be large enough to hold a complete configuration string */
#define USER_CONF_SERVER_NAME_LENGTH    128
#define USER_CONF_TLS_OBJECT_MAX_SIZE   2048
#define USER_CONF_MAGIC                 0x0123456789ABCDEFuLL

/*  Firmware Version Max. Len */
#define IOT_STATE_FW_VERSION_MAX_SIZE   64


/** Static user configuration data which must survive reboot and firmware update.
  * Do not change the field order, due to firewall constraint
  * the tls_device_key size must be placed at a 64 bit boundary.
  * Its size must also be multiple of 64 bits.
  *
  * Depending on the available board peripherals, the c2c_config and wifi_config fields may not be used.
  */
typedef struct
{
  char tls_root_ca_cert[USER_CONF_TLS_OBJECT_MAX_SIZE * 3]; /* Allow room for 3 root CA certificates */
  char tls_device_cert[USER_CONF_TLS_OBJECT_MAX_SIZE];
  char tls_device_key[USER_CONF_TLS_OBJECT_MAX_SIZE];
  uint64_t ca_tls_magic;        /**< The USER_CONF_MAGIC magic word signals that the TLS root CA certificates strings
                                    (tls_root_ca_cert) are present in Flash. */
  uint64_t device_tls_magic;    /**< The USER_CONF_MAGIC magic word signals that the TLS device certificate and key
                                    (tls_device_cert and tls_device_key) are present in Flash. */
} mbedtls_credentials_config_t;

void mbedtls_credentials_init(void);
mbedtls_credentials_config_t *mbedtls_credentials_get_config(void);


#ifdef __cplusplus
}
#endif
#endif /* MBEDTLS_CREDENTIALS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

