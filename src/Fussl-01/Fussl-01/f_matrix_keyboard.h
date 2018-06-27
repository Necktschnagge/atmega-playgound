/*
* f_matrix_keyboard.h
*
* Created: 24.06.2018 17:29:49
* Author: Maximilian Starke
*/


#ifndef __F_MATRIX_KEYBOARD_H__
#define __F_MATRIX_KEYBOARD_H__

#include <stdint.h>

#include "f_iopin.h"

namespace fsl {
	namespace ui {
		
		/*
		reads input from a key matrix
			
x_array_begin	2	3	x_size
		->	|	|	|	|
			|	|	|	|
			|	|	|	|
		   -x---x---x---x----	1
			|	|	|	|
			|	|	|	|
		   -x---x---x---x----	2
			|	|	|	|
			|	|	|	|
		   -x---x---x---x----	3
			|	|	|	|
			|	|	|	|
		   -x---x---x---x----	y_size
			|	|	|	|
		x is a button that connect row i with col j when it is pressed
		
		if connect the way like in sketch, the button code are
			 0	 1	 2	 3
			 4	 5	 6	 7
			 8	 9	10	11
			12	13	14	15
		*/
		template <uint8_t x_size, uint8_t y_size>
		class matrix_keyboard {
			
			static_assert(static_cast<uint16_t>(x_size) + static_cast<uint16_t>(y_size) < 255, "matrix size too large, x_size + y_size < 256 must hold");
			
			private:
			/* points to first GPIOPin of x-row/col pins, y pins follow immediately
			reserved pins are {x_array_begin, ... , x_array_begin + x_size + y_size -1 }
			*/
			hardware::IOPin _x_array_begin;
			
			constexpr uint8_t count_GPIOs(){ return x_size + y_size; }
			constexpr hardware::IOPin array_begin(){ return _x_array_begin; }
			constexpr hardware::IOPin array_end(){ return _x_array_begin + count_GPIOs(); }
			constexpr hardware::IOPin x_array_begin(){ return _x_array_begin; }
			constexpr hardware::IOPin x_array_end(){ return _x_array_begin + x_size; }
			constexpr hardware::IOPin y_array_begin(){ return _x_array_begin + x_size; }
			constexpr hardware::IOPin y_array_end(){ return _x_array_begin + count_GPIOs(); }
			
			public:
			
			constexpr uint16_t key_count(){	return static_cast<uint16_t>(x_size) * y_size;	}
			
			enum class key_down_count {
				NONE_PRESSED = 0,
				ONE_PRESSED = 1,
				AMBIGUOUS = 2
			};
			
			struct key_state {
				key_down_count counter;
				uint8_t x;
				uint8_t y;
				
				key_state() : counter(key_down_count::NONE_PRESSED), x(0), y(0) {}
				
				/* increments counter, ambiguous will be fixed by this operation */
				inline void inc_counter(){
					uint8_t x = static_cast<uint8_t>(counter);
					counter = static_cast<key_down_count>( (2 + 3 * x - x * x) / 2 );
					//		0 -> 1,		1 -> 2,		2 -> 2
				}
				
				/* evaluates to true if and only if one and only one key is pressed */
				inline operator bool(){	return counter==key_down_count::ONE_PRESSED;	}
				
				/* returns the key code for the key that was pressed */
				inline uint16_t operator()(){	return static_cast<uint16_t>(x) + static_cast<uint16_t>(x_size) * y;	}
			};
			
			/* x_array_begin points to first GPIOPin of x-row/col pins, y pins follow immediately
			reserved pins are {x_array_begin, ... , x_array_begin + x_size + y_size -1 }
			make sure this class is exclusively using these pins
			*/
			matrix_keyboard(hardware::IOPin x_array_begin) : _x_array_begin(x_array_begin) {
				for(hardware::IOPin iter = array_begin(); iter != array_end(); ++ iter){
					iter.set_as_input(); // get a safe state
					iter.set_pull_up(true);
				}
			}
			
			/* read the current key state of matrix_keyboard */
			key_state operator()(){
				key_state result;
				for(hardware::IOPin iter = x_array_begin(); iter != x_array_end() ; ++ iter){
					iter.write_PORT(false);	// goal: output 0V
					iter.set_as_output();
					for(hardware::IOPin jter = y_array_begin(); jter != y_array_end(); ++jter){
						if (jter.read_PIN() == false){
							result.inc_counter();
							result.x = iter - x_array_begin();
							result.y = jter - y_array_end();
						}
					}
					iter.set_as_input();
					iter.write_PORT(true);	// enable intern pull up
				}
				return result;
			}
		};


	}
}
#endif //__F_MATRIX_KEYBOARD_H__
