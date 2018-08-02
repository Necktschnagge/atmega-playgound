/*
* f_ui_concept.h
*
* Created: 16.07.2018 20:11:41
* Author: F-NET-ADMIN
*/


#ifndef __F_UI_CONCEPT_H__
#define __F_UI_CONCEPT_H__

#include "f_callbacks.h"

#include <stdint.h>

namespace fsl {
	
	namespace ui {
		
		class element {
			public:
			virtual element* work() = 0;
		};
		
		class controller : public fsl::str::callable {
			element* current_element;
			fsl::con::write_through_buffer<char,20> ostream; // better idea???: something independent from the buffer size
			fsl::con::write_through_buffer<char, 20> istream; //
			
			void operator()() override {
				current_element = current_element->work();
			}
		};
		
		/* just output stuff */
		class monolog : public element {
			
		};
		
		/* read input buffer and probably use monolog elements */
		class dialog : public element {
			
		};
		
		using char_to_bool_function = bool (* )(char);
		
		template<char_to_bool_function UP_KEY, char_to_bool_function DOWN_KEY, char_to_bool_function OK_KEY, char TEXT_SEPARATOR>
		class item_selector : public dialog {
			const char* item_texts; // separated with TEXT_SEPARATOR
			element* successor_elements;
			uint8_t position;
			
			public:
			item_selector(const char* item_texts, element* successor_elements, uint8_t position = 0) : item_texts(item_texts), successor_elements(successor_elements), position(position) {}
			
			element* work() override {
				if (!istream )
				char x 
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
			
			element* work() override {
				(*callable)();
				return successor;
			}
		};
		
		class systime_tick_delay : public element {
			uint16_t ticks; // uinion with timer specific information?
			
			// go to scheduler, disable our own task.
			// write a new timer to when we begin again
			element* work() override {
				
			}
		};
		
		class delay_end : public element {
			
			
		};
		#if false

		controller my_controller;
		

		#endif






	}
	
}



#endif //__F_UI_CONCEPT_H__
