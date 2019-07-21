/* 
* f_list_item_manager.cpp
*
* Created: 05.03.2018 14:28:39
* Author: Maximilian Starke
*
*	FPS: see at header file.
*
*/


#include "f_list_item_manager.h"

ui::ListItemManager::Item ui::ListItemManager::item_list_decoder(uint8_t index, uint8_t size, const char* labels, void (* const *callbacks)()){
	if (index < size) {
		const char * ptr = & labels[0];
		for (uint8_t i = 0; 0 < index;){
			if (*ptr == '\0') break;
			if (*ptr == '\n') ++i;
			++ptr;
		}
		Item item;
		hardware::copyString(item.label,ptr,8,false);
		item.callback = callbacks[index];
	}
	return Item();
}