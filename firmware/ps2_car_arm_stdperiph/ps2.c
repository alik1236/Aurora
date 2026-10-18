/**
  ******************************************************************************
  * @file    ps2.c
  * @brief   PS2 手柄驱动（软件模拟 SPI，9 字节轮询，寄存器直操作）
  *
  *  时序：空闲 CS/CLK/CMD 全高，CS 拉低 -> 9 字节 -> CS 拉高
  *        时钟下降沿数据变化，上升沿采样（SPI Mode 3），LSB 在前
  *
  *  9 字节请求：01 42 00 00 00 00 00 00 00
  *  典型应答  ：FF 73 5A B1 B2 RX RY LX LY
  *
  *  微秒延时：DWT 周期计数器（64MHz，由 main.c 初始化）
  ******************************************************************************
  */

#include "ps2.h"
#include "stm32f1_reg.h"

/* ----------------------------- 电平操作 ----------------------------- */
#define ATT_LOW()   gpio_clr_pin(PS2_ATT_PORT, PS2_ATT_PIN)
#define ATT_HIGH()  gpio_set_pin(PS2_ATT_PORT, PS2_ATT_PIN)
#define CLK_LOW()   gpio_clr_pin(PS2_CLK_PORT, PS2_CLK_PIN)
#define CLK_HIGH()  gpio_set_pin(PS2_CLK_PORT, PS2_CLK_PIN)
#define CMD_LOW()   gpio_clr_pin(PS2_CMD_PORT, PS2_CMD_PIN)
#define CMD_HIGH()  gpio_set_pin(PS2_CMD_PORT, PS2_CMD_PIN)
#define DAT_READ()  gpio_read_pin(PS2_DAT_PORT, PS2_DAT_PIN)

/* 时钟半周期，单位 us；PS2 最高约 250kHz */
#define PS2_HALF_PERIOD_US   2U

/* ------------------------ 初始化用的命令 ------------------------ */
static const uint8_t s_cfg_enter[9] = {0x01, 0x43, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t s_cfg_mode[9]  = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
static const uint8_t s_cfg_exit[9]  = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};

/* --------------------------- 微秒延时（DWT，64MHz） --------------------------- */
static void ps2_delay_us(uint32_t us)
{
    uint32_t start;

    if (us == 0U)
    {
        return;
    }

    start = REG(DWT_BASE + 0x04);
    while ((REG(DWT_BASE + 0x04) - start) < (us * 64UL))
    {
        __asm volatile("nop");
    }
}

/* --------------------------- 单字节收发 --------------------------- */
static uint8_t ps2_transfer(uint8_t cmd)
{
    uint8_t data = 0U;
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        if ((cmd & 0x01U) != 0U)
        {
            CMD_HIGH();
        }
        else
        {
            CMD_LOW();
        }
        cmd >>= 1;

        ps2_delay_us(PS2_HALF_PERIOD_US);
        CLK_LOW();
        ps2_delay_us(PS2_HALF_PERIOD_US);

        CLK_HIGH();
        ps2_delay_us(1U);

        data >>= 1;
        if (DAT_READ())
        {
            data |= 0x80U;
        }
    }

    ps2_delay_us(4U);

    return data;
}

/* --------------------------- 一次完整传输 --------------------------- */
static void ps2_send(const uint8_t *tx, uint8_t *rx, uint8_t len)
{
    uint8_t i;

    ATT_LOW();
    ps2_delay_us(10U);

    for (i = 0U; i < len; i++)
    {
        rx[i] = ps2_transfer(tx[i]);
    }

    ps2_delay_us(5U);
    ATT_HIGH();
    ps2_delay_us(30U);
}

/* --------------------------- 对外接口 --------------------------- */
void PS2_Init(void)
{
    PS2_Data_t tmp;
    uint8_t rx[9];
    uint8_t retry;

    ATT_HIGH();
    CLK_HIGH();
    CMD_HIGH();

    delay_ms(300U);

    for (retry = 0U; retry < 3U; retry++)
    {
        ps2_send(s_cfg_enter, rx, 9U);
        delay_ms(20U);

        ps2_send(s_cfg_mode, rx, 9U);
        delay_ms(20U);

        ps2_send(s_cfg_exit, rx, 9U);
        delay_ms(30U);

        if (PS2_Poll(&tmp) != 0U)
        {
            break;
        }

        delay_ms(100U);
    }
}

uint8_t PS2_Poll(PS2_Data_t *out)
{
    static const uint8_t tx[9] = {0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx[9];
    uint16_t raw;

    if (out == NULL)
    {
        return 0U;
    }

    out->ok = 0U;

    ps2_send(tx, rx, 9U);

    if ((rx[1] == 0xFFU) || (rx[1] == 0x00U))
    {
        return 0U;
    }

    out->type = rx[1];

    raw = (uint16_t)rx[3] | ((uint16_t)rx[4] << 8);
    out->buttons = (uint16_t)(~raw);

    out->rx = rx[5];
    out->ry = rx[6];
    out->lx = rx[7];
    out->ly = rx[8];

    if (out->type == 0x41U)
    {
        /* 数字模式没有摇杆数据，强制回中 */
        out->rx = 0x80U;
        out->ry = 0x80U;
        out->lx = 0x80U;
        out->ly = 0x80U;
    }

    out->ok = 1U;
    return 1U;
}

uint8_t PS2_IsPressed(const PS2_Data_t *pad, uint16_t mask)
{
    if ((pad == NULL) || (pad->ok == 0U))
    {
        return 0U;
    }

    return ((pad->buttons & mask) != 0U) ? 1U : 0U;
}
