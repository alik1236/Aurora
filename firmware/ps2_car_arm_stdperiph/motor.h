/**
  ******************************************************************************
  * @file    motor.h
  * @brief   底盘电机接口（L298N 双 PWM，TIM3 四路）
  ******************************************************************************
  */
#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

#define MOTOR_MAX_SPEED     1000    /* 最大占空比基数 */
#define MOTOR_PWM_ARR       999     /* TIM3 ARR（1kHz） */

void Motor_Init(void);
void Motor_Stop(void);
void Motor_Drive(int16_t left, int16_t right);   /* -1000 ~ +1000 */

#endif /* __MOTOR_H */
