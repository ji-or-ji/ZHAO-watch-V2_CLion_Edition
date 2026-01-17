cmake_minimum_required(VERSION 3.20)
project(Ultra-thin-ZHAO-Watch-V2 C ASM)

# ==================== 工具链设置 ====================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

if(WIN32)
    # Windows 上的工具链路径
    set(TOOLCHAIN_PATH "D:/Program Files/GNU Arm Embedded Toolchain/10 2021.10/bin")
#    set(EXE_SUFFIX ".exe")
else()
    # Linux/Mac 上的路径
set(TOOLCHAIN_PATH "/usr/local/gcc-arm-none-eabi/bin")
    set(EXE_SUFFIX "")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/arm-none-eabi-gcc${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-g++${EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc${EXE_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy${EXE_SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PATH}/arm-none-eabi-objdump${EXE_SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN_PATH}/arm-none-eabi-size${EXE_SUFFIX}")

# ==================== 关键宏定义与编译选项 ====================
# 对于STM32F103C8T6（中容量设备），需要定义：
# 1. STM32F103xB - 设备系列定义
# 2. USE_STDPERIPH_DRIVER - 使用标准外设库
# 3. STM32F10X_MD - 中容量设备（必须）
add_compile_definitions(
        STM32F103xB
        STM32F10X_MD  # 这是关键！中容量设备定义
        USE_STDPERIPH_DRIVER
        HSE_VALUE=8000000  # 外部晶振频率，根据实际硬件设置
)

add_compile_options(
        -mcpu=cortex-m3
        -mthumb
        -mfloat-abi=soft
        -Og
        -g
        -gdwarf-2
        # 代码生成选项
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
        # 特殊处理
        -fno-common
        -fstack-usage
)

# ==================== 包含路径 ====================
include_directories(
        # 用户代码目录
        User
        System
        APPdata
        MPU6050
        # 自定义驱动头文件目录
        Drivers/STM32F10x_StdPeriph_Driver/inc
        Drivers/STM32F10x_StdPeriph_Driver/src
        # CMSIS核心文件 - 这些路径必须正确
        Drivers/CMSIS/Include  # core_cm3.h 应该在这里
        Drivers/CMSIS/Device/ST/STM32F1xx/Include
        # Startup目录（包含core_cm3.h的备份位置）
        Libraries/Startup
)

# ==================== 链接脚本 ====================
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Libraries/LinkerScript/STM32F103C8Tx_FLASH.ld)
if(NOT EXISTS ${LINKER_SCRIPT})
    message(FATAL_ERROR "链接脚本未找到: ${LINKER_SCRIPT}")
endif()

# ==================== 收集所有源文件 ====================
# 启动文件
set(STARTUP_ASM_SRCS
        Libraries/Startup/startup_stm32f10x_md.S
)

# CMSIS 系统文件
set(CMSIS_SRCS
        Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f10x.c
)

# 标准外设库源文件（选择性添加）
set(STD_PERIPH_SRCS
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
        Drivers/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c
        Drivers/STM32F10x_StdPeriph_Driver/src/misc.c
        # 根据需要添加其他外设驱动
)

# 用户自定义驱动（位于StdPeriph_Driver/inc目录下的.c文件）
set(USER_DRIVER_SRCS
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/Key.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/OLED.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/menu.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/Time_page.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/OLED_Data.c
        Drivers/STM32F10x_StdPeriph_Driver/inc/menu_Data.c
)

# 系统文件
set(SYSTEM_SRCS
        System/Delay.c
        System/MyRTC.c
        System/Serial2.c
        System/stm32_power.c
        System/Timer.c
        # 添加其他系统文件
)

# 应用文件
set(APP_SRCS
        APPdata/AHT20.c
        APPdata/ASR_su_21t.c
        APPdata/CH7800play.c
        APPdata/RF_Ev1527.c
        # 添加其他应用文件
)

# MPU6050驱动
set(MPU6050_SRCS
        MPU6050/MPU6050.c
        MPU6050/MyI2C.c
)

# 用户主文件
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
        -u _printf_float
        -u _scanf_float
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