/*
 * f_gui.cpp
 *
 * Created: 12.09.2016 10:25:46
 *  Author: Maximilian Starke
 *
 *	FILE PROGRTESS STATE: see at f_gui.h
 *
 */ 
#include <stdlib.h>
#include "f_gui.h"

/************************************************************************/
/* namespace input                                                      */
/************************************************************************/

namespace input {
	uint8_t gbutton;
	uint8_t gchange;
	EventContainer inputEvents;
}

void input::init(){
	DDRA  &= 0b00000111; // set all inputs (should be so before) <<<<< test via reading register
	PORTA |= 0b11111000; // activate pull up resistors
	gbutton = 0;
	gchange = 0;
	deleteEventsAll();
}

bool input::readInput(){

	uint8_t button = 0b00011111 & ((~PINA)>>3); // because input pin signals are negative logic:		HIGH == button released			LOW == button pressed
	gchange = gbutton ^ button; // In Previous Line: <<<<< check whether there come ZEROs in when shifting
	gbutton = button;
	return static_cast<bool>(gchange);
}

bool input::exec(bool all /* = false */){
	bool result = false;
	for(int8_t i = 4; i>=0; --i){
		if (gchange & (1<<i)){
			if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].enable > 0){
				if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].enable == 1){
					if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].callbackReference.callbackProcedure != nullptr){
						inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].callbackReference.callbackProcedure();
					}
				}
				if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].enable == 2){
					if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].callbackReference.callbackObject != nullptr){
						inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].callbackReference.callbackObject->operator ()();
					}
				}
				if (!all) return true;
				result = true;
			}
		}
	}
	return result;
}

void input::setEvent(int8_t eventId, fsl::str::callable* callback, bool enabled /* = true */){
	if ( (eventId >= 10) || (eventId < 0) ){ // illegal eventId
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = 2 * (2*enabled - 1) ;
	inputEvents.buttonEvent[eventId/2][eventId%2].callbackReference.callbackObject = callback;
}

void input::setEvent(int8_t eventId, void (*callbackProcedure)(), bool enabled /* = true */){
	if ( (eventId >= 10) || (eventId < 0) ){
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = 2 * enabled -1;
	inputEvents.buttonEvent[eventId/2][eventId%2].callbackReference.callbackProcedure = callbackProcedure;
}

bool input::enableEvent(int8_t eventId){
	if ( (eventId >= 10) || (eventId < 0) ){
		return false;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = abs(inputEvents.buttonEvent[eventId/2][eventId%2].enable);
	return inputEvents.buttonEvent[eventId/2][eventId%2].enable;
}

void input::disableEvent(int8_t eventId, bool deleting){
	if ( (eventId >= 10) || (eventId < 0) ){
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = - abs(inputEvents.buttonEvent[eventId/2][eventId%2].enable) * (deleting == false);
}

int8_t input::getEventCode(){
	bool existingEvent{false};				// to mark if there was at least one button state change
	constexpr uint8_t COUNT_BUTTONS {5};
	uint8_t button {COUNT_BUTTONS};			// illegal value
		
	for(uint8_t i = 0; i < COUNT_BUTTONS; ++i){ // iterate through all positions where a button is saved in the global gchange
		if ((1<<i) & gchange){
			if (existingEvent) return MULTI_CHANGE;
			button = i;
			existingEvent = true;
		}
	}
	if (!existingEvent) return NO_CHANGE;
	return 2*button + !!(gchange & gbutton);
}



#ifdef debugxl

void ArcProgramItemManager::runItemProcedureInternal() {
	arch::runProgram(position + (!includingOffProgram));
	if (finalProcedure != nullptr){
		finalProcedure();
	}
}

#endif