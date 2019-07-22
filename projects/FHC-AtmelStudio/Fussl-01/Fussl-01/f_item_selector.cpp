/*
* f_item_selector.cpp
*
* Created: 05.03.2018 13:05:54
* Author: Maximilian Starke
*
* FPS: see at f_item_selector.h
*/

#include "f_item_selector.h"
#include "old_or_deprecated/f_gui.h"
#include "f_ledline.h"

void ui::ItemSelector::OkayCall::operator ()() {
	using namespace input;
	if (host->flags.get(OK_DOWN) == 0) { // okay is just now pressed down
		host->deactivate_button_events();
		led::printDotsOnly(0xFF);
		hardware::delay(DELAY_ON_OKAY);
		setEvent(host->button_okay, BUTTON_UP, &host->okay_call); // set event handler for button release
		host->flags.set_true(OK_DOWN);
	}
	else { // ok was pressed down and is was released just now
		led::printDotsOnly(0x00);
		host->p_item_manager->run_item_procedure();
		host->finalize();
	}
}

void ui::ItemSelector::NextCall::operator ()() {
	++(*host->p_item_manager);
	host->print_item();
}

void ui::ItemSelector::PreviousCall::operator ()() {
	--(*host->p_item_manager);
	host->print_item();
}

bool ui::ItemSelector::valid() {
	return
	(button_okay < 5) && // okay button is valid
	((button_next < 5) || (button_prev < 5)) && // next or previous button is valid
	( // no button_id has got a number which was not defined
	(button_next < 5) + (button_next == NO_BUTTON) + (button_prev < 5) + (button_prev == NO_BUTTON) == 2
	) &&
	(p_item_manager != nullptr) // itemManager is valid
	;
}

void ui::ItemSelector::activate_button_events() {
	using namespace input; // << this is not so pretty, since I want to change namespace sometimes
	flags.set_false(OK_DOWN);
	deleteEvent(makeEvent(button_okay, BUTTON_UP));
	setEvent(button_okay, BUTTON_DOWN, &okay_call);
	if (button_next != NO_BUTTON) {
		deleteEvent(makeEvent(button_next, BUTTON_UP));
		setEvent(button_next, BUTTON_DOWN, &next_call, true);
	}
	if (button_prev != NO_BUTTON) {
		deleteEvent(makeEvent(button_prev, BUTTON_UP));
		setEvent(button_prev, BUTTON_DOWN, &previous_call, true);
	}
}

void ui::ItemSelector::deactivate_button_events() {
	using namespace input;
	deleteEvent(makeEvent(button_okay, BUTTON_DOWN));
	deleteEvent(makeEvent(button_okay, BUTTON_UP));
	if (button_next != NO_BUTTON) {
		deleteEvent(makeEvent(button_next, BUTTON_DOWN));
		//disableEvent(makeEvent(button_next,BUTTON_UP)); //not needed because it was disabled when occupying buttons
	}
	if (button_prev != NO_BUTTON) {
		deleteEvent(makeEvent(button_prev, BUTTON_DOWN));
		//disableEvent(makeEvent(button_prev,BUTTON_UP)); //not needed
	}
}

bool ui::ItemSelector::run(bool show_select_screen) {
	if (flags.get(IS_RUNNING)) return false;	// already running
	if (!valid()) return false;			// no valid buttons / itemManager
	if (p_item_manager->empty()) return false;	// no items

	activate_button_events();
	if (show_select_screen) {
		led::LFPrintString("SELECT");
		hardware::delay(DELAY_ON_START);
	}
	print_item();
	return true;
}

bool ui::ItemSelector::init(ItemManager* itemManager, uint8_t button_okay, uint8_t button_next, uint8_t button_prev) {
	if (flags.get(IS_RUNNING)) return false;
	ItemSelector::p_item_manager = itemManager;
	ItemSelector::button_okay = button_okay;
	ItemSelector::button_next = button_next;
	ItemSelector::button_prev = button_prev;
	// valid button codes need not be checked. this will be done right in run() right before
	return true;
}

void ui::ItemSelector::print_item() {
	if (flags.get(IS_RUNNING) == false) return;
	char label[8];
	p_item_manager->item_label(label);
	led::LFPrintString(label, 8);// this function should also exist with an second parameter to set the string length!!!#####
}
