################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/Third_Party/LwIP/system/OS/subdir.mk
-include Middlewares/Third_Party/LwIP/netif/ppp/subdir.mk
-include Middlewares/Third_Party/LwIP/core/ipv4/subdir.mk
-include Middlewares/Third_Party/LwIP/core/subdir.mk
-include Middlewares/Third_Party/LwIP/api/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/CMSIS_RTOS/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/subdir.mk
-include Middlewares/Cellular/Samples/UI/subdir.mk
-include Middlewares/Cellular/Samples/Ping/subdir.mk
-include Middlewares/Cellular/Samples/HTTP/subdir.mk
-include Middlewares/Cellular/Samples/Freertos/subdir.mk
-include Middlewares/Cellular/Samples/Echo/subdir.mk
-include Middlewares/Cellular/Samples/Custom/subdir.mk
-include Middlewares/Cellular/Samples/Com/subdir.mk
-include Middlewares/Cellular/Modules/Time_Date/subdir.mk
-include Middlewares/Cellular/Modules/Stack_Analysis/subdir.mk
-include Middlewares/Cellular/Modules/Setup/subdir.mk
-include Middlewares/Cellular/Modules/SPI/subdir.mk
-include Middlewares/Cellular/Modules/PKCS11/Test/subdir.mk
-include Middlewares/Cellular/Modules/PKCS11/Src/subdir.mk
-include Middlewares/Cellular/Modules/Ndlc/subdir.mk
-include Middlewares/Cellular/Modules/DataCache_Supplier/subdir.mk
-include Middlewares/Cellular/Modules/Cmd/subdir.mk
-include Middlewares/Cellular/Interface/Data_Cache/subdir.mk
-include Middlewares/Cellular/Interface/Com/subdir.mk
-include Middlewares/Cellular/Interface/Cellular_Mngt/subdir.mk
-include Middlewares/Cellular/Core/Trace/subdir.mk
-include Middlewares/Cellular/Core/Runtime_Library/subdir.mk
-include Middlewares/Cellular/Core/Rtosal/subdir.mk
-include Middlewares/Cellular/Core/PPPosif/subdir.mk
-include Middlewares/Cellular/Core/Ipc/subdir.mk
-include Middlewares/Cellular/Core/HW_Specific/subdir.mk
-include Middlewares/Cellular/Core/Error/subdir.mk
-include Middlewares/Cellular/Core/Cellular_Service/subdir.mk
-include Middlewares/Cellular/Core/AT_Core/subdir.mk
-include Drivers/STM32L4xx_HAL_Driver/subdir.mk
-include Drivers/CMSIS/subdir.mk
-include Drivers/BSP/X_STMOD_PLUS_MODEMS/TYPE1SC/subdir.mk
-include Drivers/BSP/Components/subdir.mk
-include Drivers/BSP/B_L462_CELL01/subdir.mk
-include Application/User/subdir.mk
-include Application/Startup/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Prj_Murata_L462
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Prj_Murata_L462.elf \

MAP_FILES += \
Prj_Murata_L462.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Prj_Murata_L462.list \

OBJCOPY_BIN += \
Prj_Murata_L462.bin \


# All Target
all: main-build

# Main-build Target
main-build: Prj_Murata_L462.elf secondary-outputs

# Tool invocations
Prj_Murata_L462.elf Prj_Murata_L462.map: $(OBJS) $(USER_OBJS) C:\Dev\DEMO-CERTIFY-BOOTSTRAPPING\Projects\B_L462E\Demonstrations\Cellular\IDE\STM32CubeIDE\STM32L462RETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Prj_Murata_L462.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Dev\DEMO-CERTIFY-BOOTSTRAPPING\Projects\B_L462E\Demonstrations\Cellular\IDE\STM32CubeIDE\STM32L462RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Prj_Murata_L462.map" -Wl,--gc-sections -static  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Prj_Murata_L462.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Prj_Murata_L462.list"
	@echo 'Finished building: $@'
	@echo ' '

Prj_Murata_L462.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Prj_Murata_L462.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Prj_Murata_L462.bin Prj_Murata_L462.elf Prj_Murata_L462.list Prj_Murata_L462.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
