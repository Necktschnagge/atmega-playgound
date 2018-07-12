/*
* f_matrix_keyboard.h
*
* Created: 24.06.2018 17:29:49
* Author: Maximilian Starke

FPS:

	Everything is ready.
	
	Please write test code:
	Maybe for test code we can separate the iterator of IOPin from the context of the matrix, s.t. we provide an own iterator
	implementing the same methods to test this.
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
		
		if connect the way like in sketch, the button codes are
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
			fsl::hw::IOPin _x_array_begin;
			
			constexpr fsl::hw::IOPin array_begin(){ return _x_array_begin; }
			constexpr fsl::hw::IOPin array_end(){ return _x_array_begin + count_GPIOs(); }
			constexpr fsl::hw::IOPin x_array_begin(){ return _x_array_begin; }
			constexpr fsl::hw::IOPin x_array_end(){ return _x_array_begin + x_size; }
			constexpr fsl::hw::IOPin y_array_begin(){ return _x_array_begin + x_size; }
			constexpr fsl::hw::IOPin y_array_end(){ return _x_array_begin + count_GPIOs(); }
			
			public:
			
			/* the number of occupied GPIO pins */
			static constexpr uint8_t count_GPIOs(){ return x_size + y_size; }
			/* the number of (theoretical) keys on the matrix keyboard */
			static constexpr uint16_t key_count(){	return static_cast<uint16_t>(x_size) * y_size;	}
			/* the past-the-end iterator for the used GPIO pins*/
			constexpr fsl::hw::IOPin GPIO_end(){ return array_end(); } // <<< is constexpr allowed here???
			
			/* enum class for number of pressed keys */
			enum class key_down_count : uint8_t {
				NONE_PRESSED = 0,
				ONE_PRESSED = 1,
				AMBIGUOUS = 2
			};
			
			static constexpr key_down_count NONE_PRESSED {key_down_count::NONE_PRESSED};
			static constexpr key_down_count ONE_PRESSED {key_down_count::ONE_PRESSED};
			static constexpr key_down_count AMBIGUOUS {key_down_count::AMBIGUOUS};
			
			/* class describing the state of the key matrix at one point in time */
			// <<< this struct is independent of all tmeplate params of nesting class. so might be moved outerwards?? or replace the uint8_t with some range_int???
			//  <<<<< usingh range int might be unnecessary overhead? ... or could otherwise we could remove the key_down_count conter; (????) 
			struct key_state {
				key_down_count counter; // "amount" of pressed keys: NONE / ONE / AMBIGUOUS
				uint8_t x; // x position of the ONE key that was pressed (if so)
				uint8_t y; // y position of the ONE key that was pressed (if so)
				
				key_state() : counter(key_down_count::NONE_PRESSED), x(0), y(0) {}
				
				/* increments counter, ambiguous will be fixed by this operation */
				/*	NONE_PRESSED	|-> ONE_PRESSED
					ONE_PRESSED		|-> AMBIGUOUS
					AMBIGUOUS		|-> AMBIGUOUS
				*/
				inline void inc_counter(){
					uint8_t x = static_cast<uint8_t>(counter);
					counter = static_cast<key_down_count>( (2 + 3 * x - x * x) / 2 );
					//		0 -> 1,		1 -> 2,		2 -> 2
				}
				
				/* conversion to bool: evaluates to true if and only if one and only one key is pressed */
				inline operator bool(){	return counter==key_down_count::ONE_PRESSED;	}
				
				/* operator (): returns the key code for the key that was pressed */
				inline uint16_t operator()(){	return static_cast<uint16_t>(x) + static_cast<uint16_t>(x_size) * y;	}
			};
			
			/* x_array_begin points to first GPIOPin of x-row/col pins, y pins follow immediately
			reserved pins are {x_array_begin, ... , x_array_begin + x_size + y_size -1 }
			past-the-end iterator can be accessed via GPIO_end()
			make sure this class is exclusively using these pins
			*/
			inline matrix_keyboard(fsl::hw::IOPin x_array_begin) : _x_array_begin(x_array_begin) {
				for(fsl::hw::IOPin iter = array_begin(); iter != array_end(); ++ iter){
					iter.set_as_input(); // get a safe state first
					iter.set_pull_up(true); // activate pull-up
				}
			}
			
			/* read the current key state of matrix_keyboard */
			key_state operator()(){
				key_state result;
				for(fsl::hw::IOPin iter = x_array_begin(); iter != x_array_end() ; ++ iter){
					iter.write_PORT(false);	// i.e. deactivate pullup, goal: output 0V
					iter.set_as_output(); // output 0 on pin iter (x-panel)
					for(fsl::hw::IOPin jter = y_array_begin(); jter != y_array_end(); ++jter){
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
