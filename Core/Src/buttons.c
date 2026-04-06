/*
 * buttons.c
 *
 *  Created on: Nov 4, 2025
 *      Author: cmb
 */
#include "buttons.h"

volatile unsigned short selectPressed;
volatile unsigned short backPressed;
volatile encMovement_t encoderState;

void selectButtonCallback(){
	selectPressed = 1;
}

void backButtonCallback(){
	backPressed = 1;
}

// called on rising A transition
// Assume A is high
void ENCACallback(){
	// check to see if B is already high
	if(readB)
		encoderState = CW;
	else
		encoderState = CCW;
}

// called on rising B transition
// Assume B is high
void ENCBCallback(){
	// check to see if A is already high
	if(readA)
		encoderState = CW;
	else
		encoderState = CCW;
}

