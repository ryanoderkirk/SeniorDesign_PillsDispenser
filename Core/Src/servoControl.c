/* servoControl.c
 *
 * Implementation for servoControl
 */

#include "servoControl.h"
#include <stdint.h>

int runContinuousServo(TIM_HandleTypeDef timer, uint32_t tim_channel, int speed) {
  // degree to appropriate pwm
  // continuous: .5ms pulse (reverse fastest) -> 1.5ms pulse (no movement) -> 2.5ms pulse (forware fastest)
  // 640000 clock ticks per second -> 32000 ticks per 1ms
  // ticks = 16000 + 64000/100 ticks per degree
  // 16000 - 80000
  uint32_t pwm = 750 + speed * 5000 / 100;

  // reconfigure channel with new speed
  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = pwm;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, tim_channel) != HAL_OK) {
    Error_Handler();
  }
  HAL_TIM_PWM_Start(&timer, tim_channel);
  return -1;
}

int run180Servo(TIM_HandleTypeDef timer, uint32_t tim_channel,
                uint32_t degree) {
  // degree to appropriate pwm
  // 180 degree servo: .5ms pulse (0 degrees) -> 2.5ms pulse (180 degrees)
  // 640000 clock ticks per second -> 32000 ticks per 1ms
  // ticks = 16000 + 64000/180 ticks per degree
  uint32_t pwm = 250 + degree * 1000 / 180;
  /*if(degree == 180)
    pwm = 80000;
  if(degree == 0)
    pwm = 16000;*/
  // reconfigure channel with new pwm
  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = pwm;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, tim_channel) != HAL_OK) {
    Error_Handler();
  }
  HAL_TIM_PWM_Start(&timer, tim_channel);
  return -1;
}

int run270Servo(TIM_HandleTypeDef timer, uint32_t tim_channel, uint32_t degree) {
  // degree to appropriate pwm
  // 180 degree servo: .5ms pulse (0 degrees) -> 2.5ms pulse (180 degrees)
  // 640000 clock ticks per second -> 32000 ticks per 1ms
  // ticks = 16000 + 64000/270 ticks per degree
  uint32_t pwm = 250 + degree * 1000 / 270;
  // reconfigure channel with new pwm
  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = pwm;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, tim_channel) != HAL_OK) {
    Error_Handler();
  }
  HAL_TIM_PWM_Start(&timer, tim_channel);
  return 0;
}

int stopServo(TIM_HandleTypeDef timer, uint32_t tim_channel) {
  HAL_TIM_PWM_Stop(&timer, tim_channel);
  return 0;
}
