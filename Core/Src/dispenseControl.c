/* dispenseControl.c
 *
 * Implementation for dispenseControl
 */

#include "dispenseControl.h"
#include <string.h>
#include "main.h"
#include "stm32h5xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include "logging.h"

static uint32_t GATE_CHANNEL = TIM_CHANNEL_1;

int dispensePills(uint32_t channel, uint32_t numberPills) {
  // ensure PWM timer and ADC are enabled and functioning
  // configure timer
  TIM_HandleTypeDef* htimDispenser = Get_PWM_Dispense_Handle();
  uint32_t ADC_channel = -1;
  // toal pill counter
  uint8_t totalPills = 0;


  switch (channel) {
    case 1:
      ADC_channel = ADC_CHANNEL_13;
      break;
    case 2:
      ADC_channel = ADC_CHANNEL_12;
      break;
    case 3:
      ADC_channel = ADC_CHANNEL_0;
      break;
    case 4:
      ADC_channel = ADC_CHANNEL_1;
      break;
    default:
      return -1;
  }

  uint32_t TIM_channel = -1;
  switch (channel) {
    case 1:
      TIM_channel = TIM_CHANNEL_1;
      break;
    case 2:
      TIM_channel = TIM_CHANNEL_2;
      break;
    case 3:
      TIM_channel = TIM_CHANNEL_3;
      break;
    case 4:
      TIM_channel = TIM_CHANNEL_4;
      break;
    default:
      return -1;
  }

  // Configure ADC to listen to channel specified
  ADC_HandleTypeDef* hadc1 = Get_ADC_Handle();
  ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};
  ADC_ChannelConfTypeDef sConfig = {0};
  AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
  AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
  AnalogWDGConfig.Channel = ADC_channel;
  AnalogWDGConfig.ITMode = ENABLE;
  AnalogWDGConfig.HighThreshold = 4000;
  AnalogWDGConfig.LowThreshold = 620;
  AnalogWDGConfig.FilteringConfig = ADC_AWD_FILTERING_8SAMPLES;
  if (HAL_ADC_AnalogWDGConfig(hadc1, &AnalogWDGConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfig.Channel = ADC_channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  // Timer 3 needs to be started to trigger ADC conversions
  HAL_TIM_Base_Start(Get_ADC_TIM_Handle());
  HAL_ADC_Start_IT(hadc1);
  char *dispensemsg = "Entering Dispense Logic\r\n";
  char *detectedMessage = "Pill detected\r\n";

  HAL_UART_Transmit(Get_DEBUG_Handle(), (uint8_t*)dispensemsg, strlen(dispensemsg),100);

//  HAL_UART_Transmit(Get_DEBUG_Handle(), &totalPills, 1 ,100);

  //JOSUE DISPENSE LOGIC
while (totalPills < numberPills)
  {

    resetPillFlag();
    //numberPills--;

    // Pills dispensing movement: detect
    // Start at 0 position

    // Try 1st pill slot: wait for feedback
    run270Servo(*htimDispenser, TIM_channel, 155);
    HAL_Delay(1500);

    run270Servo(*htimDispenser, TIM_channel, 140);
    HAL_Delay(250);
    run270Servo(*htimDispenser, TIM_channel, 170);
    HAL_Delay(250);

    // Try 2nd pill slot: wait for feedback
    run270Servo(*htimDispenser, TIM_channel, 95);
    HAL_Delay(1500);
    run270Servo(*htimDispenser, TIM_channel, 80);
    HAL_Delay(250);
    run270Servo(*htimDispenser, TIM_channel, 110);
    HAL_Delay(250);

    // Try 3rd pill slot: wait for feedback

    run270Servo(*htimDispenser, TIM_channel, 45);
    HAL_Delay(1000);

    run270Servo(*htimDispenser, TIM_channel, 30);
    HAL_Delay(250);
    run270Servo(*htimDispenser, TIM_channel, 60);
    HAL_Delay(250);

    run270Servo(*htimDispenser, TIM_channel, 3);
    HAL_Delay(1000);

    run270Servo(*htimDispenser, TIM_channel, 60);
    HAL_Delay(1000);

    resetPillFlag();
    run270Servo(*htimDispenser, TIM_channel, 180);
    HAL_Delay(1000);
    if(isPillDetected())
    {
 totalPills++;
 char msgBuffer[75];
 snprintf(msgBuffer, sizeof(msgBuffer), "Pill detected! Channel: %lu | Total Dispensed: %u\r\n", channel, totalPills);
    HAL_UART_Transmit(Get_DEBUG_Handle(), (uint8_t*)msgBuffer, strlen(msgBuffer), 100);

    if (totalPills == numberPills)
    		{
                  break;
              }

    }

    resetPillFlag();
    run270Servo(*htimDispenser, TIM_channel, 225);
    HAL_Delay(1000);
    if(isPillDetected())
    {
  totalPills++;
  char msgBuffer[75];
  snprintf(msgBuffer, sizeof(msgBuffer), "Pill detected! Channel: %lu | Total Dispensed: %u\r\n", channel, totalPills);

    HAL_UART_Transmit(Get_DEBUG_Handle(), (uint8_t*)msgBuffer, strlen(msgBuffer), 100);

    if (totalPills == numberPills)
    		{
                  break;
              }
    }
    resetPillFlag();
    run270Servo(*htimDispenser, TIM_channel, 260);
    HAL_Delay(1000);
    if(isPillDetected())
    {
 totalPills++;
 char msgBuffer[75];
 snprintf(msgBuffer, sizeof(msgBuffer), "Pill detected! Channel: %lu | Total Dispensed: %u\r\n", channel, totalPills);

      HAL_UART_Transmit(Get_DEBUG_Handle(), (uint8_t*)msgBuffer, strlen(msgBuffer), 100);

      if (totalPills == numberPills)
      {
                    break;
                }
    }

  }


  // Turn off ADC and timer 3 (which triggers conversions)
  HAL_TIM_Base_Stop(Get_ADC_TIM_Handle());
  HAL_ADC_Stop_IT(hadc1);

  // stop PWM timers
  HAL_TIM_PWM_Stop(htimDispenser, TIM_channel);
  HAL_Delay(1500);



return -1;
}


int dispenseDosage(const Dosage_t* dosage) {
  // ensure that the pills specified in the dosage exist in a channel, and that there are enough of them

  //map dose (index +1) to the channel its located in
  uint8_t doseChannel[4] = {0};
  //map dose (index +1) to the channel its located in
  uint8_t doseChannelCount[4] = {0};

  int pillExists = -1;
  if(dosage->pillOne[0] != '\0' ) {
    for (int i = 1; i < 5; i++) {
      Config_t channelConfig;
      int result = readConfig(&channelConfig, i);
      if (strcmp((char*)dosage->pillOne, (char*)channelConfig.pillName) == 0) {
        if (dosage->pillOneCount > channelConfig.pillCount) {
          LogDebug("Not enough pills in channel to dispense");
          return -1;
        }
        doseChannel[0] = i;
        doseChannelCount[0] = dosage->pillOneCount;
        pillExists = 0;
        break;
      }
    }
    if (pillExists != 0) {
      LogDebug("Could not find pill");
      return -1;
    }
  }

  pillExists = -1;
  if(dosage->pillTwo[0] != '\0' ) {
    for (int i = 1; i < 5; i++) {
      Config_t channelConfig;
      int result = readConfig(&channelConfig, i);
      if (strcmp((char*)dosage->pillTwo, (char*)channelConfig.pillName) == 0) {
        if (dosage->pillTwoCount > channelConfig.pillCount) {
          LogDebug("Not enough pills in channel to dispense");
          return -1;
        }
        doseChannel[1] = i;
        doseChannelCount[1] = dosage->pillTwoCount;
        pillExists = 0;
        break;
      }
    }
    if (pillExists != 0) {
      LogDebug("Could not find pill");
      return -2;
    }
  }

  pillExists = -1;
  if(dosage->pillThree[0] != '\0' ) {
    for (int i = 1; i < 5; i++) {
      Config_t channelConfig;
      int result = readConfig(&channelConfig, i);
      if (strcmp((char*)dosage->pillThree, (char*)channelConfig.pillName) == 0) {
        if (dosage->pillThreeCount > channelConfig.pillCount) {
          LogDebug("Not enough pills in channel to dispense");
          return -1;
        }
        doseChannel[2] = i;
        doseChannelCount[2] = dosage->pillThreeCount;
        pillExists = 0;
        break;
      }
    }
    if (pillExists != 0) {
      LogDebug("Could not find pill");
      return -1;
    }
  }

  pillExists = -1;
  if(dosage->pillFour[0] != '\0' ) {
    for (int i = 1; i < 5; i++) {
      Config_t channelConfig;
      int result = readConfig(&channelConfig, i);
      if (strcmp((char*)dosage->pillFour, (char*)channelConfig.pillName) == 0) {
        if (dosage->pillFourCount > channelConfig.pillCount) {
          LogDebug("Not enough pills in channel to dispense");
          return -1;
        }
        doseChannel[3] = i;
        doseChannelCount[3] = dosage->pillFourCount;
        pillExists = 0;
        break;
      }
    }
    if (pillExists != 0) {
      LogDebug("Could not find pill");
      return -1;
    }
  }

  for(int i = 0; i<4; i++) {
    if (doseChannel[i] == 0)
      continue;
    int result = 0;//dispensePills(doseChannel[i], doseChannelCount[i]);
    if (result != 0) {
      LogDebug("Failed to dispense pills");
      return -1;
    }
    //decrement pills picked
    Config_t config;
    if(readConfig(&config, doseChannel[i]) == 0) {
    	config.pillCount -= doseChannelCount[i];
    	writeConfig(&config);
    }
  }

  return 0;
}

int openGate() {
    TIM_HandleTypeDef* htimGate = Get_PWM_Gate_Handle();
    run270Servo(*htimGate, GATE_CHANNEL, 0);
    HAL_Delay(500);
    HAL_TIM_PWM_Stop(htimGate, GATE_CHANNEL);
    return 0;
}

int closeGate() {
    TIM_HandleTypeDef* htimGate = Get_PWM_Gate_Handle();
    run270Servo(*htimGate, GATE_CHANNEL, 90);
    HAL_Delay(500);
    HAL_TIM_PWM_Stop(htimGate, GATE_CHANNEL);
    return 0;
}
