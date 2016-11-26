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

int8_t input::getEvent(){ 
	bool multiEvent{false};								// to mark if there were more than one button state changes
	constexpr uint8_t _NO_BUTTON_CHANGED_INTERN_ {5};
	uint8_t button  = {_NO_BUTTON_CHANGED_INTERN_};			// the 'else' value which means there was no event
	
	for(uint8_t i = 0; i < 5; ++i){ // iterate through all positions where a button is saved in the global gchange
		if ((1<<i) & gchange){
			if (multiEvent) return getEvent_MULTI_CHANGE;
			button = i;
			multiEvent = true;
		}
	}
	if (button == _NO_BUTTON_CHANGED_INTERN_) return getEvent_NO_CHANGE;
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

void input::enableEvent(uint8_t buttonEvent, void (*procedure)()){
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



/****************************************************************************************************************************************************************************************************/

namespace ItemSelector {
	
	ItemManager* itemManager = nullptr;
	
	uint8_t button_okay = NO_BUTTON;							// number (0~4) of the button to occupy. Out of range arguments will cause unknown behavior
	uint8_t button_next = NO_BUTTON;
	uint8_t button_prev = NO_BUTTON;
}

void ItemSelector::enableButtons(){
	using namespace input;
	disableEvent(makeEvent(button_okay,BUTTON_UP));
	enableEvent(makeEvent(button_okay,BUTTON_DOWN),ItemSelector::okay_down);
	disableEvent(makeEvent(button_next,BUTTON_UP));
	enableEvent(makeEvent(button_next,BUTTON_DOWN),ItemSelector::next);
	if (button_prev != NO_BUTTON){
		disableEvent(makeEvent(button_prev,BUTTON_UP));
		enableEvent(makeEvent(button_prev,BUTTON_DOWN),ItemSelector::previous);
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

void ItemSelector::initialisation(uint8_t button_okay, uint8_t button_next, uint8_t button_prev, ItemManager* itemManager){
	ItemSelector::button_okay = button_okay;
	ItemSelector::button_next = button_next;
	ItemSelector::button_prev = button_prev;
	/*
	ItemSelector::itemManager = itemManager;*/
}


void ItemSelector::okay_down(){
	using namespace input;
	setButtonsFree();
	led::printDotsOnly(0xFF);
	hardware::delay(300);
	enableEvent(makeEvent(button_okay,BUTTON_UP),okay_up);
}

void ItemSelector::okay_up(){
	led::printDotsOnly(0x00);
	itemManager->runItemProcedure();
	finalize();
}


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