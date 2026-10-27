# 超薄版 ZHAO-Watch V2 · CLion 版

一块开源的 STM32 手表。固件来自 B 站 UP 主「时光久泽」的 ZHAO-Watch 项目，本仓库把这份 Keil µVision5 工程迁到 CLion + CMake + OpenOCD，并配套超薄版（L 型主板）的硬件改动。

> **固件主体不是本项目原创。** 原作者是 [时光久泽](https://space.bilibili.com/1770356056)，原项目按 GPL-3.0 开源。这里做的改造以构建系统与工程结构为主，业务逻辑尽量保持原样。

## 来源

| 出处 | 链接 |
| :-- | :-- |
| 作者 B 站空间 | <https://space.bilibili.com/1770356056> |
| 演示视频 | [BV1ob4deyEc6](https://www.bilibili.com/video/BV1ob4deyEc6/)（2024-09-15） |
| 立创开源工程 | [oshwhub.com/sgxn/ZHAO-Watch](https://oshwhub.com/sgxn/ZHAO-Watch) |
| 作者网盘 | 视频简介里有 123 云盘链接，含源码压缩包 |

原项目发表于 2024 年 9 月，GPL-3.0。作者的 GitHub 仓库当时标注「整理中」，所以早期源码主要走网盘分发。

V2 的硬件改动是把主板重构成 L 型以减小厚度，工程随之迁到 CMake。

## 硬件

| 部件 | 型号 | 说明 |
| :-- | :-- | :-- |
| 主控 | STM32F103C8T6 | Cortex-M3，64KB Flash / 20KB RAM，HSE 8MHz |
| 屏幕 | 128×64 OLED | 软件驱动，位于 `Drivers/.../inc/OLED.c` |
| 姿态 | MPU6050 | 软件 I2C，判断抬腕与落腕 |
| 温湿度 | AHT20 | 表盘旁显示环境数据 |
| 语音识别 | SU-21T | 串口 2 + 外部中断唤醒，支持离线命令词 |
| 语音播报 | CH7800 | 串口 3，帧格式 `0x7E CMD 0x00 0x02 0x00 DATA 0xEF` |
| 无线 | EV1527 | 315MHz / 433MHz 编码发射，表内可直接选通道发送 |
| 电源 | 锂电池 + ADC 分压 | PA12 检测充电状态 |

主板工程（立创 EDA）在 `bord/ReuseBlock_SU-21T.epro2`。

## 功能

**表盘**

时间、日期、星期、电池图形、温湿度。开机动画只在首次上电时播放，靠备份寄存器 `BKP_DR1 == 0xA5A5` 判断是否已经放过。

**交互**

- 抬腕点亮：MPU6050 中断触发，陀螺仪 X 轴超过阈值即唤醒
- 落腕熄屏：手腕放下时陀螺仪读数反向超过阈值
- 静止一段时间后自动切换成「微动唤醒」，轻微动作就能亮屏
- 长时间无按键自动进入待机
- 语音唤醒：SU-21T 中断拉低直接点亮并进入语音流程

**菜单一（主功能）**

| 项目 | 说明 |
| :-- | :-- |
| 电压测量 | 电压表，以及 0~30V 输入的示波器 |
| 315MHz 遥控 | EV1527 格式，选通道后下拨发送 |
| 收款微信 / 收款支付宝 | 全屏显示收款码 |
| Life_Play | 康威生命游戏 |
| 计算器 | 连续运算，四则、开方、三角函数与反三角函数 |
| 设置 | 系统参数 |

**菜单二（快捷）**

手电筒（PWM 调亮度，亮度记在备份寄存器里）、时间调整。秒表与闹钟目前是占位项，点了走 `error()`。

**语音**

SU-21T 识别到命令后可以联动打开对应功能（手表信息、电压、收款码、计算器），CH7800 负责回一句语音。语音回词可以直接连电脑替换。

## 目录结构

```
.
├── APPdata/            应用层：AHT20、SU-21T、CH7800、EV1527、计算器、电压测量
├── bord/               立创 EDA 主板工程
├── Drivers/
│   ├── CMSIS/          CMSIS 与 STM32F1 设备头
│   └── STM32F10x_StdPeriph_Driver/
│       ├── inc/        OLED、menu、Time_page 等业务驱动，以及少数 .c
│       └── src/        标准外设库
├── Libraries/
│   ├── LinkerScript/   STM32F103C8Tx_FLASH.ld
│   └── Startup/        各密度启动文件，本工程用 startup_stm32f10x_md.S
├── MPU6050/            MPU6050 与软件 I2C
├── System/             延时、RTC、串口 2/3、电源、定时器
├── User/               main.c、中断处理、外设配置头
├── CMakeLists.txt
└── .clion-openocd.cfg  OpenOCD 调试配置（ST-Link / SWD）
```

## 构建

需要 arm-none-eabi-gcc、CMake 3.20+、Ninja（或任意 CMake 生成器）。验证过的工具链是 `GNU Arm Embedded Toolchain 10 2021.10`。

`CMakeLists.txt` 顶部写死了工具链路径：

```cmake
set(TOOLCHAIN_PATH "D:/Program Files/GNU Arm Embedded Toolchain/10 2021.10/bin")
```

换机器第一件事是改这里，同时把该目录加进 `PATH`，或者直接把 `arm-none-eabi-gcc` 放进环境变量再改路径。

```powershell
cmake -B build -G Ninja `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc
cmake --build build
```

编译选项是 `-Os` 加 LTO，链接时 `--gc-sections` 并打印内存占用。产物在 `build/`：

- `Ultra-thin-ZHAO-Watch-V2.elf`
- `Ultra-thin-ZHAO-Watch-V2.hex`
- `Ultra-thin-ZHAO-Watch-V2.bin`

## 烧录与调试

CLion 里用 OpenOCD 配置，`.clion-openocd.cfg` 已经写好 ST-Link + SWD + stm32f1x：

```
source [find interface/stlink.cfg]
transport select hla_swd
source [find target/stm32f1x.cfg]
reset_config srst_only
adapter speed 1000
```

命令行烧录：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Ultra-thin-ZHAO-Watch-V2.hex verify reset exit"
```

## 两处需要校准

代码里电池电压是 ADC 分压算出来的，换板子或换了分压电阻要重新标：

| 位置 | 变量 | 默认值 | 怎么调 |
| :-- | :-- | :-- | :-- |
| `Time_page.c` | `VIN` | 3.31 | 万用表量降压芯片输出的 3.3V 轨，或者量单片机对屏幕 RES 脚的电压 |
| `Time_page.c` | `BAT_volt_CAL` | 2.000 | 显示比实际高就调小一点，低就调大 |

同文件里还有一组 `battery_voltage` / `battery_level` 对照表，把电压映射成电量百分比，觉得曲线不合适也可以改。

## 排障

**屏幕闪屏**：原作者在视频简介里给的办法，把 `OLED.c` 中 `OLED_Init()` 里 `0xd5` 命令后面那个参数改成 `0x80`。

## 已知状态

- 秒表、闹钟是占位，还没实现
- `CH7800_TelltheTime()` 是空函数，整点报时没做
- 编译会有一些变量范围的警告，作者注释里说明不影响运行
- 分支：`develop` 是默认分支，最新代码在这；`master` 停在较早的提交上

## 许可

GPL-3.0。固件主体版权归原作者 [时光久泽](https://space.bilibili.com/1770356056) 所有，本项目沿用同一协议。见 [LICENSE](LICENSE)。
