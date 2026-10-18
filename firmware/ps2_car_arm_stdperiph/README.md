# STM32F103C8T6 PS2 遥控小车 + 机械臂（标准库 / 寄存器直操作版）

> 本固件是 `STM32F103_PS2_Car_Arm` 的**标准库自包含版**：
> 不依赖 HAL / CubeMX / CMSIS，纯寄存器级直操作，Makefile + 批处理一键编译烧录。
> 功能、引脚、操作方式与原 HAL 版完全一致。

---

## 1. 编译与烧录

**环境**：Windows + STM32CubeIDE 自带工具链（`build.bat` 内已配置路径）。

```
双击 build.bat   -> 生成 firmware.hex
插好 ST-Link 后
双击 flash.bat   -> 烧录并复位运行
```

也可以命令行：`make clean && make all`（需要 `arm-none-eabi-gcc` 在 PATH）。

---

## 2. 引脚总表（与原文档一致）

| 引脚 | 功能 | 说明 |
| --- | --- | --- |
| PA0 | TIM2_CH1 | 夹爪舵机（右摇杆 Y 连续控制） |
| PA6 | TIM3_CH1 | 左电机 PWM1（L298N IN1/IN3 并联） |
| PA7 | TIM3_CH2 | 左电机 PWM2（IN2/IN4 并联） |
| PB0 | TIM3_CH3 | 右电机 PWM1 |
| PB1 | TIM3_CH4 | 右电机 PWM2 |
| PA8 / PA9 / PA10 | TIM1_CH1/2/3 | 备用舵机（上电保持 90°） |
| PB6 | TIM4_CH1 | 关节 1 |
| PB7 | TIM4_CH2 | 关节 2 |
| PB8 | TIM4_CH3 | 转台 |
| PB9 | TIM4_CH4 | 铲斗 |
| PB12 | GPIO 输入（上拉） | PS2 DAT |
| PB13 | GPIO 输出 | PS2 CMD |
| PB14 | GPIO 输出 | PS2 CS（即 ATT） |
| PB15 | GPIO 输出 | PS2 CLK |
| PC13 | GPIO 输出 | 板载 LED（在线常亮 / 失联快闪） |
| PA13 / PA14 | SWD | ST-Link 下载调试 |

---

## 3. 定时器配置

| 定时器 | PSC | ARR | 频率 | 用途 |
| --- | --- | --- | --- | --- |
| TIM3 | 63 | 999 | 1kHz | 电机（4 路） |
| TIM2 | 63 | 19999 | 50Hz | 夹爪（PA0） |
| TIM4 | 63 | 19999 | 50Hz | 机械臂 4 路（PB6~PB9） |
| TIM1 | 63 | 19999 | 50Hz | 备用 3 路（PA8~PA10） |

系统时钟：HSI 8MHz + PLL ×16 = **64MHz**（不依赖外部晶振）。
定时器计数时钟 64MHz / 64 = 1MHz，**1 个计数 = 1us**：
舵机脉宽 500 = 0°，1500 = 90°，2500 = 180°。

---

## 4. 接线（与原文档一致）

### 4.1 L298N（两片，四驱双 PWM 接法）

**L298N #1 —— 左侧**

| 引脚 | 接到 |
| --- | --- |
| IN1、IN3 | 并联后接 PA6 |
| IN2、IN4 | 并联后接 PA7 |
| ENA、ENB | 跳线帽都插上（常高） |
| OUT1 / OUT2 | 左前电机 |
| OUT3 / OUT4 | 左后电机 |

**L298N #2 —— 右侧**

| 引脚 | 接到 |
| --- | --- |
| IN1、IN3 | 并联后接 PB0 |
| IN2、IN4 | 并联后接 PB1 |
| ENA、ENB | 跳线帽都插上 |
| OUT1 / OUT2 | 右前电机 |
| OUT3 / OUT4 | 右后电机 |

**电源**：两片 +12V 并电池正，GND 并电池负且与核心板共地；5V 输出只让一片给核心板供电。

> 停止 = 两路 PWM 全高（短路刹车，坡上不溜车）；前进/后退 = 单路 PWM 调速。
> **ENA/ENB 跳线帽必须插上**，否则马达不受控（全速或不动）。

### 4.2 PS2 接收器

| 接收器 | STM32 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| DAT | PB12 |
| CMD | PB13 |
| CS | PB14 |
| CLK | PB15 |
| ACK | 不接（`PS2_USE_ACK = 0`） |

### 4.3 舵机电源

舵机必须**单独供 5V**（建议 5V/3A 以上或 UBEC），GND 与单片机共地；
不要从核心板 3.3V 取电（带不动，会导致复位）。

---

## 5. 操作方式

上电后不需要解锁，直接操作。

| 操作 | 功能 |
| --- | --- |
| 左摇杆 ↑ / ↓ | 前进 / 后退 |
| 左摇杆 ← / → | 左转 / 右转（可原地转） |
| 右摇杆 ↑ / ↓ | 夹爪开合（连续） |
| △ / ○ | 关节 1 加 / 减 |
| × / □ | 关节 2 加 / 减 |
| L1 / R1 | 转台 减 / 加（15°~165°） |
| 十字键 ↑ / ↓ | 铲斗 加 / 减（0°~90°） |
| 板载 LED | 常亮 = 手柄在线；快闪 = 失联 |

手柄失联超过 200ms，底盘自动刹车（机械臂保持最后位置）。

---

## 6. 代码结构

```
stm32f1_reg.h  寄存器定义与 GPIO/TIM 操作辅助（全工程共用）
main.c         时钟(SysTick/DWT)、外设初始化、10ms 主循环、启动代码
ps2.h/.c       PS2 手柄驱动（PB12~PB15，9 字节轮询）
motor.h/.c     L298N 双 PWM 电机驱动（TIM3 四路）
car.h/.c       摇杆解算、差速混控、斜坡、失联保护、LED
arm.h/.c       8 路舵机控制（TIM4×4 + TIM2 + TIM1×3）
Makefile / build.bat / flash.bat / stm32f1.ld   编译烧录
```

主循环：

```
每 10ms：
  PS2_Poll()  ->  Car_Update()  底盘
              ->  Arm_Update()  机械臂
```

---

## 7. 参数调整

| 位置 | 宏 | 默认 | 作用 |
| --- | --- | --- | --- |
| `car.c` | `CAR_RAMP_STEP` | 100 | 加速快慢 |
| `car.c` | `CAR_DEADZONE` | 25 | 摇杆死区 |
| `arm.h` | `ARM_STEP_DEG` | 5 | 每次按键转多少度 |
| `arm.h` | `ARM_REPEAT_MS` | 100 | 按住时重复间隔 |

舵机行程不合适，改 `arm.c` 里 `angle_to_pulse()` 的 500/2500（单位 us）。

---

## 8. 常见问题

**电机不转 / 只往一个方向转**
- L298N 的 ENA/ENB 跳线帽有没有插上？
- 电池接了吗？共地了吗？该侧两根信号线是否接反？

**舵机抖 / 单片机复位**
- 舵机电源功率不够，换 5V/3A 以上独立电源并确认共地。

**LED 快闪**
- 手柄没通信上：查 3.3V 供电、DAT/CMD 是否接反、CS 是否接 PB14。

**机械臂不动但底盘正常**
- 舵机信号线是否接在 PB6~PB9 / PA0；舵机电源是否上电。

---

## 9. 与原 HAL 版的差异

| 项目 | HAL 版 | 本版 |
| --- | --- | --- |
| 依赖 | HAL 库 + CubeMX 生成的 main.h/tim.h/gpio.h | 无，纯寄存器直操作 |
| 工程 | 需 CubeMX 生成工程后放入源码 | 自包含，Makefile + bat 直接编译 |
| 时钟 | HSE 8MHz × 9 = 72MHz | HSI 8MHz × 16 = 64MHz（免晶振） |
| 逻辑 | HAL_GetTick / HAL_Delay / HAL_GPIO | SysTick 1ms 节拍 + 寄存器操作 |
| 编译 | CubeIDE 内 Ctrl+B | `build.bat` 一键 |
