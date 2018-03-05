/*
* f_list_item_manager.h
*
* Created: 05.03.2018 14:28:39
* Author: Maximilian Starke
*
*	FPS: check everything again!
*/


#ifndef __F_LIST_ITEM_MANAGER_H__
#define __F_LIST_ITEM_MANAGER_H__

#include "f_item_manager.h"
#include "f_hardware.h"

namespace ui {
	
	class ListItemManager final: public ItemManager {
		public:
		
		struct Item {
			char label[8]; // non-null-terminated string
			void (*callback)();
			
			Item(): label{'\0'}, callback(nullptr){}
		};

		static Item item_list_decoder(uint8_t index, uint8_t size, const char* labels, void (* const *callbacks)()){
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

		
		inline static Item item_list_array_example(uint8_t index){
			const uint8_t SIZE = 4;
			const char labels[] =
R"xxx(item 1
item 2
item 3
item 4)xxx";
			void (* const callbacks[SIZE])() = {nullptr, nullptr,nullptr,nullptr};
			return item_list_decoder(index,SIZE,labels,&callbacks[0]);
		}
		
		private:
		
		Item (*item_array)(uint8_t index);
		
		uint8_t size;
		
		protected:
		
		virtual void item_label_virt(char* string_8_bytes) override {
			if (item_array != nullptr) return hardware::copyString(string_8_bytes,&(item_array(position).label[0]),8,false);//this copying should be done in the item array
		}
		
		virtual void run_item_procedure_virt() override {
			if (item_array != nullptr) return item_array(position).callback();
		}
		
		virtual uint16_t size_without_cancel_item() override {
			return size;
		}
		
		public:
		
		ListItemManager() : ItemManager() , item_array(nullptr), size(0){}
		
		virtual void init( Item (*item_array_function)(uint8_t), uint8_t size_without_cancel_item, void (*cancel_procedure)() = nullptr, int16_t std_position = 0){
			ItemManager::init(cancel_procedure, std_position);
			this->item_array = item_array_function;
			this->size = size_without_cancel_item;
		}
		
		void reset(){
			size = 0;
			item_array = nullptr;
		}
	};
	
}

#endif //__F_LIST_ITEM_MANAGER_H__
