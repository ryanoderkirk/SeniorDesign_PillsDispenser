/*
 * UI.h
 *
 *  Created on: Jan 20, 2026
 *      Author: cmb
 */

#ifndef UI_H_
#define UI_H_

#include "stm32h5xx_hal.h"
#include "buttons.h"

typedef enum UISTATE{
	NOSTATE = -1,
	MAIN = 0,
	CONFIG = 1,
	SCHEDULE = 2,
	DISPENSE = 3,
	DOSAGEINFO = 4,
	VERIFYPIN,
	CHANGEPIN,
	SAVEPIN,
	DOSAGELIST,
	SHOWTIME,
	SHOWIP,
	ALERT,
}UISTATE_t;


typedef struct option{
	UISTATE_t next;
	char name[16];
}option_t;

typedef struct menuPage{
	char* name;
	UISTATE_t backState;
	unsigned short numOptions;
	option_t options[];
}menuPage_t;

extern UISTATE_t currentState;
extern UISTATE_t prevState;

typedef struct dosage{
	char name[8];
	unsigned short hour;
	unsigned short minute;
	unsigned short pillAmounts[4];
	char p1[5];
	char p2[5];
	char p3[5];
	char p4[5];
}dosage_ui_t;

/**
 * Initialize UI to known state (main)
 */
void UI_init();

/**
 * Handle changing between states
 * input: nextState - state to switch too
 */
void changeState(UISTATE_t nextState);

/**
 * Handle select input
 * To be called once per select button press
 */
void handleSelect();

/**
 * Handle back input
 * To be called once per back button press
 */
void handleBack();

/**
 * Handle Encoder input
 * Called once per encoder state change
 */
void handleEncoder(short dir);

/**
 * Redraws screen
 */
void drawScreen();

/**
 * Gets an array of dosages programmed
 */
dosage_ui_t* getDosages(unsigned short* numDosages);

/**
 * Handles all input
 */
void UI_handleInput();

#endif /* UI_H */
