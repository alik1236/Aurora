/**
  ******************************************************************************
  * @file    car.c
  * @brief   底盘运动控制（左摇杆差速混控 + 速度斜坡 + 失联保护 + LED）
  *
  *  逻辑（与原 HAL 版一致）：
  *    左摇杆 Y  -> 油门（上推前进）
  *    左摇杆 X  -> 转向（右推右转，可原地转）
  *    差速输出 :  left = throttle + steer；right = throttle - steer
  *    死区 25，斜坡 step=100/10ms，幅值 ±1000
  *    失联 >200ms 自动刹车，LED 快闪；在线 LED 常亮
  *
  *  LED 接 PC13，低电平点亮（板载 LED）
  ******************************************************************************
  */

#include "car.h"
#include "motor.h"
#include "stm32f1_reg.h"

#define CAR_DEADZONE       25
#define CAR_RAMP_STEP      100
#define CAR_LOST_TIMEOUT   200U    /* ms，超过即认为失联 */

static int16_t s_out_left  = 0;
static int16_t s_out_right = 0;
static uint32_t s_last_good = 0U;

/* 有符号映射：0~255 -> -1000~+1000（128 为 0） */
static int16_t axis_to_speed(uint8_t v)
{
    int16_t s = (int16_t)v - 128;
    int16_t speed;

    if ((s > -CAR_DEADZONE) && (s < CAR_DEADZONE))
    {
        return 0;
    }

    speed = (int16_t)((int32_t)s * MOTOR_MAX_SPEED / 128);
    if (speed > MOTOR_MAX_SPEED)
    {
        speed = MOTOR_MAX_SPEED;
    }
    else if (speed < -MOTOR_MAX_SPEED)
    {
        speed = -MOTOR_MAX_SPEED;
    }

    return speed;
}

/* 斜坡限幅：每 10ms 最多变化 CAR_RAMP_STEP */
static int16_t ramp_limit(int16_t cur, int16_t target)
{
    if (target > cur + CAR_RAMP_STEP)
    {
        return cur + CAR_RAMP_STEP;
    }
    if (target < cur - CAR_RAMP_STEP)
    {
        return cur - CAR_RAMP_STEP;
    }

    return target;
}

static void led_on(void)   { gpio_clr_pin(GPIOC_BASE, 13); }   /* 低电平点亮 */
static void led_off(void)  { gpio_set_pin(GPIOC_BASE, 13); }

void Car_Init(void)
{
    s_out_left  = 0;
    s_out_right = 0;
    s_last_good = get_tick();
    led_off();
    Motor_Stop();
}

void Car_Update(const PS2_Data_t *pad)
{
    static uint32_t s_last_flash = 0U;
    static uint8_t  s_flash_state = 0U;

    if ((pad == NULL) || (pad->ok == 0U))
    {
        /* 失联：超时刹车 + LED 快闪 */
        if ((get_tick() - s_last_good) >= CAR_LOST_TIMEOUT)
        {
            s_out_left  = 0;
            s_out_right = 0;
            Motor_Stop();
        }

        if ((get_tick() - s_last_flash) >= 100U)
        {
            s_last_flash = get_tick();
            s_flash_state = (uint8_t)(1U - s_flash_state);
            if (s_flash_state)
            {
                led_on();
            }
            else
            {
                led_off();
            }
        }

        return;
    }

    s_last_good = get_tick();
    led_on();                             /* 在线常亮 */

    {
        int16_t throttle = axis_to_speed((uint8_t)(255U - pad->ly));  /* 上推为正 */
        int16_t steer    = axis_to_speed(pad->lx);                    /* 右推为正 */
        int16_t target_l = throttle + steer;
        int16_t target_r = throttle - steer;

        s_out_left  = ramp_limit(s_out_left, target_l);
        s_out_right = ramp_limit(s_out_right, target_r);

        Motor_Drive(s_out_left, s_out_right);
    }
}
