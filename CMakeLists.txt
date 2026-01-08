cmake_minimum_required(VERSION 3.20)
project(Ultra-thin-ZHAO-Watch-V2 C ASM)

# ==================== 工具链设置 ====================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PATH "/usr/local/gcc-arm-none-eabi/bin")
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PATH}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PATH}/arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PATH}/arm-none-eabi-objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PATH}/arm-none-eabi-size)

# ==================== 关键宏定义与编译选项 ====================
# 1. 芯片型号定义：STM32F103C8T6 属于中容量，必须定义为 STM32F103xB
# 2. 启用标准库
add_compile_definitions(STM32F103xB USE_STDPERIPH_DRIVER)

add_compile_options(
        -mcpu=cortex-m3
        -mthumb
        -mfloat-abi=soft      # F103 无硬件浮点单元，必须用 soft
        # 优化与调试
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
        # 1. 用户代码目录（User/ 下的 .h）
        User
        # 2. 系统与硬件层目录
        System
        Hardware
        # 3. 应用数据与传感器目录
        APPdata
        MPU6050
        # 4. STM32标准外设库目录
        Drivers/STM32F10x_StdPeriph_Driver/inc/
        Drivers/CMSIS/Device/ST/STM32F1xx/Include
        Drivers/CMSIS/Include
)

# ==================== 链接脚本与链接选项 ====================
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Libraries/LinkerScript/STM32F103C8Tx_FLASH.ld)
if(NOT EXISTS ${LINKER_SCRIPT})
    message(FATAL_ERROR "链接脚本未找到: ${LINKER_SCRIPT}")
endif()

add_link_options(
        -T ${LINKER_SCRIPT}
        -mcpu=cortex-m3
        -mthumb
        -mfloat-abi=soft
        -specs=nano.specs        # 使用精简版 libc，减小体积
        -specs=nosys.specs       # 无系统调用
        -Wl,--gc-sections        # 链接时移除未使用的段
        -Wl,--print-memory-usage # 显示内存使用情况（非常有用！）
        -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
        -static
        -u _printf_float         # 如果代码中使用了 printf 打印浮点数，需要此选项
        -u _scanf_float
)

# ==================== 收集所有源文件 ====================
# 启动文件 (汇编)
set(STARTUP_ASM_SRCS
        Libraries/Startup/startup_stm32f10x_md.s
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.h
        User/main.c
)

# 标准外设库源文件 (按需添加，避免编译未使用的驱动以节省时间)
file(GLOB_RECURSE STD_PERIPH_SRCS
        Drivers/STM32F10x_StdPeriph_Driver/src/*.c
)

# CMSIS 系统文件
set(CMSIS_SRCS
        Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f10x.c
        Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f10x.h
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.h
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.h
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.h
        User/main.c
        User/main.c
)

# 用户与应用代码 (递归收集所有 .c 文件)
file(GLOB_RECURSE USER_SRCS
        User/*.c
        System/*.c
        Hardware/*.c
        APPdata/*.c
        MPU6050/*.c
)

# ==================== 创建可执行目标 ====================
add_executable(${PROJECT_NAME}
        ${STARTUP_ASM_SRCS}
        ${CMSIS_SRCS}
        ${STD_PERIPH_SRCS}
        ${USER_SRCS}
        Drivers/STM32F10x_StdPeriph_Driver/inc/AD.h
        User/main.c
)

# ==================== 后续步骤：生成十六进制、二进制文件 ====================
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