/*
* f_ui_concept.h
*
* Created: 16.07.2018 20:11:41
* Author: Maximilian Starke
*/


#ifndef __F_UI_CONCEPT_H__
#define __F_UI_CONCEPT_H__

#include "f_callbacks.h"
#include "f_buffer.h"
#include <stdint.h>

namespace fsl {
	
	namespace ui {
		
		class controller;
		
		class element {
			public:
			virtual element* work(controller& c) = 0;
		};
		
		class controller : public fsl::str::callable {
			element* current_element;
			fsl::con::wt_buffer<char>* ostream;
			fsl::con::wt_buffer<char>* istream;
			
			public:
			inline controller(element& first_ui_element, fsl::con::wt_buffer<char>* in, fsl::con::wt_buffer<char>* out) : current_element(&first_ui_element), ostream(out), istream(in) {}
			inline virtual void operator()() override { current_element = current_element->work(*this); }
			
			inline fsl::con::wt_buffer<char>& out() const { return *ostream; }
			inline fsl::con::wt_buffer<char>& in() const { return *istream; }
		};
		
		/* just output stuff, i.e. does not save any values calculated from input
		and does not invoke individual actions dependent on input,
		but may read input for the purpose of waiting until ok pressed */
		class monolog : public element {
			
		};

		template<char OK>		
		class ok_monolog {
		};
		
		template<char OK>
		class string_output : public ok_monolog<OK> {
			const char* string;
			element* successor;
			enum class state { SHOW_MESSAGE, WAIT_OK };
			state my_state;
			
			public:
			inline string_output(const char* string, element* successor) : string(string), successor(successor) {}
			
			inline void reset(){ my_state = state::SHOW_MESSAGE; }
				
			inline void set_string(const char* string){ this->string = string; }

			inline virtual element* work(controller& c){
				if (my_state == state::SHOW_MESSAGE){
					c.out() << "\f" << string;
					my_state = state::WAIT_OK;
				} else {
					if ((!c.in().empty()) && (c.in().read() == OK)) {
						reset();
						return successor;
					}
				}
				return *this;
			}
		};
		
		/* read input buffer to create values from input stream or invoke different actions depending on input */
		class dialog : public element {
		};
		
		using char_to_bool_function = bool (* )(char);
		
		template<char_to_bool_function UP_KEY, char_to_bool_function DOWN_KEY, char_to_bool_function OK_KEY, char TEXT_SEPARATOR>
		class item_selector : public dialog {
			const char* item_texts; // separated with TEXT_SEPARATOR, max 256 items
			element* successor_elements;
			uint8_t position;
			
			uint8_t count_items() const {
				uint8_t count{ 1 };
				for (auto iter = item_texts; *iter != '\0'; ++iter){
					if (*iter == TEXT_SEPARATOR) ++count;
				}
				return count;
			}
			
			void update_screen(controller& c){
				c.out() << "\fSELECT:\n";
				uint8_t counter{ position };
				for (auto iter = item_texts; *iter != '\0'; ++iter){
					counter = counter - (*iter == TEXT_SEPARATOR);
					if (counter == 0) {
						auto jter = iter + 1;
						while( (*jter != TEXT_SEPARATOR) && (*jter != '\0') ) ++jter;
						char old = *jter; // but it is const, use some print string (const char*, size_t)#####
						*jter = '\0';
						c.out() << (iter + 1);
						*jter = old;
					}
				}
			}
			
			public:
			item_selector(const char* item_texts, element* successor_elements, uint8_t position = 0) : item_texts(item_texts), successor_elements(successor_elements), position(position) {}
			
			
			
			element* work(controller& c) override {
				if (!c.in().empty()){
					char ch = c.in().read();
					if ( OK_KEY(ch) ){
						return successor_elements[position];
					}
					if ( UP_KEY(ch) ){
						position = (position + 1) % count_items();
						update_screen(c);
					}
					if ( DOWN_KEY(ch) ){
						position = (static_cast<uint16_t>(position) + count_items() -1) % count_items();
						update_screen(c);
					}
				}
				return this;
			}
		};
		
		template <typename int_type>
		class integer_input : public dialog {
			
		};
		
		template <uint8_t string_size>
		class string_input : public dialog {
			
		};
		
		class action : public element {
			element* successor;
			fsl::str::callable* callable;
			
			element* work(controller& c) override {
				(*callable)();
				return successor;
			}
		};
		
		class systime_tick_delay : public element {
			uint16_t ticks; // uinion with timer specific information?
			
			// go to scheduler, disable our own task.
			// write a new timer to when we begin again
			element* work(controller& c) override {
				
				return this;
			}
		};
		
		class delay_end : public element {
			
			
		};
		#if false

		controller my_controller;
		

		#endif






	}
	
}

//#### allow interupting task: a task that will be called on every systime update!!!


#endif //__F_UI_CONCEPT_H__
