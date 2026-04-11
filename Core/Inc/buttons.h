/*
 * buttons.h
 *
 *  Created on: Nov 4, 2025
 *      Author: cmb
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "stm32h5xx_hal.h"

#define SELECT_BUTTON_PORT 	GPIOF
#define SELECT_BUTTON_PIN 	GPIO_PIN_0

#define BACK_BUTTON_PORT 	GPIOF
#define BACK_BUTTON_PIN		GPIO_PIN_1

#define ENC_A_PORT			GPIOF
#define ENC_A_PIN			GPIO_PIN_2
#define ENC_B_PORT			GPIOF
#define ENC_B_PIN			GPIO_PIN_3

#define readA HAL_GPIO_ReadPin(ENC_A_PORT,ENC_A_PIN)
#define readB HAL_GPIO_ReadPin(ENC_B_PORT,ENC_B_PIN)


typedef enum encMovement{
	NONE,	// no movement
	CW,		// Clockwise Movement
	CCW		// Counter Clockwise Movement
}encMovement_t;

extern volatile unsigned short selectPressed;
extern volatile unsigned short backPressed;
extern volatile encMovement_t encoderState;


void selectButtonCallback();

void backButtonCallback();

void ENCACallback();

void ENCBCallback();


#endif /* BUTTONS_H_ */
