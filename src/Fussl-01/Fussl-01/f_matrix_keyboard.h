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
	
	enum class key_state {
		NONE_PRESSED = 0,
		ONE_PRESSED = 1,
		AMBIGUOUS = 2
	};
	
	struct return_type {
		key_state state;
		uint8_t x;
		uint8_t y;
		
		return_type() : state(key_state::NONE_PRESSED), x(0), y(0) {}
		
		inline void inc_state(){
			uint8_t x = static_cast<uint8_t>(state);
			state = static_cast<key_state>( 2 + 3 * x - x * x );
			/*
				0 -> 1
				1 -> 2
				2 -> 2
			*/
		}
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
	return_type operator()(){
		return_type result;
		for(hardware::IOPin iter = x_array_begin(); iter != x_array_end() ; ++ iter){
			iter.write_PORT(false);
			iter.set_as_output();
			for(hardware::IOPin jter = y_array_begin(); jter != y_array_end(); ++jter){
				if (jter.read_PIN() == false){
					result.inc_state();
					if (result.state == key_state::AMBIGUOUS) return result;
					result.x = iter - x_array_begin();
					result.y = jter - y_array_end();
				}
			}
		}
		return result;
	}
};

#endif //__F_MATRIX_KEYBOARD_H__
