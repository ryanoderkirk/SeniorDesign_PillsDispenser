/*
 * UI.c
 *
 *  Created on: Jan 20, 2026
 *      Author: cmb
 */

#include "buttons.h"
#include "UI.h"
#include "main.h"
#include "filesystem.h"
#include "dispenseControl.h"
#include "Alert.h"
#include "logging.h"
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
#include "fonts.h"
#include <stdlib.h>
#include <stdio.h>

#define font3Height 20
#define initialOffset 30

const menuPage_t mainPage = {
		.name = "MAIN",
		.backState = MAIN,
		.numOptions = 3,
		.options = {
				{.next = CONFIG, .name = "CONFIG"},
				{.next = DOSAGELIST, .name = "DISPENSE"},
				{.next = DISPENSE_ALERT, .name = "TEST ALERT"}
		}
};

const menuPage_t configPage = {
		.name = "CONFIG",
		.backState = MAIN,
		.numOptions = 3,
		.options = {
				{.next = VERIFYPIN, .name = "CHANGE PIN"},
				{.next = SHOWTIME, .name = "SHOW TIME"},
				{.next = SHOWIP, .name = "SHOW IP ADDRESS"}
		}
};

const menuPage_t schedulePage = {
		.name = "SCHEDULE",
		.backState = MAIN,
		.numOptions = 3,
		.options = {
				{.next = CONFIG, .name = "CONFIG"},
				{.next = MAIN, .name = "MAIN"},
				{.next = DOSAGELIST, .name = "DISPENSE"}
		}
};

//const short numMenus = 4;
//const menuPage_t *menus[] = {&mainPage,&configPage,&schedulePage,&dispensePage};

enum UISTATE currentState = MAIN;
enum UISTATE prevState = NONE;

short currentSelection = 0;
short lastSelection = 0;


// holds dosage list
dosage_ui_t dosages[5];

dosage_ui_t* dosageList = dosages;
unsigned short numDosages = 0;
unsigned short selectedDosage = 0;

char savedPin[] = "0000";
char selectPin[] = "0000";
short pinIndex = 0;
enum UISTATE pinNextState = NONE;
enum UISTATE pinReturnState = NONE;


//holds time string
char time[8] = "10:37";
// holds IP string
char IP[16] = "000.000.000.000";
/**
 * Handlers for default menu pages
 */
void drawMenuPage(const menuPage_t*);
void menuPageHandleSelect(const menuPage_t*);
void menuPageHandleBack(const menuPage_t*);
void menuPageHandleEnc(const menuPage_t*, short dir);
/**
 * Handles draws for standardized menu page cursor
 */
void drawMenuPageCursor(const menuPage_t* this);

/**
 * Redraws Cursor on current menu
 */
void drawCursor();

/**
 * Helper function to draw a cursor
 */
void drawCursorHandler(const uint8_t loc, const uint8_t total);

/**
 * Handlers for pin menus
 */
void loadPin();
uint8_t checkPin();
void savePin(char* pin);
void handleSelectPin();
void handleBackPin();
void handleEncPin(short dir);
void drawPin();
void drawSelectedPin();
void drawPinCursor();

// Gets IP address as a string
char* getIP();

// gets time as a formatted string
char* getTime();

void UI_init(){
	currentSelection = 0;
	currentState = MAIN;
	prevState = NONE;
	drawScreen();
	loadPin();
}


void changeState(UISTATE_t nextState){
	LogDebug("UI Change State from %d to %d\n",currentState,nextState);
	if(nextState == DOSAGELIST){
		dosageList = getDosages(&numDosages);
	}
	if(nextState == VERIFYPIN || nextState == CHANGEPIN){
		currentState = nextState;
		pinIndex = 0;
		for(int i = 0; i < 4; ++i){
			selectPin[i] = '0';
		}
		drawScreen();
		return;
	}
	lastSelection = currentSelection;
	currentSelection = 0;
	prevState = currentState;
	currentState = nextState;
	drawScreen();
	if(nextState == DISPENSE){
		// put dispense call here
		Dosage_t doses[5];
		readDoses(doses,5);
		dispenseDosage(doses+selectedDosage);
		// dispense(c1,c2,c3,c4);
		// assuming blocking
		changeState(MAIN);
	}
}

void handleSelect(){
	// verify in valid state
	if(currentState < 0){
		changeState(MAIN);
		return;
	}

	switch(currentState){
		case MAIN:
			menuPageHandleSelect(&mainPage);
			break;
		case CONFIG:
			if(configPage.options[currentSelection].next == VERIFYPIN){
				pinNextState = CHANGEPIN;
				pinReturnState = MAIN;
			}
			menuPageHandleSelect(&configPage);
			break;
		case SCHEDULE:
			menuPageHandleSelect(&schedulePage);
			break;
		case DOSAGELIST:
			selectedDosage = currentSelection;
			changeState(DOSAGEINFO);
			break;
		case DOSAGEINFO:
			pinNextState = DISPENSE;
			pinReturnState = DOSAGEINFO;
			changeState(VERIFYPIN);
			break;
		case VERIFYPIN:
			handleSelectPin();
			if(pinIndex == 4){
				if(checkPin()){
					changeState(pinNextState);
					break;
				}
					pinIndex = 0;
			}
			drawPinCursor();
			break;
		case CHANGEPIN:
			handleSelectPin();
			if(pinIndex == 4){
				savePin(selectPin);
				changeState(pinReturnState);
				break;
			}
			drawPinCursor();
			break;
		case DISPENSE:
			changeState(MAIN);
			break;
		default:
			break;
	}

}

void handleBack(){
	// verify in valid state
	if(currentState < 0){
		changeState(MAIN);
		return;
	}

	switch(currentState){
		case MAIN:
			menuPageHandleBack(&mainPage);
			break;
		case CONFIG:
			menuPageHandleBack(&configPage);
			break;
		case SCHEDULE:
			menuPageHandleBack(&schedulePage);
			break;
		case DOSAGELIST:
			changeState(MAIN);
			break;
		case DOSAGEINFO:
			changeState(DOSAGELIST);
			break;
		case VERIFYPIN:
		case CHANGEPIN:
			handleBackPin();
			if(pinIndex == -1){
				changeState(pinReturnState);
				break;
			}
			drawPinCursor();
			break;
		case SHOWTIME:
		case SHOWIP:
			changeState(CONFIG);
			break;
		case LOW_PILL_ALERT:
		case DISPENSE_ALERT:
			changeState(MAIN);
			break;

		default:
			break;
	}
}

void handleEncoder(short dir){
	// verify in valid state
	if(currentState < 0){
		return;
	}

	switch(currentState){
		case MAIN:
			menuPageHandleEnc(&mainPage,dir);
			break;
		case CONFIG:
			menuPageHandleEnc(&configPage,dir);
			break;
		case SCHEDULE:
			menuPageHandleEnc(&schedulePage,dir);
			break;
		case DOSAGELIST:
			if(dir){
				++currentSelection;
				if(currentSelection >= numDosages){
					currentSelection = 0;
				}
			}
			else{
				--currentSelection;
				if(currentSelection < 0){
					currentSelection = numDosages - 1;
				}
			}
			break;
		case VERIFYPIN:
		case CHANGEPIN:
			handleEncPin(dir);
			drawSelectedPin();
			break;
		default:
			break;
	}
	drawCursor();
}

void drawScreen(){
	// verify in valid state
	if(currentState < 0){
		return;
	}
    ILI9341_FillScreen(BLACK);
	switch(currentState){
		case MAIN:
			drawMenuPage(&mainPage);
			drawCursor();
			break;
		case CONFIG:
			drawMenuPage(&configPage);
			drawCursor();
			break;
		case SCHEDULE:
			drawMenuPage(&schedulePage);
			drawCursor();
			break;
		case DOSAGELIST:
			ILI9341_DrawText("Dosages",FONT4,10,5,BLACK,WHITE);
			// display menu contents
			for(int i = 0; i < numDosages; i++){
				ILI9341_DrawText(dosageList[i].name,FONT3,25,i*font3Height + initialOffset,BLACK,WHITE);
			}
			drawCursor();
			break;
		case DOSAGEINFO:
			dosage_ui_t* dose = dosageList + selectedDosage;
			ILI9341_DrawText(dose->name,FONT4,10,5,BLACK,WHITE);
			// handle time
			char doseTime[6] = "  :  ";
			doseTime[0] = dose->hour/10 + '0';
			doseTime[1] = dose->hour%10 + '0';
			doseTime[3] = dose->minute/10 + '0';
			doseTime[4] = dose->minute%10 + '0';
			ILI9341_DrawText(doseTime,FONT4,25,font3Height + initialOffset,BLACK,WHITE);
			char pillAmount[32];
			char* pillNames[4] = {dose->p1,dose->p2,dose->p3,dose->p4};
			for(int i = 0; i < 4; ++i){
				snprintf(pillAmount,32,"%.10s: %02d",pillNames[i],dose->pillAmounts[i]);
				ILI9341_DrawText(pillAmount,FONT4,25,(1+i)*font3Height + initialOffset,BLACK,WHITE);
			}
			break;
		case VERIFYPIN:
		case CHANGEPIN:
			drawPin();
			break;
		case DISPENSE:
			ILI9341_DrawText("DISPENSING NOW",FONT4,10,5,BLACK,WHITE);
			ILI9341_DrawText(dosageList[selectedDosage].name,FONT3,25,25,BLACK,WHITE);
			break;
		case SHOWTIME:
			char* time = getTime();
			ILI9341_DrawText(time,FONT4,10,5,BLACK,WHITE);
			break;
		case SHOWIP:
			char* ip = getIP();
			ILI9341_DrawText(ip,FONT4,10,5,BLACK,WHITE);
			break;
		case DISPENSE_ALERT:
			ILI9341_DrawText("Dosage to be Taken",FONT3,10,5,BLACK,WHITE);
			uint8_t channel;
			readDosageAlertChannel(&channel);
			Dosage_t doses[5];
			int num_doses = readDoses(doses,5);
			if(num_doses < channel){
				break;
			}
			Dosage_t dosage = doses[channel];
			char timeStr[] = "  :  ";
			snprintf(timeStr,6,"%02d:%02d",dosage.hour,dosage.min);
			char DAbuffer1[32];
			snprintf(DAbuffer1,32,"Dosage at %s",timeStr);
			ILI9341_DrawText(DAbuffer1,FONT3,10,25,BLACK,WHITE);
			uint8_t pillAmounts[4] = {dosage.pillOneCount,dosage.pillTwoCount,dosage.pillThreeCount,dosage.pillFourCount};
			char* names[4] = {dosage.pillOne,dosage.pillTwo,dosage.pillThree,dosage.pillFour};
			for(int i = 0; i < 4; ++i){
				char DAbuffer2[32];
				snprintf(DAbuffer2,32,"%.10s: %02d",names[i],pillAmounts[i]);
				ILI9341_DrawText(DAbuffer2,FONT3,10,25 + (i+1)*font3Height,BLACK,WHITE);
			}
			
			//ILI9341_DrawText("THERE ARE PILLS",FONT3,10,5,BLACK,WHITE);
			//ILI9341_DrawText("TO BE TAKEN", FONT3,10,25,BLACK,WHITE);
			break;
		case LOW_PILL_ALERT:
			ILI9341_DrawText("LOW PILL ALERT",FONT3,10,5,BLACK,WHITE);
			int i = 1;
			for(;i < 5; ++i){
				if(readLowPillAlert(i)) break;
			}
			if(i == 5){
				break;
			}
			Config_t config;
			readConfig(&config,i);
			char buffer1[32];
			char buffer2[16];
			snprintf(buffer1,32,"%d left in %.10s",config.pillCount,config.pillName);
			snprintf(buffer2,16,"Channel %d",config.channel);
			ILI9341_DrawText(buffer1,FONT3,10,25,BLACK,WHITE);
			ILI9341_DrawText(buffer2,FONT3,10,45,BLACK,WHITE);
			break;
		default:
			break;
	}
}

void drawCursor(){
	// verify in valid state
	if(currentState < 0){
		return;
	}

	switch(currentState){
		case MAIN:
			drawMenuPageCursor(&mainPage);
			break;
		case CONFIG:
			drawMenuPageCursor(&configPage);
			break;
		case SCHEDULE:
			drawMenuPageCursor(&schedulePage);
			break;
		case DOSAGELIST:
			drawCursorHandler(currentSelection,numDosages);
			break;
		default:
			break;
	}
}

void drawMenuPage(const menuPage_t* this){
	// display name
	ILI9341_DrawText(this->name,FONT4,10,5,BLACK,WHITE);
	// display menu contents
	for(int i = 0; i < this->numOptions; i++){
		ILI9341_DrawText(this->options[i].name,FONT3,25,i*font3Height + initialOffset,BLACK,WHITE);
	}
	//ILI9341_DrawChar('>',FONT3,10,initialOffset,BLACK,WHITE);
}

void menuPageHandleSelect(const menuPage_t* this){
	changeState(this->options[currentSelection].next);
}

void menuPageHandleBack(const menuPage_t* this){
	changeState(this->backState);
}

void menuPageHandleEnc(const menuPage_t* this,short dir){
	// increment or decrement current option
	if(dir){
		++currentSelection;
		if(currentSelection >= this->numOptions){
			currentSelection = 0;
		}
	}
	else{
		--currentSelection;
		if(currentSelection < 0){
			currentSelection = this->numOptions - 1;
		}
	}
}

void drawCursorHandler(const uint8_t loc, const uint8_t total){
	ILI9341_DrawFilledRectangleCoord(0,initialOffset,25,total*font3Height + initialOffset,BLACK);
	ILI9341_DrawChar('>',FONT3,10,initialOffset + font3Height * loc,BLACK,WHITE);
}

void drawMenuPageCursor(const menuPage_t* this){
	drawCursorHandler(currentSelection, this->numOptions);
}

dosage_ui_t* getDosages(unsigned short* num){
	// returning a set dosages
	//LogDebug("Entering get dosages\n");
	Dosage_t fsDosage[5];
	int num_doses = readDoses(fsDosage,5);
	if(num_doses < 0){
		LogDebug("Bad File Read in UI.\n");
		num = 0;
		return dosages;
	}
	*num = num_doses;
	for(int i = 0; i < num_doses; ++i){
		dosage_ui_t* dose = dosages + i;
		sprintf(dose->name,"%02d:%02d",fsDosage[i].hour,fsDosage[i].min);
		//LogDebug("%s\n",dose->name);
		// pill one name
		sprintf(dose->p1,"%.4s",fsDosage[i].pillOne);
		// pill two name
		sprintf(dose->p2,"%.4s",fsDosage[i].pillTwo);
		// pill three name
		sprintf(dose->p3,"%.4s",fsDosage[i].pillThree);
		// pill four name
		sprintf(dose->p4,"%.4s",fsDosage[i].pillFour);

		//LogDebug("%s\n",dose->p1);
		//LogDebug("%s\n",dose->p2);
		//LogDebug("%s\n",dose->p3);
		//LogDebug("%s\n",dose->p4);
		dose->pillAmounts[0] = fsDosage[i].pillOneCount;
		dose->pillAmounts[1] = fsDosage[i].pillTwoCount;
		dose->pillAmounts[2] = fsDosage[i].pillThreeCount;
		dose->pillAmounts[3] = fsDosage[i].pillFourCount;
	}
/*
	// 4 different dosages
	*num = 4;

	dosage_ui_t* dose = dosages;
	dose->name[0] = 'm';
	dose->name[1] = 'o';
	dose->name[2] = 'r';
	dose->name[3] = 'n';
	dose->name[4] = 'i';
	dose->name[5] = 'n';
	dose->name[6] = 'g';
	dose->name[7] = '\0';
	dose->hour = 6;
	dose->minute = 0;
	dose->pillAmounts[0] = 1;
	dose->pillAmounts[1] = 0;
	dose->pillAmounts[2] = 0;
	dose->pillAmounts[3] = 1;

	dose = dosages + 1;
	dose->name[0] = 'n';
	dose->name[1] = 'o';
	dose->name[2] = 'o';
	dose->name[3] = 'n';
	dose->name[4] = '\0';
	dose->hour = 12;
	dose->minute = 0;
	dose->pillAmounts[0] = 0;
	dose->pillAmounts[1] = 1;
	dose->pillAmounts[2] = 0;
	dose->pillAmounts[3] = 1;

	dose = dosages + 2;
	dose->name[0] = 'e';
	dose->name[1] = 'v';
	dose->name[2] = 'e';
	dose->name[3] = 'n';
	dose->name[4] = 'i';
	dose->name[5] = 'n';
	dose->name[6] = 'g';
	dose->name[7] = '\0';
	dose->hour = 15;
	dose->minute = 30;
	dose->pillAmounts[0] = 0;
	dose->pillAmounts[1] = 0;
	dose->pillAmounts[2] = 1;
	dose->pillAmounts[3] = 1;

	dose = dosages + 3;
	dose->name[0] = 'n';
	dose->name[1] = 'i';
	dose->name[2] = 'g';
	dose->name[3] = 'h';
	dose->name[4] = 't';
	dose->name[5] = '\0';
	dose->hour = 21;
	dose->minute = 0;
	dose->pillAmounts[0] = 0;
	dose->pillAmounts[1] = 0;
	dose->pillAmounts[2] = 0;
	dose->pillAmounts[3] = 1;
*/
	return dosages;
}

void loadPin(){
	uint8_t buffer[4];
	readPincode(buffer);
	savedPin[0] = buffer[0]+'0';
	savedPin[1] = buffer[1]+'0';
	savedPin[2] = buffer[2]+'0';
	savedPin[3] = buffer[3]+'0';
	return;
}

uint8_t checkPin(){
	for(int i = 0; i < 4; ++i){
		if(savedPin[i] != selectPin[i])
			return 0;
	}
	return 1;
}

void savePin(char* pin){
	for(int i = 0; i < 4; ++i){
		savedPin[i] = pin[i];
	}
	uint8_t buffer[4] = {savedPin[0]-'0',savedPin[1]-'0',savedPin[2]-'0',savedPin[3]-'0'};
	writePincode(buffer);
}

void handleSelectPin(){
	++pinIndex;
}

void handleBackPin(){
	if (pinIndex == 0){
		changeState(prevState);
	}
	--pinIndex;
}

void handleEncPin(short dir){
	if(dir){
		++selectPin[pinIndex];
		if(selectPin[pinIndex] > '9'){
			selectPin[pinIndex] = '0';
		}
	}
	else{
		--selectPin[pinIndex];
		if(selectPin[pinIndex] < '0'){
			selectPin[pinIndex] = '9';
		}
	}
}

void drawPin(){
	if(currentState == VERIFYPIN){
		ILI9341_DrawText("VERIFY PIN",FONT4,10,5,BLACK,WHITE);
	}
	else{
		ILI9341_DrawText("CHANGE PIN",FONT4,10,5,BLACK,WHITE);
	}
	drawSelectedPin();
	drawPinCursor();

}

void drawSelectedPin(){
	ILI9341_DrawText(selectPin,FONT3,25,initialOffset,BLACK,WHITE);
}

void drawPinCursor(){
	ILI9341_DrawFilledRectangleCoord(25,45,75,65,BLACK);
	ILI9341_DrawText("^",FONT3,pinIndex*8 + 25,45,BLACK,WHITE);
	//char debug[] = "0";
	//debug[0] = '0' + pinIndex;
	//ILI9341_DrawText(debug,FONT3,80,80,BLUE,GREEN);

}

uint8_t IPTESTARRAY[] = {10,19,96,1};
char* getIP(){
	sprintf(IP,"%03d.%03d.%03d.%03d",IPTESTARRAY[0],IPTESTARRAY[1],IPTESTARRAY[2],IPTESTARRAY[3]);
	return IP;
}

char* getTime(){
	  RTC_DateTypeDef rtcDate;
	  RTC_TimeTypeDef rtcTime;
	  get_rtc_typedef(&rtcDate, &rtcTime);
	  sprintf(time,"%02d:%02d",rtcTime.Hours,rtcTime.Minutes);
	  LogDebug("%s\n",time);
	return time;
}

void UI_handleInput(){
    if (selectPressed == 1)
    {
    	selectPressed = 0;
    	handleSelect();
    }
    if (backPressed == 1)
    {
    	backPressed = 0;
    	handleBack();
    }
    if (encoderState == CW)
    {
    	encoderState = NONE;
    	handleEncoder(0);
    }
    else if (encoderState == CCW)
    {
    	encoderState = NONE;
    	handleEncoder(1);
    }
    if (readDosageAlert()){
    	clearDosageAlert();
    	changeState(DISPENSE_ALERT);
    }
}
