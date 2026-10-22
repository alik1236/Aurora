# 四驱机械臂小车（DRV8833 版）

与 TB6612 版共用同一套控制逻辑（PS2、舵机、上电、刹车、失联保护），区别只在电机驱动层和引脚。

- 主控：STM32F103C8T6，72 MHz
- 驱动：DRV8833 × 2，四路直流电机
- 电机 PWM：TIM2 + TIM1 共 **8 路 20 kHz**（每个电机 IN1/IN2 各一路）
- 舵机：TIM4 三路 50 Hz（云台 / 机械臂 / 抓爪）
- 遥控：PS2（SPI1，PA4~PA7）
- 调试：**USART3**（PB10/PB11），115200
- 供电：**只能 2S 锂电（7.4 V，满电 8.4 V）**，DRV8833 的 VM 上限 10.8 V，3S 会超压

## 与 TB6612 版的关键差异

| 项目 | TB6612 版 | DRV8833 版 |
| --- | --- | --- |
| 每电机引脚 | 1 路 PWM + 2 方向 | **2 路 PWM（IN1/IN2）** |
| 使能 | STBY × 2 | nSLEEP × 2 |
| 电机 PWM | TIM2 CH1~4 | **TIM2 CH1~4 + TIM1 CH1~4** |
| 调试串口 | USART1（PA9/PA10） | **USART3（PB10/PB11）** |
| 电池 | 2S / 3S 均可 | **仅 2S** |

## 引脚分配

| 功能 | 引脚 | 说明 |
| --- | --- | --- |
| FL 左前 IN1 / IN2 | PA0 / PA8 | TIM2_CH1 / TIM1_CH1 |
| RL 左后 IN1 / IN2 | PA1 / PA9 | TIM2_CH2 / TIM1_CH2 |
| FR 右前 IN1 / IN2 | PA2 / PA10 | TIM2_CH3 / TIM1_CH3 |
| RR 右后 IN1 / IN2 | PA3 / PA11 | TIM2_CH4 / TIM1_CH4 |
| DRV8833 #1 nSLEEP | PB3 | 高电平使能 |
| DRV8833 #2 nSLEEP | PB9 | 高电平使能 |
| PS2 | PA4 CS、PA5 CLK、PA6 DAT、PA7 CMD | SPI1 |
| 舵机 | PB6 / PB7 / PB8 | TIM4_CH1~3 |
| 调试串口 | PB10 TX、PB11 RX | USART3 |
| LED | PC13 | 低电平亮 |
| SWD | PA13 / PA14 | 调试 |

## DRV8833 接线

每片 DRV8833 两个通道：

| 引脚 | 接到 |
| --- | --- |
| VM | 2S 电池正极（经开关、保险） |
| GND | 系统地 |
| nSLEEP | PB3（#1）/ PB9（#2） |
| AIN1 / AIN2 | 左前 PA0 / PA8（#1）；右前 PA2 / PA10（#2） |
| BIN1 / BIN2 | 左后 PA1 / PA9（#1）；右后 PA3 / PA11（#2） |
| AOUT1 / AOUT2 | 对应电机 |
| BOUT1 / BOUT2 | 对应电机 |
| nFAULT | 不接（未使用可悬空；裸芯片建议 10 kΩ 上拉） |

> 模块板上若额外有 VCC/3V3 引脚，按模块丝印接 3.3 V；裸芯片没有独立逻辑 VCC。

## 电机真值表

| IN1 | IN2 | 状态 |
| --- | --- | --- |
| PWM | 0 | 正转，占空比 = PWM |
| 0 | PWM | 反转，占空比 = PWM |
| 1 | 1 | 短接制动 |
| 0 | 0 | 滑行 |

## CubeMX 配置

### 时钟（与 TB6612 版相同）

HSE 8 MHz → PLL ×9 → 72 MHz；AHB /1、APB1 /2（36 MHz）、APB2 /1。

### GPIO

| 引脚 | 模式 | 初始电平 |
| --- | --- | --- |
| PA4 | Output Push Pull | High |
| PB3 / PB9 | Output Push Pull | Low |
| PC13 | Output Push Pull | High |

SYS Debug = **Serial Wire**（PB3 是 JTDO，必须关 JTAG）。

### TIM1 与 TIM2（各 4 路电机 PWM）

两个定时器参数完全一样：

| 参数 | 值 |
| --- | --- |
| Clock Source | Internal Clock |
| Channel1~4 | PWM Generation CHx |
| Prescaler | 0 |
| Counter Mode | Up |
| Counter Period (ARR) | 3599 |
| auto-reload preload | Enable |
| PWM Mode | PWM mode 1 |
| Pulse | 0 |
| Output compare preload | Enable |
| CH Polarity | High |

- TIM1 → PA8/PA9/PA10/PA11
- TIM2 → PA0/PA1/PA2/PA3

### TIM4（舵机，同 TB6612 版）

PSC 71、ARR 19999、CH1~3 PWM、Pulse 1500、PB6/PB7/PB8。

### SPI1（PS2）

Full-Duplex Master、8 bit、MSB、256 分频、CPOL High、CPHA 2 Edge、NSS Software、CRC Disabled。

### USART3（调试）

Asynchronous、115200、8N1、PB10/PB11。

### Project Manager

- Toolchain：STM32CubeIDE
- 取消勾选 “Generate peripheral initialization as a pair of .c/.h files per peripheral”
- 勾选 Keep User Code
- 生成后复制本目录 `Core/Inc`、`Core/Src`，用本目录 `main.c` 覆盖生成的 main.c

## 代码结构

| 文件 | 说明 |
| --- | --- |
| `motor.c` | DRV8833 双 PWM 驱动（本版主要改动） |
| `main.c` | TIM1/TIM2/TIM4/SPI1/USART3 初始化 |
| `debug.c` | 串口改为 huart3 |
| `ps2.c`、`servo.c`、`chassis.c`、`servo_teleop.c` | 与 TB6612 版相同 |
| `app_config.h` | 参数相同，`CFG_ALLOW_REVERSE` 默认 0 |

## 默认操作

与 TB6612 版一致：上电即进入可行驶状态，驱动保持休眠，首次推杆唤醒；失联 200 ms 制动，恢复后需摇杆回中 0.6 s；松杆/后拉/L2 刹车。

## 注意

- **不要用 3S 电池**，DRV8833 VM 最大 10.8 V。
- 每通道连续 1.2 A、峰值 2 A；四个电机堵转可能超限，建议选堵转 ≤1.2 A 的电机。
- nSLEEP 不能悬空，代码上电默认拉低，进入可行驶状态后拉高。
- 调试串口从 USART1 改到了 USART3（PB10/PB11），接线时注意。

本版源码已用 CubeIDE 自带 ARM GCC 做过全量编译 + 链接验证。
