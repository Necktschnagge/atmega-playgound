/*
 * f_hardware.h
 *
 * Created: 01.09.2016 15:10:01
 *  Author: Maximilian Starke
 */ 
/************************************************************************/
/* FPS: nothing to do, small functions
	<<< some day: please look at the declaration of _delay_ms in cpp
	and checkout whether it is possible to pipe higher arguments than 10 ms.
	                                                                    */
/************************************************************************/

/************************************************************************/
/* hardware contains some utilities often required
   you can just use them without anything to prepare
                                                                        */
/************************************************************************/


#include <stdint.h>

#ifndef F_HARDWARE_H_
#define F_HARDWARE_H_


namespace hardware {

	constexpr uint16_t EEPNULL {0xFFFF}; // nullptr for the EEPROM
	
		/* make a busy waiting time gap [ms] */
	void delay(uint16_t ms);
	
	/* return whether an eeprom address is out of range (NULL), throw an error if a non standard NULL address (!=EEPNULL) was used */
	bool isEEPNull(uint16_t address);

	/* copy a string from source to destination */
	/* it will only copy (count) characters and will add an '\0' if nullTerminated */
	/* the real string length of destination will be count + 1 (if nullTerminated) */
	/* but if source has an '\0' before copying will be stopped immediately */
	void copyString(char* destination, const char* source, uint8_t count, bool nullTerminated);
	
	/*template<int index>
	class IOPin {
		
		private:
		
		static constexpr uint8_t MAX {8*6 + 5 - 2}; // 53 - 2
			// BECAUSE OF 2 32kHz OSC INPUTS AT THESE PORTS
		
		public:
		
		IOPin<index> instance;
		
		IOPin();
		
		
		inline operator int() { return this->operator bool(); };
		
			* get input value *
		inline operator bool() {
			if (index < 8) {
				
			} else if (index < 16) {

			} else if (index < 16) {
				
			}
		}
		
		
			* set output or pull-up /
		void operator = (bool);
		
		
	};*/
	/*
		occupied pins
		PG3..4 are TOSC1..2 for external RTC clock osc.
		
	
	*/
	
	
	
}

#endif /* F_HARDWARE_H_ */