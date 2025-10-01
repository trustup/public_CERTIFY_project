################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Dev/DEMO-CERTIFY-BOOTSTRAPPING/Middlewares/ST/STM32_Cellular/Samples/Ping/Src/ping_client.c 

OBJS += \
./Middlewares/Cellular/Samples/Ping/ping_client.o 

C_DEPS += \
./Middlewares/Cellular/Samples/Ping/ping_client.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Cellular/Samples/Ping/ping_client.o: C:/Dev/DEMO-CERTIFY-BOOTSTRAPPING/Middlewares/ST/STM32_Cellular/Samples/Ping/Src/ping_client.c Middlewares/Cellular/Samples/Ping/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32L462xx '-DMBEDTLS_CONFIG_FILE=<mbedtls_config.h>' -DUSE_HAL_DRIVER -DUSE_CUSTOM_CONFIG=1 -DDISCO_L462 -DUSE_STM32L462E_CELL01 -c -I"C:/Dev/DEMO-CERTIFY-BOOTSTRAPPING/Projects/B_L462E/Demonstrations/Cellular/IDE/STM32CubeIDE/Middlewares/Cellular/Modules/PKCS11/Inc" -I../../../Core/Inc -I../../../STM32_Cellular/App -I../../../STM32_Cellular/Target -I../../../../../../../Drivers/BSP/B_L462E_CELL01 -I../../../../../../../Drivers/BSP/Components/ssd1315 -I../../../../../../../Drivers/BSP/X_STMOD_PLUS_MODEMS/TYPE1SC/AT_modem_type1sc/Inc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../../../../../../../Drivers/STM32L4xx_HAL_Driver/Inc -I../../../../../../../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../../../../../../Middlewares/Third_Party/LwIP/system/arch -I../../../../../../../Middlewares/Third_Party/LwIP/src/include -I../../../../../../../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../../../../../../../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../../../../../../../Middlewares/Third_Party/LwIP/system -I../../../../../../../Middlewares/Third_Party/LiamBindle_mqtt-c/include -I../../../../../../../Middlewares/Third_Party/MbedTLS/include -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/Com/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/Custom/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/Echo/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/HTTP/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/MQTT/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/Ping/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Samples/UI/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/AT_Core/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Cellular_Service/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Error/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Ipc/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/PPPosif/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Runtime_Library/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Rtosal/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Core/Trace/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Interface/Cellular_Mngt/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Interface/Com/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Interface/Data_Cache/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Cmd/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/DataCache_Supplier/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/MbedTLS_Wrapper/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Ndlc/Core/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Ndlc/Interface/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Setup/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/SPI/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Stack_Analysis/Inc -I../../../../../../../Middlewares/ST/STM32_Cellular/Modules/Time_Date/Inc -I../../../../../../../Middlewares/ST/STM32_Network_Library/Includes -I../../../../../../../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Cellular-2f-Samples-2f-Ping

clean-Middlewares-2f-Cellular-2f-Samples-2f-Ping:
	-$(RM) ./Middlewares/Cellular/Samples/Ping/ping_client.cyclo ./Middlewares/Cellular/Samples/Ping/ping_client.d ./Middlewares/Cellular/Samples/Ping/ping_client.o ./Middlewares/Cellular/Samples/Ping/ping_client.su

.PHONY: clean-Middlewares-2f-Cellular-2f-Samples-2f-Ping

