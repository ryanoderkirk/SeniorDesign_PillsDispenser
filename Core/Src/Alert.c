/*
 *
 * Alert.c
 * Contains function definitions pertinent to Alert subsystem
 *
 */

#include "Alert.h"
#include "filesystem.h"
#include "main.h"
#include <stdint.h>

// hold alert States
volatile dosage_alert_t dosageAlert = CLEAR;
volatile uint8_t lowPillAlert = 0;

volatile uint8_t dosageAlertDose = 0;

const uint8_t lowBar = 10;
volatile uint8_t lowPillAlertTriggered = 0;

// check if a dosage needs an alert based on hours and minutes from rtc
// returns if the alert is active or cleared
dosage_alert_t checkDosageAlert() {
  Dosage_t doses[5];
  int num_doses = readDoses(doses, 5);
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  get_rtc_typedef(&date, &time);

  for (int i = 0; i < num_doses; ++i) {
    if (time.Hours == doses[i].hour && time.Minutes == doses[i].min) {
      dosageAlert = ACTIVE;
      dosageAlertDose = i;
      return dosageAlert;
    }
  }
  return dosageAlert;
}

uint8_t readDosageAlert(){
	return dosageAlert == ACTIVE;
}
uint8_t readDosageAlertChannel(uint8_t* channel){
	*channel = dosageAlertDose;
  return dosageAlert == ACTIVE;
}


void clearDosageAlert(){
	dosageAlert = 0;
}

// check if any pill channels are low
// returns which pill channels are low
uint8_t checkLowPillAlert() {
  for (int i = 1; i < 5; ++i) {
    Config_t config;
    int error = readConfig(&config, i);
    if (error != 0) {
      continue;
    }
    if (config.pillCount < lowBar) {
      if ((lowPillAlertTriggered & (1 << (i - 1))) == 0) {
        lowPillAlert = lowPillAlert | (1 << (i - 1));
        lowPillAlertTriggered = lowPillAlertTriggered | (1 << (i - 1));
      }
    }
  }
  return lowPillAlert;
}

uint8_t readLowPillAlert(uint8_t channel){
	return (lowPillAlert & (1 << (channel-1))) != 0;
}


void clearLowPillAlert(uint8_t channel) {
	  lowPillAlert = lowPillAlert & ~(1 << (channel - 1));
}

void resetLowPillAlert(uint8_t channel) {
  lowPillAlertTriggered = lowPillAlertTriggered & ~(1 << (channel - 1));
}
