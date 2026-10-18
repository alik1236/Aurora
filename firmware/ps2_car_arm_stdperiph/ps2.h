/**
  ******************************************************************************
  * @file    ps2.h
  * @brief   PS2 手柄驱动（软件模拟 SPI，9 字节轮询）
  * @note    引脚按原工程文档固定为 PB12~PB15，无 ACK 引脚
  ******************************************************************************
  */
#ifndef __PS2_H
#define __PS2_H

#include <stdint.h>
#include <stddef.h>

/* ============================== 引脚定义 ==============================
 * 与原文档保持一致：
 *   DAT = PB12（输入，上拉）
 *   CMD = PB13（输出）
 *   CS  = PB14（输出，即 ATT）
 *   CLK = PB15（输出）
 * 原接收器没有 ACK 脚，所以 PS2_USE_ACK 设为 0。
 */
#define PS2_DAT_PORT    GPIOB_BASE
#define PS2_DAT_PIN     12

#define PS2_CMD_PORT    GPIOB_BASE
#define PS2_CMD_PIN     13

#define PS2_ATT_PORT    GPIOB_BASE
#define PS2_ATT_PIN     14     /* 文档里叫 CS */

#define PS2_CLK_PORT    GPIOB_BASE
#define PS2_CLK_PIN     15

#define PS2_USE_ACK     0      /* 1 = 接 ACK，0 = 不接 */

/* ============================== 按键位定义 ==============================
 * PS2_Poll() 返回的 buttons 中，某一位 = 1 表示该键按下
 */
#define PS2_BTN_SELECT      0x0001U
#define PS2_BTN_L3          0x0002U
#define PS2_BTN_R3          0x0004U
#define PS2_BTN_START       0x0008U
#define PS2_BTN_UP          0x0010U
#define PS2_BTN_RIGHT       0x0020U
#define PS2_BTN_DOWN        0x0040U
#define PS2_BTN_LEFT        0x0080U
#define PS2_BTN_L2          0x0100U
#define PS2_BTN_R2          0x0200U
#define PS2_BTN_L1          0x0400U
#define PS2_BTN_R1          0x0800U
#define PS2_BTN_TRIANGLE    0x1000U
#define PS2_BTN_CIRCLE      0x2000U
#define PS2_BTN_CROSS       0x4000U
#define PS2_BTN_SQUARE      0x8000U

/* ============================== 数据结构 ============================== */
typedef struct
{
    uint16_t buttons;   /* 按键状态，见 PS2_BTN_xxx，1 = 按下 */
    uint8_t  lx;        /* 左摇杆 X：0x00 最左，0x80 居中，0xFF 最右 */
    uint8_t  ly;        /* 左摇杆 Y：0x00 最上，0x80 居中，0xFF 最下 */
    uint8_t  rx;        /* 右摇杆 X */
    uint8_t  ry;        /* 右摇杆 Y */
    uint8_t  type;      /* 手柄返回 ID：0x41 数字模式，0x73/0x79 模拟模式 */
    uint8_t  ok;        /* 1 = 本次数据有效 */
} PS2_Data_t;

void    PS2_Init(void);
uint8_t PS2_Poll(PS2_Data_t *out);
uint8_t PS2_IsPressed(const PS2_Data_t *pad, uint16_t mask);

#endif /* __PS2_H */
