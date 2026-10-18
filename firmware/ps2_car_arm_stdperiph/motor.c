/**
  ******************************************************************************
  * @file    motor.c
  * @brief   底盘电机驱动（L298N 双 PWM，TIM3 四路，寄存器直操作）
  *
  *  接法（两片 L298N，IN 脚调速+方向，ENA/ENB 跳线帽常高）：
  *    左电机 : IN1、IN3 并联 -> PA6(TIM3_CH1)   IN2、IN4 并联 -> PA7(TIM3_CH2)
  *    右电机 : IN1、IN3 并联 -> PB0(TIM3_CH3)   IN2、IN4 并联 -> PB1(TIM3_CH4)
  *
  *  调速逻辑（speed -1000~+1000）：
  *    前进 : PWM_A = |speed|, PWM_B = 0
  *    后退 : PWM_A = 0,       PWM_B = |speed|
  *    停止 : 两路都输出高电平 -> 短路刹车（坡上不溜车）
  ******************************************************************************
  */

#include "motor.h"
#include "stm32f1_reg.h"

static void set_channel_pair(uint32_t pwm_reg_a, uint32_t pwm_reg_b, int16_t speed)
{
    uint32_t mag;

    if (speed > MOTOR_MAX_SPEED)
    {
        speed = MOTOR_MAX_SPEED;
    }
    else if (speed < -MOTOR_MAX_SPEED)
    {
        speed = -MOTOR_MAX_SPEED;
    }

    if (speed == 0)
    {
        /* 刹车：两路全高（L298N 同侧全高 = 短路制动） */
        REG(pwm_reg_a) = MOTOR_PWM_ARR;
        REG(pwm_reg_b) = MOTOR_PWM_ARR;
        return;
    }

    if (speed > 0)
    {
        mag = (uint32_t)speed;
        REG(pwm_reg_a) = (mag * MOTOR_PWM_ARR) / MOTOR_MAX_SPEED;
        REG(pwm_reg_b) = 0U;
    }
    else
    {
        mag = (uint32_t)(-speed);
        REG(pwm_reg_a) = 0U;
        REG(pwm_reg_b) = (mag * MOTOR_PWM_ARR) / MOTOR_MAX_SPEED;
    }
}

void Motor_Init(void)
{
    /* 初始：两路全高刹车，防止上电即窜车 */
    TIM_CCR1(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR2(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR3(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR4(TIM3_BASE) = MOTOR_PWM_ARR;
}

void Motor_Stop(void)
{
    TIM_CCR1(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR2(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR3(TIM3_BASE) = MOTOR_PWM_ARR;
    TIM_CCR4(TIM3_BASE) = MOTOR_PWM_ARR;
}

void Motor_Drive(int16_t left, int16_t right)
{
    /* TIM_CCR1/CCR2 = 左电机双 PWM，TIM_CCR3/CCR4 = 右电机双 PWM */
    set_channel_pair(TIM3_BASE + 0x34UL, TIM3_BASE + 0x38UL, left);
    set_channel_pair(TIM3_BASE + 0x3CUL, TIM3_BASE + 0x40UL, right);
}
