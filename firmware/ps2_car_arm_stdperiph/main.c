/**
  ******************************************************************************
  * @file    main.c
  * @brief   PS2 遥控小车 + 机械臂（STM32F103C8T6，标准库/寄存器直操作版）
  *
  *  芯片    : STM32F103C8T6（64KB Flash / 20KB RAM）
  *  方式    : 寄存器级直操作（不依赖 HAL / CubeMX / CMSIS），可直接编译烧录
  *  时钟    : 内部 HSI 8MHz + PLL x16 = 64MHz（不依赖外部晶振）
  *  驱动    : 2xL298N（TIM3 四路双 PWM），4 个 TT 马达
  *  遥控    : PS2 手柄（软件模拟时序，PB12~PB15）
  *  机械臂  : TIM4 四路 + TIM2_CH1 夹爪 + TIM1 三路备用，共 8 路舵机
  *
  *  引脚分配（与原文档完全一致）：
  *    电机   : PA6=TIM3_CH1 PA7=TIM3_CH2（左电机）；PB0=TIM3_CH3 PB1=TIM3_CH4（右电机）
  *    夹爪   : PA0 = TIM2_CH1
  *    备用舵机: PA8/PA9/PA10 = TIM1_CH1/CH2/CH3（上电保持 90 中位）
  *    机械臂 : PB6~PB9 = TIM4_CH1~CH4（关节1/关节2/转台/铲斗）
  *    PS2   : DAT=PB12(输入上拉) CMD=PB13 CS=PB14 CLK=PB15
  *    LED   : PC13（在线常亮 / 失联快闪）
  *
  *  控制分工：
  *    左摇杆       -> 底盘前后 + 转向（差速，可原地转）
  *    右摇杆 Y     -> 夹爪开合（连续）
  *    △ / ○        -> 关节 1 加 / 减
  *    × / □        -> 关节 2 加 / 减
  *    L1 / R1      -> 转台 减 / 加（限位 15~165 度）
  *    十字键 上/下 -> 铲斗 加 / 减（限位 0~90 度）
  *
  *  编译烧录：双击 build.bat 生成 firmware.hex，插好 ST-Link 后双击 flash.bat。
  ******************************************************************************
  */

#include "stm32f1_reg.h"
#include "ps2.h"
#include "motor.h"
#include "car.h"
#include "arm.h"

/* ================= 系统时钟（HSI + PLL -> 64MHz） ================= */
static void SystemClock_Init(void)
{
    /* Flash：64MHz 需要 2 等待周期 + 预取使能 */
    REG(FLASH_BASE + 0x00) = 0x12UL;

    RCC_CR |= 1UL;                        /* HSI 使能 */
    while (!(RCC_CR & 2UL)) { }           /* 等 HSI 就绪 */

    /* PLL：输入 HSI/2=4MHz，倍频 x16 -> 64MHz（PLLMUL=0111, PLLSRC=0） */
    RCC_CFGR = (RCC_CFGR & ~(0xFUL << 18)) | (0x7UL << 18);
    /* APB1 预分频 /2（APB1 上限 36MHz）；APB2 不分频 */
    RCC_CFGR = (RCC_CFGR & ~(0x7UL << 8)) | (0x4UL << 8);

    RCC_CR |= (1UL << 24);                /* PLL 使能 */
    while (!(RCC_CR & (1UL << 25))) { }   /* 等 PLL 就绪 */
    RCC_CFGR |= (2UL << 0);               /* SW=PLL */
    while (((RCC_CFGR >> 2) & 3UL) != 2UL) { }
}

/* ================= SysTick 1ms 系统节拍 ================= */
volatile uint32_t g_sys_tick_ms;

static void SysTick_Init(void)
{
    SYSTICK_LOAD = 64000UL - 1UL;         /* 64MHz / 64000 = 1ms */
    SYSTICK_VAL  = 0UL;
    SYSTICK_CTRL = 0x07UL;                /* CLKSOURCE=core, TICKINT, ENABLE */
}

/* ================= DWT 微秒延时（PS2 用） ================= */
static void DWT_Init(void)
{
    REG(DEMCR_ADDR) |= (1UL << 24);       /* TRCENA */
    REG(DWT_BASE + 0x04) = 0UL;           /* CYCCNT = 0 */
    REG(DWT_BASE + 0x00) |= 1UL;          /* CYCCNTENA */
}

/* ================= 外设初始化 ================= */
static void Peripheral_Init(void)
{
    /* 时钟门控：AFIO GPIOA GPIOB GPIOC TIM1；TIM2 TIM3 TIM4 */
    RCC_APB2ENR |= (1UL << 0) | (1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 11);
    RCC_APB1ENR |= (1UL << 0) | (1UL << 1) | (1UL << 2);

    /* 释放 PA15/PB3/PB4/PB15：关闭 JTAG、保留 SWD（SWJ_CFG=010） */
    REG(AFIO_BASE + 0x04) = (REG(AFIO_BASE + 0x04) & ~(7UL << 24)) | (2UL << 24);

    /* --- 电机 PWM（复用推挽 50MHz）：PA6/PA7（左），PB0/PB1（右） --- */
    gpio_set(GPIOA_BASE, 6, 0xB);
    gpio_set(GPIOA_BASE, 7, 0xB);
    gpio_set(GPIOB_BASE, 0, 0xB);
    gpio_set(GPIOB_BASE, 1, 0xB);

    /* --- 机械臂舵机（复用推挽 50MHz）：PA0=TIM2_CH1, PB6~PB9=TIM4_CH1~4,
     *     PA8/PA9/PA10=TIM1_CH1/2/3 --- */
    gpio_set(GPIOA_BASE, 0, 0xB);
    gpio_set(GPIOB_BASE, 6, 0xB);
    gpio_set(GPIOB_BASE, 7, 0xB);
    gpio_set(GPIOB_BASE, 8, 0xB);
    gpio_set(GPIOB_BASE, 9, 0xB);
    gpio_set(GPIOA_BASE, 8, 0xB);
    gpio_set(GPIOA_BASE, 9, 0xB);
    gpio_set(GPIOA_BASE, 10, 0xB);

    /* --- PS2：CS=PB14 输出、CMD=PB13 输出、CLK=PB15 输出、DAT=PB12 输入上拉 --- */
    gpio_set(GPIOB_BASE, 14, 0x3);
    gpio_set(GPIOB_BASE, 13, 0x3);
    gpio_set(GPIOB_BASE, 15, 0x3);
    gpio_set(GPIOB_BASE, 12, 0x8);
    GPIO_ODR(GPIOB_BASE) |= (1UL << 12);   /* ODR=1 -> 上拉 */

    /* --- LED：PC13 推挽输出（初始灭） --- */
    gpio_set(GPIOC_BASE, 13, 0x3);
    gpio_set_pin(GPIOC_BASE, 13);

    /* --- 定时器 PWM ---
     *  TIM3 电机：PSC=63, ARR=999 -> 1MHz/1000 = 1kHz
     *  TIM1/2/4 舵机：PSC=63, ARR=19999 -> 1MHz/20000 = 50Hz（1 计数 = 1us） */
    tim_pwm_init(TIM3_BASE, 1, 63, 999, 0);
    tim_pwm_init(TIM3_BASE, 2, 63, 999, 0);
    tim_pwm_init(TIM3_BASE, 3, 63, 999, 0);
    tim_pwm_init(TIM3_BASE, 4, 63, 999, 0);

    tim_pwm_init(TIM2_BASE, 1, 63, 19999, 0);

    tim_pwm_init(TIM4_BASE, 1, 63, 19999, 0);
    tim_pwm_init(TIM4_BASE, 2, 63, 19999, 0);
    tim_pwm_init(TIM4_BASE, 3, 63, 19999, 0);
    tim_pwm_init(TIM4_BASE, 4, 63, 19999, 0);

    tim_pwm_init(TIM1_BASE, 1, 63, 19999, 1);
    tim_pwm_init(TIM1_BASE, 2, 63, 19999, 1);
    tim_pwm_init(TIM1_BASE, 3, 63, 19999, 1);
}

/* ================= 主循环 ================= */
static void App_Loop(void)
{
    static uint32_t last_ctrl = 0U;
    PS2_Data_t pad;

    if ((get_tick() - last_ctrl) >= 10U)
    {
        last_ctrl = get_tick();

        if (PS2_Poll(&pad) == 0U)
        {
            pad.ok = 0U;                  /* 失联：底盘自动刹车，机械臂保持 */
        }

        Car_Update(&pad);
        Arm_Update(&pad);
    }
}

/* ================= 中断向量表与启动 ================= */
extern int main(void);

void Reset_Handler(void);
static void Default_Handler(void) { for (;;) { } }

void NMI_Handler(void)         __attribute__((alias("Default_Handler")));
void HardFault_Handler(void)   __attribute__((alias("Default_Handler")));
void MemManage_Handler(void)   __attribute__((alias("Default_Handler")));
void BusFault_Handler(void)    __attribute__((alias("Default_Handler")));
void UsageFault_Handler(void)  __attribute__((alias("Default_Handler")));
void SVCall_Handler(void)      __attribute__((alias("Default_Handler")));
void DebugMon_Handler(void)    __attribute__((alias("Default_Handler")));
void PendSV_Handler(void)      __attribute__((alias("Default_Handler")));

/* SysTick 1ms 节拍中断 */
void SysTick_Handler(void) { g_sys_tick_ms++; }

__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[48])(void) = {
    (void (*)(void))0x20005000UL,   /* 栈顶 = RAM 顶（20KB） */
    Reset_Handler,
    NMI_Handler, HardFault_Handler, MemManage_Handler, BusFault_Handler,
    UsageFault_Handler, 0, 0, 0, 0, SVCall_Handler, DebugMon_Handler, 0,
    PendSV_Handler, SysTick_Handler,
    /* 以下为外设 IRQ（全部默认处理） */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
};

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;

__attribute__((naked, section(".text.Reset_Handler")))
void Reset_Handler(void)
{
    __asm volatile(
        "ldr  r0, =_sdata\n"
        "ldr  r1, =_edata\n"
        "ldr  r2, =_sidata\n"
        "movs r3, #0\n"
        "1: cmp  r0, r1\n"
        "bcs  2f\n"
        "ldr  r3, [r2], #4\n"
        "str  r3, [r0], #4\n"
        "b    1b\n"
        "2: ldr  r0, =_sbss\n"
        "ldr  r1, =_ebss\n"
        "movs r2, #0\n"
        "3: cmp  r0, r1\n"
        "bcs  4f\n"
        "str  r2, [r0], #4\n"
        "b    3b\n"
        "4: bl   main\n"
        "5: b    5b\n");
}

/* ================= 入口 ================= */
int main(void)
{
    SystemClock_Init();
    SysTick_Init();
    DWT_Init();
    Peripheral_Init();

    Motor_Init();                 /* L298N 底盘 PWM 清零 */
    Arm_Init();                   /* 机械臂舵机 PWM + 初始角度 */
    PS2_Init();                   /* 手柄初始化（约 0.5s） */

    for (;;)
    {
        App_Loop();
    }
}
