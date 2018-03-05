/*
* f_item_manager.cpp
*
* Created: 05.03.2018 13:13:37
* Author: Maximilian Starke
*/


#include "f_item_manager.h"
#include "f_hardware.h"

ui::ItemManager& ui::ItemManager::operator++(){ // omitting the return value could be better for small RAM ######
	position = (position + 1) % size();
	return *this;
}

ui::ItemManager& ui::ItemManager::operator--(){
	position = (position - 1) % size();
	return *this;
}

void ui::ItemManager::item_label(char* string_8_bytes){
	if (on_cancel_item()){
		hardware::copyString(string_8_bytes,CANCEL_LABEL,8,false);
		} else {
		item_label_virt(string_8_bytes);
	}
}

void ui::ItemManager::run_item_procedure(){
	if (on_cancel_item()){
		cancel_procedure(); // if clause makes calling save. We cannot be on_cancel_item if cancel_procedure == nullptr
		} else {
		run_item_procedure_virt();
	}
}