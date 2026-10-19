# CubeIDE + CubeMX 工程搭建指南（STM32F103C8T6・L298N 遥控小车）

> 适用：CRTC2026 机械臂小车（4 马达 + 5 舵机 + PS2 手柄，驱动 L298N ×2）
> 环境：STM32CubeIDE 2.2.0（已装），固件包 STM32Cube_FW_F1_V1.8.0（已装）
> 应用代码：
> `firmware/remote_car_l298n.c`
> （HAL 版，按此指南的引脚 / 时钟配置编写）



***

## 一、新建工程



1. 打开 STM32CubeIDE → **File → New → STM32 Project**；

2. 搜索框输入 `STM32F103C8`，在结果中选中 **STM32F103C8Tx** → **Next**；

3. 工程名建议 `firmware_cubeide`，注意**保存位置选&#x20;**`D:\桌面\Aurora\firmware`**&#x20;之外新建的文件夹**（避免和现有文件混在一起，建议放 `D:\桌面\Aurora\firmware\cubeide` 上一级）→ **Finish**；

4. 若提示选择固件版本，选 **V1.8.0**（本地已装，无需联网下载）；

5. 等待首次生成代码，完成后自动打开 `.ioc` 图形配置界面（CubeMX）。



***

## 二、CubeMX 引脚配置（.ioc 界面）

按下面顺序点，**每项都配，缺一不可**：

### 1. SYS → Debug（先做，释放 PA15）0



* 左侧 **System Core → SYS**；

* **Debug** 下拉选 **Serial Wire**。

### 2. 时钟（保持默认 HSI 8MHz，不碰外部晶振）



* **System Core → RCC**：HSE 保持 **Disabled**（不依赖晶振）；

* **Clock Configuration** 页：不动，保持默认 8MHz 即可。

### 3. 定时器（电机 1kHz / 舵机 50Hz）

每个 TIM 的配置：Prescaler、Counter Period 照下表填。



| 定时器  | 通道  | 引脚   | 模式（Mode）           | Prescaler | Counter Period | 用途     |
| ---- | --- | ---- | ------------------ | --------- | -------------- | ------ |
| TIM1 | CH4 | PA11 | PWM Generation CH4 | 79        | 99             | 左后轮 RL |
| TIM2 | CH4 | PA3  | PWM Generation CH4 | 79        | 99             | 右后轮 RR |
| TIM3 | CH1 | PA6  | PWM Generation CH1 | 79        | 99             | 左前轮 FL |
| TIM3 | CH2 | PA7  | PWM Generation CH2 | 79        | 99             | 右前轮 FR |
| TIM4 | CH1 | PB6  | PWM Generation CH1 | 79        | 1999           | 舵机 S1  |
| TIM4 | CH2 | PB7  | PWM Generation CH2 | 79        | 1999           | 舵机 S2  |
| TIM4 | CH3 | PB8  | PWM Generation CH3 | 79        | 1999           | 舵机 S3  |
| TIM4 | CH4 | PB9  | PWM Generation CH4 | 79        | 1999           | 舵机 S4  |

操作：左侧 **Timers → TIMx** → 对应 Channel 下拉选 **PWM Generation CHn** → 在下方 **Parameter Settings** 填 Prescaler / Counter Period。**Pulse 初始值填 0**。

### 4. GPIO（方向 + PS2 + 软舵机）

在芯片引脚图上逐个左键点击设成以下功能：



| 引脚   | 功能                            | 说明                          |
| ---- | ----------------------------- | --------------------------- |
| PA0  | GPIO\_Output                  | S5 夹爪软舵机                    |
| PA1  | GPIO\_Output                  | FL IN1                      |
| PA2  | GPIO\_Output                  | RR IN1                      |
| PA4  | GPIO\_Output                  | FR IN1                      |
| PA8  | GPIO\_Output                  | FL IN2                      |
| PA9  | GPIO\_Output                  | RL IN2                      |
| PA10 | GPIO\_Output                  | FR IN2                      |
| PA12 | GPIO\_Output                  | RL IN1                      |
| PA15 | GPIO\_Output                  | RR IN2（SYS=Serial Wire 后可用） |
| PB12 | GPIO\_Output                  | PS2 ATT                     |
| PB13 | GPIO\_Input（**GPIO\_PULLUP**） | PS2 DAT，须设上拉                |
| PB14 | GPIO\_Output                  | PS2 CMD                     |
| PB15 | GPIO\_Output                  | PS2 CLK                     |

要点：PB13 点击后，下方 **GPIO mode** 选 **Input mode**，**GPIO Pull-up/Pull-down** 选 **Pull-up**。

### 5. 生成代码



* 点工具栏 **保存（Ctrl+S）** → 弹窗问 "Generate Code?" 选 **Yes**。



***

## 三、放入应用代码并接线调用



1. 把 `firmware/remote_car_l298n.c` **复制**到生成的工程目录 `Core/Src/` 下；

2. 打开 `Core/Src/main.c`，加三处（都在 USER CODE 注释区域内，不要删生成代码）：



```
/\\\* USER CODE BEGIN 0 \\\*/

extern void RemoteCar\\\_Init(void);

extern void RemoteCar\\\_Loop(void);

/\\\* USER CODE END 0 \\\*/
```



```
\&#x20; /\\\* USER CODE BEGIN 2 \\\*/

\&#x20; RemoteCar\\\_Init();

\&#x20; /\\\* USER CODE END 2 \\\*/
```



```
\&#x20; /\\\* USER CODE BEGIN WHILE \\\*/

\&#x20; while (1)

\&#x20; {

\&#x20;   RemoteCar\\\_Loop();

\&#x20;   /\\\* USER CODE END WHILE \\\*/

\&#x20;   /\\\* USER CODE BEGIN 3 \\\*/

\&#x20; }

\&#x20; /\\\* USER CODE END 3 \\\*/
```

> 注意：如果生成代码后
> `while(1)`
> 里已有默认内容（如
> `HAL_Delay(1000);`
> ），保留不动，把
> `RemoteCar_Loop();`
> 加在 USER CODE BEGIN WHILE 区域即可。



***

## 四、编译



1. 点工具栏**锤子图标（Build）**；

2. 底部 Console 出现 `Finished building target: firmware_cubeide.elf`，上方无红色错误即可。



***

## 五、烧录（ST-Link）



1. **接线**：ST-Link `SWDIO→PA13`、`SWCLK→PA14`、`GND→GND`（3.3V 可不接，核心板单独供电）；

2. 菜单 **Run → Run Configurations**（或直接点绿色播放键）→ 选 **STM32 Cortex-M C/C++ Application** → 左侧选中你的工程；

3. **Debugger** 标签页：Debug probe 选 **ST-LINK (ST-Link GDB server)**，Interface 选 **SWD**，频率 1MHz（更稳）；

4. 点 **Run**（或 **Debug**）→ 首次可能弹信任 / 视角确认，接受即可；

5. 看到 `Download verified successfully` / 程序停在 main 即烧录成功，点 Resume（▶）运行。



***

## 六、验证与常见问题

**烧录后应看到：**



* 舵机 S1\~S5 通电后回中（90°）—— 没回中说明 50Hz 没配好或接线反；

* PS2 手柄按摇杆，车能前进 / 后退 / 转向 —— 不响应查 PB13 上拉、手柄配对模式；

* 夹爪（S5）按 △ 能开合 —— 软舵机 PA0 正常。

**常见问题：**



| 现象                                            | 原因 / 处理                                                 |
| --------------------------------------------- | ------------------------------------------------------- |
| 编译报 `undefined reference to 'RemoteCar_Init'` | remote\_car\_l298n.c 没放进 Core/Src，或没被工程编译（检查 Source 列表） |
| 烧录报 `No ST-Link detected`                     | 线序 / 接触 / 驱动（用 D:\ 桌面 \ST-Link 驱动 装一遍）                  |
| 电机只响不转                                        | L298N ENA/ENB **跳线帽没拆**、或 VSS/GND 没接                    |
| 电机方向反                                         | 调换该电机 IN1/IN2 两根线，或改代码里对应宏                              |
| 舵机抖动 / 尖叫                                     | 舵机电源不足（需要独立 5V/6V 供电，不要从单片机 3.3V 取）                     |



***

## 七、引脚总表（对照实物）



| 模块          | 接法                                                   |
| ----------- | ---------------------------------------------------- |
| L298N-1（左轮） | ENA=PA6 → FL；ENB=PA11 → RL；IN1/IN2 按第四节 GPIO 表       |
| L298N-2（右轮） | ENA=PA7 → FR；ENB=PA3 → RR；IN1/IN2 按第四节 GPIO 表        |
| L298N 供电    | VS=7.4V 电池，VSS=5V，GND 与单片机共地                         |
| PS2 接收器     | VCC=3.3V，GND=GND，DAT=PB13，CMD=PB14，ATT=PB12，CLK=PB15 |
| 舵机 S1\~S4   | 信号线 = PB6/PB7/PB8/PB9，电源 = V5（独立供电）                  |
| 舵机 S5（夹爪）   | 信号线 = PA0                                            |