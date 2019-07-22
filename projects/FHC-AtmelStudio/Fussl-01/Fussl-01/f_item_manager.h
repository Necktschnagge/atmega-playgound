/*
* f_item_manager.h
*
* Created: 05.03.2018 13:13:37
* Author: Maximilian Starke
*
*	FPS:	Everything alright. There may be some <<<< refactoring suggestions
*
*/


#ifndef __F_ITEM_MANAGER_H__
#define __F_ITEM_MANAGER_H__

#include <stdint.h>

namespace ui {

		/************************************************************************/
		/*	
		An ItemManager is an object that delivers information about items, i.e.
		their labels and their underlying effect functions, to an selection engine
		by offering an increment and decrement and output of label and callback function ptr.
		
		ItemManager is an abstract class which is supposed to be base class
		of any further item manager.
				
		pure virtual functions that you need to override are:
		getItemLabelInternal
		runItemProcedureInternal
		getSize
		[maybe] init
		overwritten init has to init its own stuff first and call init of super class afterwards.
		*/
		/************************************************************************/

	class ItemManager {		
		private:		
		void (*cancel_procedure)(); /* pointer to void terminated cancel function */

		/* return true <=> existing cancel procedure as item <=> cancelProcedure!=nullptr */
		inline bool has_cancel_item(){
			return cancel_procedure!=nullptr;
		}

		/* return whether the selected item is the cancel item*/
		inline bool on_cancel_item(){
			return (has_cancel_item()) && (position == size()-1);
		}
		
		/* return the amount of items (inclusive cancel item if present) (the mod to iterate) */
		inline int16_t size(){
			return has_cancel_item() + size_without_cancel_item();
		}
		
		protected:
		
		/* item position / current item */
		/* standard constraints: 0 <= position <= 32.001 */
		int16_t position;
		
		/* read the position and write the matching label to given pointer */
		virtual void item_label_virt(char* string_8_chars) = 0;
		// allow to give a param string length to get x characters, return whether string fits <<<<<<
		
		/* read the position and execute the matching item procedure */
		virtual void run_item_procedure_virt() = 0;
		
		/* constraint:  0 <= getSize() <= 32.000 */
		/* returns the amount of items (except the cancelItem)*/
		virtual uint16_t size_without_cancel_item() = 0;
		// explained: (2^15 - 3) (productive items) + 1 cancelItem + 1 (increasing) = must be less than 2^15 //
		// constraint:  0 <= getSize() <= 32.765 = 2^15 -3 to avoid fatal overflows //
		
		public:
		
		static constexpr char CANCEL_LABEL[] = "-CANCEL-";
		
		/* minimalistic constructor. You need to init data with init() */
		ItemManager() : cancel_procedure(nullptr), position(0) {}

		/* set the cancel procedure and maybe the starting position for selecting */
		inline virtual void init(void (*cancelProcedure)(), int16_t std_position = 0){
			this->cancel_procedure = cancelProcedure;
			this->position = std_position % size();
		}
		
		/* return true if there isn't any item, even no cancelItem */
		inline bool empty() {
			return size()==0;
		}
		
		/* run the cancel procedure if possible and return has_cancel_item() */
		inline bool run_cancel_procedure(){
			if (has_cancel_item()){	cancel_procedure();	return true;	} else {	return false;	}
		}
		
		/* increase the item position, warning: screen update is task of ItemSelector */
		ItemManager& operator++();
		
		/* decrease the item position, warning: screen update is task of ItemSelector */
		ItemManager& operator--();
		
		/* writes the current item label to the pointer position, string might be non-(null terminated) */
		void item_label(char* string_8_bytes);
		
		/* run the procedure of the selected item */
		void run_item_procedure();
	};

}

#endif //__F_ITEM_MANAGER_H__
