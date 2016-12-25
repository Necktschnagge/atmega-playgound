/*
 * f_gui.cpp
 *
 * Created: 12.09.2016 10:25:46
 *  Author: F-NET-ADMIN
 */ 
#include <stdlib.h>
#include "f_gui.h"
#include "f_ledline.h"

namespace input {
	uint8_t gbutton;
	uint8_t gchange;
	event_container_t inputEvents;
}
/*
int8_t input::getEventOld(){ 
	bool multiEvent{false};								// to mark if there were more than one button state changes
	constexpr uint8_t _NO_BUTTON_CHANGED_INTERN_ {5};
	uint8_t button  = {_NO_BUTTON_CHANGED_INTERN_};			// the 'else' value which means there was no event
	
	for(uint8_t i = 0; i < 5; ++i){ // iterate through all positions where a button is saved in the global gchange
		if ((1<<i) & gchange){
			if (multiEvent) return MULTI_CHANGE;
			button = i;
			multiEvent = true;
		}
	}
	if (button == _NO_BUTTON_CHANGED_INTERN_) return NO_CHANGE;
	return 2*button + !!(gchange & gbutton);
}


void input::fetchEvents(){
	uint8_t button = 0b00011111 & ((~PINA)>>3); // because input pin signals are negative logic:		HIGH == button released			LOW == button pressed 
	gchange = gbutton ^ button; // In Previous Line: <<<<< check whether there come ZEROs in when shifting
	gbutton = button;

	for(int8_t i = 4; i>=0; --i){
		if (gchange & (1<<i)){	
			if (inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].enable){
				inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].proc();
				// delay because of prelling <<<<<<<<<
				hardware::delay(25);
			}
		}
	}
}

void input::enableEventOld(uint8_t buttonEvent, void (*procedure)()){
	if ( buttonEvent >= 10){
		//## throw error;
		return;
	}
	inputEvents.buttonEvent[buttonEvent/2][buttonEvent%2].enable = true;
	inputEvents.buttonEvent[buttonEvent/2][buttonEvent%2].proc = procedure;
}

void input::disableEvents(){
	for(uint8_t button = 0; button <5; ++button){
		inputEvents.buttonEvent[button][BUTTON_UP].enable = false;
		inputEvents.buttonEvent[button][BUTTON_DOWN].enable = false;
	}
}
*/
/* refactored input stuff */

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
					(*(inputEvents.buttonEvent[i][!!(gbutton & (1<<i))].callbackReference.callbackObject))();
				}
				if (!all) return true;
				result = true;
			}
		}
	}
	return result;
}

void input::setEvent(uint8_t eventId, Callable* callback, bool enabled /* = true */){
	if ( eventId >= 10 ){
		// throw error ####
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = 2 * (2*enabled - 1) ;
	inputEvents.buttonEvent[eventId/2][eventId%2].callbackReference.callbackObject = callback;
}

void input::setEvent(uint8_t eventId, void (*callbackProcedure)(), bool enabled /* = true */){
	if ( eventId >= 10 ){
		// throw error ####
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = 2 * enabled -1;
	inputEvents.buttonEvent[eventId/2][eventId%2].callbackReference.callbackProcedure = callbackProcedure;
}

/* this should not be used. make your own prg logic
const input::event_t& input::getEvent(uint8_t eventId){
	if ( eventId >= 10 ){
		// throw error ####
		eventId = 0;
	}
	return inputEvents.buttonEvent[eventId/2][eventId%2];
}
*/

bool input::enableEvent(uint8_t eventId){
	if ( eventId >= 10 ){
		// throw error ####
		return false;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = abs(inputEvents.buttonEvent[eventId/2][eventId%2].enable);
	return inputEvents.buttonEvent[eventId/2][eventId%2].enable;
}

void input::disableEvent(uint8_t eventId){
	if ( eventId >= 10 ){
		// throw error ####
		return;
	}
	inputEvents.buttonEvent[eventId/2][eventId%2].enable = - abs(inputEvents.buttonEvent[eventId/2][eventId%2].enable);
}


uint8_t input::getEventCode(){
	bool existingEvent{false};								// to mark if there were more than one button state changes
	constexpr uint8_t _NO_BUTTON_CHANGED_INTERN_ {5};
	uint8_t button {_NO_BUTTON_CHANGED_INTERN_};			// the 'else' value which means there was no event
	constexpr uint8_t COUNT_BUTTONS {5};
		
	static_assert(_NO_BUTTON_CHANGED_INTERN_ >= COUNT_BUTTONS, "error in programming logic: no_button seems to be a button representation too");
		
	for(uint8_t i = 0; i < COUNT_BUTTONS; ++i){ // iterate through all positions where a button is saved in the global gchange
		if ((1<<i) & gchange){
			if (existingEvent) return MULTI_CHANGE;
			button = i;
			existingEvent = true;
		}
	}
	if (button == _NO_BUTTON_CHANGED_INTERN_) return NO_CHANGE;
	return 2*button + !!(gchange & gbutton);
}

/****************************************************************************************************************************************************************************************************/
#ifdef debugxl
void ItemSelector::enableButtons(){
	using namespace input;
	disableEvent(makeEvent(button_okay,BUTTON_UP));
	setEvent(makeEvent(button_okay,BUTTON_DOWN),&okayCall);
	disableEvent(makeEvent(button_next,BUTTON_UP));
	setEvent(makeEvent(button_next,BUTTON_DOWN),&nextCall);
	if (button_prev != NO_BUTTON){
		disableEvent(makeEvent(button_prev,BUTTON_UP));
		setEvent(makeEvent(button_prev,BUTTON_DOWN),&previousCall);
	}
}

void ItemSelector::setButtonsFree(){
	using namespace input;
	disableEvent(makeEvent(button_okay,BUTTON_DOWN));
	disableEvent(makeEvent(button_okay,BUTTON_UP));
	disableEvent(makeEvent(button_next,BUTTON_DOWN));
	disableEvent(makeEvent(button_next,BUTTON_UP));
	if (button_prev != NO_BUTTON){
		disableEvent(makeEvent(button_prev,BUTTON_DOWN));
		disableEvent(makeEvent(button_prev,BUTTON_UP));
	}
}

void ItemSelector::init(uint8_t button_okay, uint8_t button_next, uint8_t button_prev, ItemManager* itemManager){
	ItemSelector::button_okay = button_okay;
	ItemSelector::button_next = button_next;
	ItemSelector::button_prev = button_prev;
	ItemSelector::itemManager = itemManager;
	ok_pressed = 0;
}


void ItemSelector::okay_down(){
	if (ok_pressed = 0){
		setButtonsFree();
		led::printDotsOnly(0xFF);
		hardware::delay(300);
		input::setEvent(input::makeEvent(button_okay,input::BUTTON_UP),&ItemSelector::okayCall);
		ok_pressed = 1;
	} else {
		led::printDotsOnly(0x00);
		itemManager->runItemProcedure();
		finalize();	
	}
}

//
//void ItemSelector::okay_up(){
	//
//}


void ItemSelector::next(){
	++(*itemManager);
	printItem();
}

void ItemSelector::previous(){
	--(*itemManager);
	printItem();
}

void ItemSelector::printItem(){
	char label[9];
	itemManager->getItemLabel(label);
	label[8] = '\0';
	led::LFPrintString(label);// this function should also exist with an secound parameter to set the string legth!!!#####
}

bool ItemSelector::run(){
	// no items:
	if (itemManager->isEmpty()) return false; //<<<<<< throw error
	enableButtons();
	led::LFPrintString("SELECT");
	hardware::delay(1000);
	printItem();
	return true;
}




bool ItemManager::runCancelProcedure(){
	if (canCancel()){
		cancelProcedure();
		finalize();
		return true;
		} else {
		return false;
	}
}

ItemManager& ItemManager::operator++(){
	position = (position + 1) % mod();
	return *this;
}

ItemManager& ItemManager::operator--(){
	position = (position - 1) % mod();
	return *this;
}

void ItemManager::getItemLabel(char* string_8_bytes){
	if (onCancelItem()){
		//## make a Cancel
		char mystring[] = "CANCEL";
		hardware::copyString(string_8_bytes,mystring,8,false);
		} else {
		getItemLabelInternal(string_8_bytes);
	}
}

void ItemManager::runItemProcedure(){
	if (onCancelItem()){
		runCancelProcedure();
		} else {
		runItemProcedureInternal();
	}
	finalize();
}




void ArcProgramItemManager::runItemProcedureInternal() {
	arch::runProgram(position + (!includingOffProgram));
	if (finalProcedure != nullptr){
		finalProcedure();
	}
}

#endif