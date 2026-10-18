/**
  ******************************************************************************
  * @file    car.h
  * @brief   底盘运动控制接口
  ******************************************************************************
  */
#ifndef __CAR_H
#define __CAR_H

#include "ps2.h"

void Car_Init(void);
void Car_Update(const PS2_Data_t *pad);

#endif /* __CAR_H */
