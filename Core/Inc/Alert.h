/*
 * 	Alert.h
 * 	Contains function and variable declarations pertinent to the alert system
 *
 */

#ifndef ALERT_H
#define ALERT_H


#include <stdint.h>

typedef enum dosage_alert{
	CLEAR,
	ACTIVE
}dosage_alert_t;

typedef enum low_pill_alert{
	off = 0,
	channel1 = 1,
	channel2 = 2,
	channel3 = 4,
	channel4 = 8
}low_pill_alert_mask_t;

extern dosage_alert_t dosageAlert;
extern uint8_t lowPillAlert;

dosage_alert_t checkDosageAlert();

uint8_t checkLowPillAlert();

#endif
