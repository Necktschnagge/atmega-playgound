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
		
		class controller {
			element* current_element;
			
			void operator()(){
				current_element = current_element->work();
			}
			};
		
		/* just output stuff */
		class monolog : public element {
			
		};
		
		/* read input buffer and probably use monolog elements */
		class dialog : public element {
			
			};
			
		class item_selector : public dialog {
			
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
			
		class delay : public element {
			
			};

#if false

	controller my_controller;
	

#endif






	}
	
}



#endif //__F_UI_CONCEPT_H__
