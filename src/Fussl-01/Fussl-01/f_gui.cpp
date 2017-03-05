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
#include "f_ledline.h"

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

void input::setEvent(int8_t eventId, Callable* callback, bool enabled /* = true */){
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

/************************************************************************/
/* ItemSelector                                                         */
/************************************************************************/

void ItemSelector::OkayCall::operator ()() const {
	using namespace input;
	if (host->ok_pressed == 0){
		host->freeButtons();
		led::printDotsOnly(0xFF);
		hardware::delay(300);
		setEvent(host->button_okay,BUTTON_UP,&host->okayCall);
		host->ok_pressed = 1;
		} else {
		led::printDotsOnly(0x00);
		host->itemManager->runItemProcedure();
		host->finalize();
	}
}

void ItemSelector::NextCall::operator ()() const {
	++(*host->itemManager);
	host->printItem();
}

void ItemSelector::PreviousCall::operator ()() const {
	--(*host->itemManager);
	host->printItem();
}

bool ItemSelector::isConfigValid(){
	return
		(button_okay < 5) &&
		((button_next < 5) || (button_prev < 5)) &&
			(
				(button_next < 5) + (button_next == NO_BUTTON) + (button_prev < 5) + (button_prev == NO_BUTTON) == 2
			) &&
		(itemManager != nullptr)
	;
}

void ItemSelector::occupyButtons(){
	using namespace input;
	ok_pressed = 0;
	deleteEvent(makeEvent(button_okay,BUTTON_UP));
	setEvent(button_okay,BUTTON_DOWN,&okayCall);
	if (button_next != NO_BUTTON){
		deleteEvent(makeEvent(button_next,BUTTON_UP));
		setEvent(button_next,BUTTON_DOWN,&nextCall,true);
	}
	if (button_prev != NO_BUTTON){
		deleteEvent(makeEvent(button_prev,BUTTON_UP));
		setEvent(button_prev,BUTTON_DOWN,&previousCall,true);
	}
}

void ItemSelector::freeButtons(){
	using namespace input;
	deleteEvent(makeEvent(button_okay,BUTTON_DOWN));
	deleteEvent(makeEvent(button_okay,BUTTON_UP));
	if (button_next != NO_BUTTON){
		deleteEvent(makeEvent(button_next,BUTTON_DOWN));
		//disableEvent(makeEvent(button_next,BUTTON_UP)); //not needed because it was disabled when occupying buttons
	}
	if (button_prev != NO_BUTTON){
		deleteEvent(makeEvent(button_prev,BUTTON_DOWN));
		//disableEvent(makeEvent(button_prev,BUTTON_UP)); //not needed
	}
}

void ItemSelector::init(ItemManager* itemManager, uint8_t button_okay, uint8_t button_next, uint8_t button_prev){
	ItemSelector::itemManager = itemManager;
	ItemSelector::button_okay = button_okay;
	ItemSelector::button_next = button_next;
	ItemSelector::button_prev = button_prev;
	/* it may not be necessary to check here whether b#########uttons are valid but we need to do sometime */
}

void ItemSelector::printItem(){
	char label[9];
	itemManager->getItemLabel(label);
	label[8] = '\0';
	led::LFPrintString(label);// this function should also exist with an second parameter to set the string length!!!#####
}

bool ItemSelector::run(){
	if (!isConfigValid()) return false;			// no valid buttons / itemManager
	if (itemManager->isEmpty()) return false;	// no items
	
	occupyButtons();
	led::LFPrintString("SELECT");
	hardware::delay(1000);// <<< this also could be replaced.
	printItem();
	return true;
}

/************************************************************************/
/*  ItemManager                                                         */
/************************************************************************/

bool ItemManager::runCancelProcedure(){
	if (canCancel()){
		cancelProcedure();
		return true;
		} else {
		return false;
	}
}

ItemManager& ItemManager::operator++(){ // omitting the return value could be better for small RAM ######
	position = (position + 1) % countItems();
	return *this;
}

ItemManager& ItemManager::operator--(){
	position = (position - 1) % countItems();
	return *this;
}

void ItemManager::getItemLabel(char* string_8_bytes){
	if (onCancelItem()){
		char mystring[] = "-CANCEL-";
		hardware::copyString(string_8_bytes,mystring,8,false);
		} else {
		getItemLabelInternal(string_8_bytes);
	}
}

void ItemManager::runItemProcedure(){
	if (onCancelItem()){
		cancelProcedure(); // if clause makes calling save
		} else {
		runItemProcedureInternal();
	}
}


#ifdef debugxl

void ArcProgramItemManager::runItemProcedureInternal() {
	arch::runProgram(position + (!includingOffProgram));
	if (finalProcedure != nullptr){
		finalProcedure();
	}
}

#endif