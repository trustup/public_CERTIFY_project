===================================================================
HOW TO include mqttclient application in IAR L462 and L496 projects
===================================================================
By default the mqttclient application is not included in the delivered firmware. To include it follow the instruction below:

================================================
1/ Remove "Exclude from build" option for the following components of the IART project
--------------------------
- Middlewares/ST/STM32_Cellular/Samples/MQTT
- Middlewares/ST/STM32_Network_Library
- Middlewares/ST/STM32_Cellular/Modules/MbedTLS_Wrapper
- Middlewares/Third_Party/LiamBindle_mqtt-c
- Middlewares/Third_Party/MbedTLS
=======================================================================
2/ Define following Constants in Middlewares/ST/STM32_Cellular/Samples/MQTT/Inc/mqttclient_conf.h
(To get these parameters from MQTT server account)
Note: These parameters can be defined by setup menu: 
	At boot menu choose "Setup configuration Menu" then "Configuration: Mqttclient" and enter "URL", "username" and "password"
	The other default parameters can be kept.
---------------------------------------------------
To update:
#define MQTTCLIENT_DEFAULT_SERVER_NAME     ((uint8_t *)"<TO_BE_DEFINED>")  /* mqtt server URL       */
#define MQTTCLIENT_DEFAULT_USERNAME        ((uint8_t *)"<TO_BE_DEFINED>")  /* mqtt server user name */
#define MQTTCLIENT_DEFAULT_PASSWORD        ((uint8_t *)"<TO_BE_DEFINED>")  /* mqtt server password  */
=======================================================================
3/only if stackhero-network.com then ROOT CA for MQTT Server authentication must be updated in mqttclient_conf.h
(To get the certificate from MQTT server account)
Note: if the certificate is wrong only a warning error will occur and the application will work
-------------------------------------------------
#define MQTTCLIENT_ROOT_CA = {...}
=======================================================================
4/ Remove other applications from Firmware
(only L462 board because of memory lack)
--------------------------
in plf_features.h :
- Following variables to unset :
#define USE_ECHO_CLIENT    (0)
#define USE_HTTP_CLIENT    (0)
#define USE_PING_CLIENT    (0)
#define USE_COM_CLIENT     (0)
- Following variables to set :
#define USE_MQTT_CLIENT    (1)
=======================================================================
