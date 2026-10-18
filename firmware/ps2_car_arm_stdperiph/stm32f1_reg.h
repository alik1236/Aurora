/**
  ******************************************************************************
  * @file    stm32f1_reg.h
  * @brief   STM32F103C8T6 寄存器定义与基础操作（标准库/寄存器直操作版）
  *
  *  本头文件提供全工程共用的寄存器基址宏、GPIO/定时器位操作辅助函数，
  *  不依赖 HAL / CubeMX / CMSIS 头文件。
  *
  *  时钟约定：HSI 8MHz + PLL x16 = 64MHz（不依赖外部晶振）
  *  定时器时钟：APB1 预分频 /2 -> TIM2/3/4 计数时钟 = 64MHz
  *            APB2 直连    -> TIM1 计数时钟 = 64MHz
  *  统一用 PSC=63 -> 计数频率 1MHz，1 个计数 = 1us
  ******************************************************************************
  */
#ifndef __STM32F1_REG_H
#define __STM32F1_REG_H

#include <stdint.h>

/* ================= 外设基址 ================= */
#define RCC_BASE      0x40021000UL
#define GPIOA_BASE    0x40010800UL
#define GPIOB_BASE    0x40010C00UL
#define GPIOC_BASE    0x40011000UL
#define AFIO_BASE     0x40010000UL
#define FLASH_BASE    0x40022000UL
#define TIM1_BASE     0x40012C00UL
#define TIM2_BASE     0x40000000UL
#define TIM3_BASE     0x40000400UL
#define TIM4_BASE     0x40000800UL

/* ================= 内核外设基址 ================= */
#define SYSTICK_BASE  0xE000E010UL
#define DWT_BASE      0xE0001000UL
#define DEMCR_ADDR    0xE000EDFCUL

#define REG(addr)     (*(volatile uint32_t *)(addr))

/* ================= RCC 寄存器 ================= */
#define RCC_CR      REG(RCC_BASE + 0x00)
#define RCC_CFGR    REG(RCC_BASE + 0x04)
#define RCC_APB2ENR REG(RCC_BASE + 0x18)
#define RCC_APB1ENR REG(RCC_BASE + 0x1C)

/* ================= GPIO 寄存器 ================= */
#define GPIO_CRL(g)  REG((g) + 0x00)
#define GPIO_CRH(g)  REG((g) + 0x04)
#define GPIO_IDR(g)  REG((g) + 0x08)
#define GPIO_ODR(g)  REG((g) + 0x0C)
#define GPIO_BSRR(g) REG((g) + 0x10)

/* ================= 定时器寄存器（偏移） ================= */
#define TIM_CR1(t)   REG((t) + 0x00)
#define TIM_EGR(t)   REG((t) + 0x14)
#define TIM_CCMR1(t) REG((t) + 0x18)
#define TIM_CCMR2(t) REG((t) + 0x1C)
#define TIM_CCER(t)  REG((t) + 0x20)
#define TIM_CNT(t)   REG((t) + 0x24)
#define TIM_PSC(t)   REG((t) + 0x28)
#define TIM_ARR(t)   REG((t) + 0x2C)
#define TIM_CCR1(t)  REG((t) + 0x34)
#define TIM_CCR2(t)  REG((t) + 0x38)
#define TIM_CCR3(t)  REG((t) + 0x3C)
#define TIM_CCR4(t)  REG((t) + 0x40)
#define TIM_BDTR(t)  REG((t) + 0x44)

/* ================= SysTick 寄存器 ================= */
#define SYSTICK_CTRL REG(SYSTICK_BASE + 0x00)
#define SYSTICK_LOAD REG(SYSTICK_BASE + 0x04)
#define SYSTICK_VAL  REG(SYSTICK_BASE + 0x08)

/* ==========================================================
 * GPIO 配置辅助
 *   mode4: 0x3 = 推挽输出 50MHz
 *          0xB = 复用推挽 50MHz
 *          0x8 = 输入（配合 ODR 位=1 即上拉）
 * ========================================================== */
static inline void gpio_set(uint32_t port, uint32_t pin, uint32_t mode4)
{
    uint32_t cr  = (pin < 8) ? GPIO_CRL(port) : GPIO_CRH(port);
    uint32_t sh  = (pin % 8) * 4;

    if (pin < 8)
    {
        GPIO_CRL(port) = (cr & ~(0xFUL << sh)) | (mode4 << sh);
    }
    else
    {
        GPIO_CRH(port) = (cr & ~(0xFUL << sh)) | (mode4 << sh);
    }
}

static inline void gpio_set_pin(uint32_t port, uint32_t pin)  { GPIO_BSRR(port) = (1UL << pin); }
static inline void gpio_clr_pin(uint32_t port, uint32_t pin)  { GPIO_BSRR(port) = (1UL << (pin + 16)); }
static inline uint32_t gpio_read_pin(uint32_t port, uint32_t pin) { return (GPIO_IDR(port) >> pin) & 1UL; }

/* ==========================================================
 * 定时器 PWM 辅助：初始化某定时器单通道 PWM（PWM1 模式）
 *   psc/arr 由调用方给定；同一定时器多通道可多次调用（位或累加）
 *   is_tim1 = 1 时使能 TIM1 的 MOE 主输出
 * ========================================================== */
static inline void tim_pwm_init(uint32_t tim, uint32_t ch,
                                uint32_t psc, uint32_t arr, uint32_t is_tim1)
{
    TIM_PSC(tim) = psc;
    TIM_ARR(tim) = arr;

    if (ch == 1)
    {
        TIM_CCMR1(tim) |= (0x6UL << 4)  | (1UL << 3);   /* OC1M=110 PWM1, OC1PE */
    }
    else if (ch == 2)
    {
        TIM_CCMR1(tim) |= (0x6UL << 12) | (1UL << 11);  /* OC2M=110, OC2PE */
    }
    else if (ch == 3)
    {
        TIM_CCMR2(tim) |= (0x6UL << 4)  | (1UL << 3);   /* OC3M=110, OC3PE */
    }
    else
    {
        TIM_CCMR2(tim) |= (0x6UL << 12) | (1UL << 11);  /* OC4M=110, OC4PE */
    }

    TIM_CCER(tim) |= (1UL << ((ch - 1) * 4));            /* CCxE 使能 */
    TIM_EGR(tim)  |= 1UL;                                /* UG 重载 */
    if (is_tim1)
    {
        TIM_BDTR(tim) |= (1UL << 15);                    /* TIM1 MOE 主输出 */
    }
    TIM_CR1(tim)  |= 1UL;                                /* CEN 启动 */
}

/* ==========================================================
 * 系统时间（由 main.c 提供）：
 *   g_sys_tick_ms 由 SysTick 1ms 中断累加
 * ========================================================== */
extern volatile uint32_t g_sys_tick_ms;
static inline uint32_t get_tick(void) { return g_sys_tick_ms; }
static inline void delay_ms(uint32_t ms)
{
    uint32_t start = g_sys_tick_ms;
    while ((g_sys_tick_ms - start) < ms) { }
}

#endif /* __STM32F1_REG_H */
