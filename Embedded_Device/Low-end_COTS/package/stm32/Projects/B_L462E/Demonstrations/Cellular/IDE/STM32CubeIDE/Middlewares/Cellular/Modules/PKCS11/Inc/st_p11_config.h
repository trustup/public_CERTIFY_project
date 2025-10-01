/**
  ******************************************************************************
  * @file    st_p11_config.h
  * @author  MCD Application Team
  * @brief   Header Configuration file
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

#ifndef ST_P11_CONFIG_H
#define ST_P11_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/


/* Maximum length for each object name */
#define MAX_LABEL_LENGTH                        20

/* Maximum certificates size */
#define MAX_CERTIFICATE_LEN                     2000

/* Maximum data object size */
#define MAX_DATAOBJECT_LEN                      150

/* Maximum data object size */
#define MAX_SECKEYRESPONSE_LEN                  12

/* Name of the private key object belonging to the main keypair */
#define MAIN_PRIVATE_KEY_IN_KEY_PAIR            "PrivK"

/* Name of the public key object belonging to the main keypair */
#define MAIN_PUBLIC_KEY_IN_KEY_PAIR             "PubK"

/* Name of the private key object belonging to the secondary keypair */
#define SECONDARY_PRIVATE_KEY_IN_KEY_PAIR       "PrivK2"

/* Name of the public key object belonging to the secondary keypair */
#define SECONDARY_PUBLIC_KEY_IN_KEY_PAIR        "PubK2"

/* Name of the private key object belonging to the third keypair */
#define THIRD_PRIVATE_KEY_IN_KEY_PAIR           "PrivK3"

/* Name of the main certificate object */
#define MAIN_CERTIFICATE                        "Identity Certificate"

/* Name of the second certificate object */
#define SECONDARY_CERTIFICATE                   "CertK2"

/* Name of the third certificate object */
#define THIRD_CERTIFICATE                       "CertK3"

/* Name of the fourth certificate object */
#define FOURTH_CERTIFICATE                       "CertK4"

/* Name of the main public key object */
#define MAIN_PUBLIC_KEY                         "ppubk_a"

/* Name of the secondary public key object */
#define SECONDARY_PUBLIC_KEY                    "ppubk_b"

/* Maximum length for certificate's subject */
#define MAX_SUBJECT_LENGTH                      256

/* Name of the main secret key object */
#define MAIN_SECRET_KEY                         "PSK"

/* Name of the first secret key derived from main secret key */
#define FIRST_DERIVED_KEY                       "MSK"

/* Name of the secret key derived from main secret key */
#define ENC_DEC_KEY                      		"EDK"

/* Name of the data object */
#define DATA_OBJECT 							"MUD file URL"

/* Name of the PMK_i */
#define PMK_1		 							"PMK_1"
#define PMK_2		 							"PMK_2"
#define PMK_3		 							"PMK_3"
#define PMK_4		 							"PMK_4"
#define PMK_5		 							"PMK_5"
#define PMK_6		 							"PMK_6"
#define PMK_7		 							"PMK_7"
//#define PMK_8		 							"PMK_8"

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* ST_P11_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
