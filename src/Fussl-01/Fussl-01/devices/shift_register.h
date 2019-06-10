/*
 * shift_register.h
 *
 * Created: 10.06.2019 20:19:39
* Author: Maximilian Starke
 */ 


#ifndef SHIFT_REGISTER_H_
#define SHIFT_REGISTER_H_

#include "f_iopin.h"

namespace fsl {
	namespace dev {
		
		class shift_register {
			constexpr void* signal_delay; // what type??
		private:
			void set_data_bit(bool bit);
			void send_clock_signal();
			void send_latch_signal();
			
			// set data bit
			// clock
			// latch
			
			// length of shift register? push an entire bit sequence? endian? integer value?
			// keep in memory what was pushed?
			
		};
		
		class custom_pin_shift_register : public shift_register {
			
		private:
			gpio_pin data;
			gpio_pin clock;
			gpio_pin latch;
		};
		
		class bundle_pin_shift_register : public shift_register {
			
		private:
			gpio_pin data;
			inline gpio_pin clock(){ return data + 1; }
			inline gpio_pin latch(){ return data + 2; }
			
			void set_data_bit(bool bit){ data = bit; }
			void send_clock_signal(){ clock() = true; /* delay */; clock() = false; }
			void send_latch_signal(){ latch() = true; /*delay*/; latch() = false; }
			
		};
		
		
		
	}
}



#endif /* SHIFT_REGISTER_H_ */