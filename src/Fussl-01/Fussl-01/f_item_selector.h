/*
* f_item_selector.h
*
* Created: 05.03.2018 13:05:54
* Author: Maximilian Starke
*
* FPS: everything ready and manually checked.
*		some <<< improvements may be found
*
*/


#ifndef __F_ITEM_SELECTOR_H__
#define __F_ITEM_SELECTOR_H__

#include "f_concepts.h"
#include "f_item_manager.h"

namespace ui {

	/************************************************************************/
	/* HOW TO USE:
	1.	Create an ItemSelector.
	2.	init() the ItemSelector.
	3.	run() the ItemSelector
	[	you may pause() and resume() the ItemManager several times	]
	4.	finalize the ItemSelector yourself or
	let it be finalized automatically by an ok button event.
	*/
	/************************************************************************/
	
	class ItemSelector final {
		public:
		static constexpr uint8_t NO_BUTTON {0xFF};
		static constexpr uint16_t DELAY_ON_OKAY{ 333 }; // time
		static constexpr uint16_t DELAY_ON_START{ 1000 }; // time for "SELECT" screen
		
		private:
		/* Singleton for OK Button callback */
		class OkayCall final : public concepts::Callable {
			private:
			ItemSelector* host;
			OkayCall(ItemSelector* itemSelector) : host(itemSelector) {}
			
			public:
			virtual void operator()() final override;
			friend class ItemSelector;
		};
		
		/* Singleton for NEXT Button callback */
		class NextCall final : public concepts::Callable {
			private:
			ItemSelector* host;
			NextCall(ItemSelector* itemSelector) : host(itemSelector) {}
			
			public:
			virtual void operator()() final override;
			friend class ItemSelector;
		};
		
		/* Singleton for PREVIOUS Button callback */
		class PreviousCall final : public concepts::Callable {
			private:
			ItemSelector* host;
			PreviousCall(ItemSelector* itemSelector) : host(itemSelector) {}
			public:
			virtual void operator()() final override;
			friend class ItemSelector;
		};
		
		// Callbacks which are given to the buttons:
		const OkayCall okay_call {this}; /// <<<< if any error on compilation. the callable method is non const. so i might be forbidden to call this method since object is const.
		const NextCall next_call {this};
		const PreviousCall previous_call {this};
		
		// item manager which supports access to item data
		ItemManager* p_item_manager{ nullptr };
		
		// button codes:
		uint8_t button_okay {NO_BUTTON};
		uint8_t button_next {NO_BUTTON};
		uint8_t button_prev {NO_BUTTON};
		
		concepts::Flags flags; // one object to store multiple flags:
		/* defined flags in this class */
		static constexpr concepts::Flags::flag_id OK_DOWN {0}; // ok was pressed
		static constexpr concepts::Flags::flag_id IS_RUNNING {1}; // run() was called, not yet stopped
		
		/* check whether buttons and ItemManager* are valid */
		inline bool valid();
		
		/* enable button events (for internal use only) */
		/* setEvent for ok_bttn AND (next_bttn OR prev_bttn) */
		/* button_down is used. the button_ups will be disabled */
		void activate_button_events();
		
		/* disable the NEXT, PREVIOUS & OKAY methods associated to the buttons (for internal use only) */
		void deactivate_button_events();

		/* return false if selector already running */
		/* returns false if there is no item given and no valid cancel_procedure */
		/* returns false if you [init()]ed illegal buttons */
		/* returns true if the selector was started */
		bool run(bool show_select_screen);
		
		public:
		
		/* minimalistic constructor. please init data via init() */
		ItemSelector(){}
		
		/* init variables, the ItemManager* is just a pointer to an ItemManager. You have to care about construction, destruction yourself */
		/* return true if everything was initialized. returns false if could not init because still running */
		inline bool init(ItemManager* itemManager, uint8_t button_okay, uint8_t button_next, uint8_t button_prev);
		// we should catch if init is called while selector is running !!! ####
		
		/* stop the ItemSelector */
		/* disable button events (for using programmer) */
		inline void finalize(){
			// compulsory
			flags.set_false(IS_RUNNING); // reset running flag
			deactivate_button_events(); // -> reset button events
			button_okay = NO_BUTTON; // -> invalidate ItemSelector s.t. it cannot be startet via run() or resume()
			// optional (...)
			/*{
			button_next = NO_BUTTON;
			button_prev = NO_BUTTON;
			itemManager = nullptr;
			}*/
		}
		
		/* stop the ItemSelector but keep all data to start running again from current state */
		inline void pause(){
			flags.set_false(IS_RUNNING); // reset running flag
			deactivate_button_events();
		}
		
		/* wrapper: resume after pause() [means calling run() again] */
		inline bool resume(){
			return run(false);
		}
		
		/* print the current item label (...where position of ItemManager is pointing to) */
		///<<<<<< make this function virtual, create a new class for ledline-printing ItemSelectors
		// you do not need to use this. But maybe it could be useful if you build an extern screen saver.
		// if selector is not running it does nothing
		void print_item();

		/* method for the class user to call to start the selecting engine */
		/* return false if selector already running */
		/* returns false if there is no item given and no valid cancel_procedure */
		/* returns false if you [init()]ed illegal buttons */
		/* returns true if the selector was started */
		bool start(){
			return run(true);
		}
	};
	
}

#endif //__F_ITEM_SELECTOR_H__
