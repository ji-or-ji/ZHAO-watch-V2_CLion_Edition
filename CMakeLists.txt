cmake_minimum_required(VERSION 3.20)

# ========== 交叉编译设置==========
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)

# 阻止 CMake 在编译器检测时尝试链接可执行文件
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 强制生成 .elf 文件，避免 .exe 后缀
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# ========== 项目定义 ==========
project(Ultra-thin-ZHAO-Watch-V2 C ASM)

# ==================== 工具链路径（保留供自定义命令使用）====================
if(WIN32)
    set(TOOLCHAIN_PATH "D:/Program Files/GNU Arm Embedded Toolchain/10 2021.10/bin")
else()
    set(TOOLCHAIN_PATH "/usr/local/gcc-arm-none-eabi/bin")
endif()

# objcopy/size 等工具仍需要手动指定路径
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy.exe")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/arm-none-eabi-objdump.exe")
set(CMAKE_SIZE   "${TOOLCHAIN_PATH}/arm-none-eabi-size.exe")

# ==================== 关键宏定义与编译选项 ====================
add_compile_definitions(
        STM32F103xB
        STM32F10X_MD
        USE_STDPERIPH_DRIVER
        HSE_VALUE=8000000
)

add_compile_options(
        -mcpu=cortex-m3
        -mthumb
        -mfloat-abi=soft
        -Os
        -flto
        -g3
        -gdwarf-2
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
        -fno-common
        -fstack-usage
)

# ==================== 包含路径 ====================
include_directories(
        User
        System
        APPdata
        MPU6050
        Drivers/STM32F10x_StdPeriph_Driver/inc
        Drivers/STM32F10x_StdPeriph_Driver/src
        Drivers/CMSIS/Include
        Drivers/CMSIS/Device/ST/STM32F1xx/Include
        Libraries/Startup
)

# ==================== 链接脚本 ====================
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Libraries/LinkerScript/STM32F103C8Tx_FLASH.ld)
if(NOT EXISTS ${LINKER_SCRIPT})
    message(FATAL_ERROR "链接脚本未找到: ${LINKER_SCRIPT}")
endif()

# ==================== 收集所有源文件 ====================
set(STARTUP_ASM_SRCS
        Libraries/Startup/startup_stm32f10x_md.S
)

set(CMSIS_SRCS
        Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f10x.c
)

set(STD_PERIPH_SRCS
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c
        Drivers/STM32F10x_StdPeriph_Driver/src/misc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c
)

set(USER_DRIVER_SRCS
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/Key.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/OLED.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/menu.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/Time_page.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/OLED_Data.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/menu_Data.c
)

set(SYSTEM_SRCS
        System/Delay.c
        System/MyRTC.c
        System/Serial2.c
        System/Serial3.c
        System/stm32_power.c
        System/Timer.c
)

set(APP_SRCS
        APPdata/AHT20.c
        APPdata/ASR_su_21t.c
        APPdata/CH7800play.c
        APPdata/RF_Ev1527.c
        APPdata/calculator.c
        APPdata/voltage_mea.c
)

set(MPU6050_SRCS
        MPU6050/MPU6050.c
        MPU6050/MyI2C.c
)

set(USER_SRCS
        User/main.c
        User/stm32f10x_it.c
)

# ==================== 创建可执行目标 ====================
add_executable(${PROJECT_NAME}
        ${STARTUP_ASM_SRCS}
        ${CMSIS_SRCS}
        ${STD_PERIPH_SRCS}
        ${USER_DRIVER_SRCS}
        ${SYSTEM_SRCS}
        ${APP_SRCS}
        ${MPU6050_SRCS}
        ${USER_SRCS}
)

# ==================== 启用数学库=======================
target_link_libraries(${PROJECT_NAME} m)

# ==================== 强制清除 Windows 目标属性 ====================
set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        SUFFIX ".elf"
        PREFIX ""
)

# ==================== 链接选项 ====================
target_link_options(${PROJECT_NAME} PRIVATE
        -T ${LINKER_SCRIPT}
        -mcpu=cortex-m3
        -mthumb
        -mfloat-abi=soft
        -specs=nano.specs
        -specs=nosys.specs
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
        -static
        "LINKER:-u,_printf_float"
        "LINKER:-u,_scanf_float"
)

# ==================== 生成十六进制、二进制文件 ====================
add_custom_target(${PROJECT_NAME}.hex ALL
        DEPENDS ${PROJECT_NAME}
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
        COMMENT "生成 HEX 文件"
)

add_custom_target(${PROJECT_NAME}.bin ALL
        DEPENDS ${PROJECT_NAME}
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMENT "生成 BIN 文件"
)

# 编译后显示大小信息
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "固件大小:"
)