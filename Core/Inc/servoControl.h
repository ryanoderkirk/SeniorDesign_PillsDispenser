/* servoControl.h 
 *
 * Initialize and control continuous and 180 degree servos
*/

#ifndef SERVOCONTROLX_H
#define SERVOCONTROLX_H

#include "stm32h5xx_hal.h"

int runContinuousServo(TIM_HandleTypeDef timer, uint32_t tim_channel, int speed);
int run180Servo(TIM_HandleTypeDef timer, uint32_t tim_channel, uint32_t degree);
int run270Servo(TIM_HandleTypeDef timer, uint32_t tim_channel, uint32_t degree);
int stopServo(TIM_HandleTypeDef timer, uint32_t tim_channel);

#endif
