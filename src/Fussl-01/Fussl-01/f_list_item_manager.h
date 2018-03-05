/*
* f_list_item_manager.h
*
* Created: 05.03.2018 14:28:39
* Author: Maximilian Starke
*
*	FPS: everything ready.
*/


#ifndef __F_LIST_ITEM_MANAGER_H__
#define __F_LIST_ITEM_MANAGER_H__

#include "f_item_manager.h"
#include "f_hardware.h"

namespace ui {
	
	/* This is an ItemManager that takes a list of labels
	and a list of corresponding callback pointers */
	class ListItemManager final: public ItemManager {
		public:
		
		/* one item consisting of one label and one callback */
		struct Item {
			char label[8]; // non-null-terminated string
			void (*callback)();
			
			Item(): label{'\0'}, callback(nullptr){}
		};

		/* function that helps to read continuously stored labels and callbacks */
		/* consider [size] Items that are stored the following way...
		   [labels] is a string with all item labels separated with '\n', ending with \0,
		   [callbacks] is a pointer to an array of function pointers corresponding to these labels
		   [index] is the index of a specific Item.
		   Then this function picks out the Item at position [index], i.e.
		   grab the label as well as callback, put them into an Item object an return it.
		   If given [index] >= [size], a standard Item will be returned (empty string + nullptr)
		   */
		static Item item_list_decoder(uint8_t index, uint8_t size, const char* labels, void (* const *callbacks)());
		
		/* example how to use item_list_decoder */
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
		
		/* Function pointer to a function returning the Item at given index. */
		/* This way extern functions provides all the data needed here. */
		Item (*item_array)(uint8_t index);
		
		/* size of list (no cancel item is encountered) */
		uint8_t size;
		
		protected:
		
		/* provide item label for superclass */
		virtual void item_label_virt(char* string_8_bytes) override {
			if (item_array != nullptr) return hardware::copyString(string_8_bytes,&(item_array(position).label[0]),8,false);//this copying should be done in the item array
		}
		
		/* provide callback function pointer for superclass */
		virtual void run_item_procedure_virt() override {
			if (item_array != nullptr) return item_array(position).callback();
		}
		
		/* provide size for superclass */
		virtual uint16_t size_without_cancel_item() override {
			return size;
		}
		
		public:
		
		/* minimalistic std c-tor. use init() for initialization */
		ListItemManager() : ItemManager() , item_array(nullptr), size(0){}
		
		/* init the item manager, set array function, list size, cancel event callback, and position in list where to start */
		virtual void init( Item (*item_array_function)(uint8_t), uint8_t size_without_cancel_item, void (*cancel_procedure)() = nullptr, int16_t std_position = 0){
			this->item_array = item_array_function;
			this->size = size_without_cancel_item;
			ItemManager::init(cancel_procedure, std_position);
		}
		
		/* reset item manager to right after construction. 
		   run init() next. */
		void reset(){
			size = 0;
			item_array = nullptr;
		}
	};
}

#endif //__F_LIST_ITEM_MANAGER_H__
