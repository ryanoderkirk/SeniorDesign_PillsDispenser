/*
 * buttons.c
 *
 *  Created on: Nov 4, 2025
 *      Author: cmb
 */
#include "buttons.h"
#include "main.h"
#include "logging.h"

volatile unsigned short selectPressed;
volatile unsigned short backPressed;
volatile encMovement_t encoderState;

void selectButtonCallback(){
	selectPressed = 1;
	LogDebug("SELECT pressed\n");
}

void backButtonCallback(){
	backPressed = 1;
    LogDebug("BACK pressed\n");
}

// called on rising A transition
// Assume A is high
void ENCACallback(){
	// check to see if B is already high
	if(readB){
		encoderState = CW;
    	LogDebug("CW Triggered\n");
	}
	else{
		encoderState = CCW;
    	LogDebug("CCW Triggered\n");
	}
	for(int i = 0; i < 1000; i++){
		for(int j = 0; j < 100; j++){

		}
	}
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

