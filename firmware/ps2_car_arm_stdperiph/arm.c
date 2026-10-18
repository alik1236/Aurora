/**
  ******************************************************************************
  * @file    arm.c
  * @brief   机械臂舵机控制（寄存器直操作，整数运算，无浮点依赖）
  *
  *  控制映射（与原 HAL 版一致）：
  *    △ / ○         关节 1 加 / 减
  *    × / □         关节 2 加 / 减
  *    L1 / R1       转台 减 / 加（限位 15~165°）
  *    十字键 上 / 下 铲斗 加 / 减（限位 0~90°）
  *    右摇杆 Y       夹爪连续（500us=闭合，2500us=张开）
  *
  *  角度 -> 脉宽：pulse = 500 + deg * 2000 / 180（整数）
  ******************************************************************************
  */

#include "arm.h"
#include "stm32f1_reg.h"

/* 舵机当前角度（0~180 度） */
static int16_t s_angle[4] = {90, 90, 90, 45};   /* 关节1 关节2 转台 铲斗 */
static uint16_t s_gripper = ARM_PULSE_MID;

static void servo_write(uint32_t tim, uint32_t ch, uint16_t pulse)
{
    if (pulse < ARM_PULSE_MIN)
    {
        pulse = ARM_PULSE_MIN;
    }
    else if (pulse > ARM_PULSE_MAX)
    {
        pulse = ARM_PULSE_MAX;
    }

    switch (ch)
    {
    case 1: TIM_CCR1(tim) = pulse; break;
    case 2: TIM_CCR2(tim) = pulse; break;
    case 3: TIM_CCR3(tim) = pulse; break;
    case 4: TIM_CCR4(tim) = pulse; break;
    default: break;
    }
}

/* 角度 -> 脉宽 us（1 计数 = 1us） */
static uint16_t angle_to_pulse(int16_t deg)
{
    if (deg < 0)
    {
        deg = 0;
    }
    else if (deg > 180)
    {
        deg = 180;
    }

    return (uint16_t)(ARM_PULSE_MIN + (int32_t)deg * 2000 / 180);
}

static void arm_flush(void)
{
    servo_write(TIM4_BASE, 1, angle_to_pulse(s_angle[0]));   /* 关节 1 PB6 */
    servo_write(TIM4_BASE, 2, angle_to_pulse(s_angle[1]));   /* 关节 2 PB7 */
    servo_write(TIM4_BASE, 3, angle_to_pulse(s_angle[2]));   /* 转台   PB8 */
    servo_write(TIM4_BASE, 4, angle_to_pulse(s_angle[3]));   /* 铲斗   PB9 */
    servo_write(TIM2_BASE, 1, s_gripper);                    /* 夹爪   PA0 */
}

void Arm_Init(void)
{
    /* 备用舵机（TIM1_CH1~3）保持 90° 中位 */
    servo_write(TIM1_BASE, 1, ARM_PULSE_MID);
    servo_write(TIM1_BASE, 2, ARM_PULSE_MID);
    servo_write(TIM1_BASE, 3, ARM_PULSE_MID);

    arm_flush();
}

void Arm_Update(const PS2_Data_t *pad)
{
    static uint16_t s_prev_btn = 0U;
    static uint32_t s_last_step = 0U;
    uint16_t now_btn;

    if ((pad == NULL) || (pad->ok == 0U))
    {
        return;                       /* 失联时机械臂保持最后位置 */
    }

    now_btn = pad->buttons;

    /* 夹爪：右摇杆 Y 连续控制（0x00 上 = 闭合 500us，0xFF 下 = 张开 2500us） */
    s_gripper = (uint16_t)(ARM_PULSE_MIN + (uint32_t)pad->ry * (ARM_PULSE_MAX - ARM_PULSE_MIN) / 255U);

    /* 边沿触发（按下瞬间步进一次），按住每 ARM_REPEAT_MS 重复步进 */
    if ((now_btn != s_prev_btn) || ((get_tick() - s_last_step) >= ARM_REPEAT_MS))
    {
        uint16_t edge = (uint16_t)(now_btn & ~s_prev_btn);

        if ((edge & PS2_BTN_TRIANGLE) || (now_btn & PS2_BTN_TRIANGLE))   /* △ 关节1 + */
        {
            s_angle[0] += ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_CIRCLE) || (now_btn & PS2_BTN_CIRCLE))       /* ○ 关节1 - */
        {
            s_angle[0] -= ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_CROSS) || (now_btn & PS2_BTN_CROSS))         /* × 关节2 + */
        {
            s_angle[1] += ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_SQUARE) || (now_btn & PS2_BTN_SQUARE))       /* □ 关节2 - */
        {
            s_angle[1] -= ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_R1) || (now_btn & PS2_BTN_R1))               /* R1 转台 + */
        {
            s_angle[2] += ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_L1) || (now_btn & PS2_BTN_L1))               /* L1 转台 - */
        {
            s_angle[2] -= ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_UP) || (now_btn & PS2_BTN_UP))               /* 十字上 铲斗 + */
        {
            s_angle[3] += ARM_STEP_DEG;
        }
        if ((edge & PS2_BTN_DOWN) || (now_btn & PS2_BTN_DOWN))           /* 十字下 铲斗 - */
        {
            s_angle[3] -= ARM_STEP_DEG;
        }

        /* 限位 */
        if (s_angle[0] > 180) { s_angle[0] = 180; }
        if (s_angle[0] < 0)   { s_angle[0] = 0; }
        if (s_angle[1] > 180) { s_angle[1] = 180; }
        if (s_angle[1] < 0)   { s_angle[1] = 0; }
        if (s_angle[2] > ARM_TURNTABLE_MAX) { s_angle[2] = ARM_TURNTABLE_MAX; }
        if (s_angle[2] < ARM_TURNTABLE_MIN) { s_angle[2] = ARM_TURNTABLE_MIN; }
        if (s_angle[3] > ARM_BUCKET_MAX)    { s_angle[3] = ARM_BUCKET_MAX; }
        if (s_angle[3] < 0)                 { s_angle[3] = 0; }

        s_last_step = get_tick();
        arm_flush();
    }

    s_prev_btn = now_btn;
}
