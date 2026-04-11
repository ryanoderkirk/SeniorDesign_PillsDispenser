/*
 *
 * Alert.c
 * Contains function definitions pertinent to Alert subsystem
 *
 */

#include "Alert.h"
#include "main.h"
#include "filesystem.h"
#include <stdint.h>

// hold alert States
dosage_alert_t dosageAlert = CLEAR;
uint8_t lowPillAlert = 0;

const uint8_t lowBar = 10;
uint8_t lowPillAlertTriggered = 0;

// check if a dosage needs an alert based on hours and minutes from rtc
// returns if the alert is active or cleared
dosage_alert_t checkDosageAlert(){
	Dosage_t doses[5];
	int num_doses = readDoses(doses,5);
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	get_rtc_typedef(&date, &time);

	for(int i = 0; i < num_doses; ++i){
		if(time.Hours == doses[i].hour && time.Minutes == doses[i].min){
			dosageAlert = ACTIVE;
			return dosageAlert;
		}
	}
	return dosageAlert;
}

// check if any pill channels are low
// returns which pill channels are low
uint8_t checkLowPillAlert(){
	for(int i = 1; i < 5;++i){
		Config_t config;
		int error = readConfig(&config,i);
		if(error != 0){
			continue;
		}
		if(config.pillCount < lowBar){
			if(lowPillAlertTriggered & (1 << (i-1)) == 0){
				lowPillAlert = lowPillAlert | (1 << (i-1));
				lowPillAlertTriggered = lowPillAlertTriggered | (1 << (i-1));
			}
		}
	}
	return lowPillAlert;
}

void clearLowPillAlert(uint8_t channel){
	lowPillAlertTriggered = lowPillAlertTriggered & ~(1 << (channel - 1));
}
