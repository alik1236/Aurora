/**
  ******************************************************************************
  * @file    arm.h
  * @brief   机械臂舵机控制接口
  *
  *  舵机分配（50Hz，1 计数 = 1us，500=0° 1500=90° 2500=180°）：
  *    TIM4_CH1(PB6) 关节 1    TIM4_CH2(PB7) 关节 2
  *    TIM4_CH3(PB8) 转台      TIM4_CH4(PB9) 铲斗
  *    TIM2_CH1(PA0) 夹爪      TIM1_CH1~3(PA8/9/10) 备用（保持 90°）
  ******************************************************************************
  */
#ifndef __ARM_H
#define __ARM_H

#include "ps2.h"

#define ARM_PULSE_MIN       500
#define ARM_PULSE_MAX       2500
#define ARM_PULSE_MID       1500

#define ARM_STEP_DEG        5          /* 每次按键步进角度 */
#define ARM_REPEAT_MS       100U       /* 按住时重复间隔 */
#define ARM_TURNTABLE_MIN   15         /* 转台限位 */
#define ARM_TURNTABLE_MAX   165
#define ARM_BUCKET_MAX      90         /* 铲斗限位 */

void Arm_Init(void);
void Arm_Update(const PS2_Data_t *pad);

#endif /* __ARM_H */
