/* 
* f_iopin.h
*
* Created: 13.03.2018 17:41:04
* Author: F-NET-ADMIN
*/


#ifndef __F_IOPIN_H__
#define __F_IOPIN_H__

#include <stdint.h>
#include <avr/io.h>

#include "f_range_int.h"

namespace hardware {
	
		class IOPin {
		public:
		
		static constexpr uint8_t MAX {8*6 + 5 - 2};
		/* there are 53 programmable I/O-Lines (also see summary of ATMEGA128's data sheet)
		valid pin ids are all in {0, 1, ... , MAX - 1} !!!
		
		for our project we subtract 2, because PG3, PG4 are used as TOSC1/2 for ext RTC-OSC source (32kHz OSC).
		*/
		
		//using PINID = uint8_t;
		using PINID = range_int<uint8_t,MAX,true>;
		
		
		enum class Port : uint8_t {
			A = 0, B = 8, C = 16, D = 24, E = 32, F = 40, G = 48
		};
		
		enum class DD : uint8_t {
			IN = 0, OUT = 1
		};
		
		
		static constexpr PINID NO_IOPIN{ PINID::OUT_OF_RANGE };
		
		/* calculates pin_id of given port and bit inside port
		with undefined inputs, result is undefined */
		inline static constexpr PINID get_pin_id(Port port, uint8_t bit){
			return static_cast<uint8_t>(port) + bit;
		}
		
		/* with illegal pin_id, result my be undefined */
		inline static constexpr Port get_port_from_pin_id(PINID::base_type pin_id){
			return static_cast<Port>((pin_id / 8) * 8);
		}
		
		/* with illegal pin_id, result my be undefined */
		inline static constexpr uint8_t get_bit_inside_port_from_pin_id(PINID pin_id){
			return pin_id % 8;
		}
		
		private:
		
		PINID pin_id; // unique id for one particular GPIO Pin
				
		public:
		
		/* c-tors */
		
		IOPin(PINID pin_id): pin_id(pin_id) {}
		
		IOPin(Port port, uint8_t bit) : pin_id(get_pin_id(port,bit)) {}
		
		
		/* GPIO Accessors and Modifiers */
		
		inline bool read_PIN() const {
			const uint8_t pin_select = (static_cast<uint8_t>(1) << (pin_id % 8));
			
			if (pin_id < 8)  return static_cast<bool>(PINA & pin_select);
			if (pin_id < 16) return static_cast<bool>(PINB & pin_select);
			if (pin_id < 24) return static_cast<bool>(PINC & pin_select);
			if (pin_id < 32) return static_cast<bool>(PIND & pin_select);
			if (pin_id < 40) return static_cast<bool>(PINE & pin_select);
			if (pin_id < 48) return static_cast<bool>(PINF & pin_select);
			if (pin_id < 56) return static_cast<bool>(PING & pin_select);
			
			/* this region should never be reached */
			return true; // virtual true (pull up) on error
		}
		
		/* allow casting from to bool <-> DD*/
		inline void write_DD(DD data_direction) const {
		///#####
		}
		
		inline DD read_DD() const {
			const uint8_t pin_select = (static_cast<uint8_t>(1) << (pin_id % 8));
			
			if (pin_id < 8)  return static_cast<DD>(static_cast<bool>(DDRA & pin_select));
			if (pin_id < 16) return static_cast<DD>(static_cast<bool>(DDRB & pin_select));
			if (pin_id < 24) return static_cast<DD>(static_cast<bool>(DDRC & pin_select));
			if (pin_id < 32) return static_cast<DD>(static_cast<bool>(DDRD & pin_select));
			if (pin_id < 40) return static_cast<DD>(static_cast<bool>(DDRE & pin_select));
			if (pin_id < 48) return static_cast<DD>(static_cast<bool>(DDRF & pin_select));
			if (pin_id < 56) return static_cast<DD>(static_cast<bool>(DDRG & pin_select));
			
			/* this region should never be reached */
			return DD::OUT; // virtual false (output pin) on error
		}
		
		inline void write_PORT(bool value) const {
			///######
		}
		
		inline bool read_PORT() const {
			const uint8_t pin_select = (static_cast<uint8_t>(1) << (pin_id % 8));
			
			if (pin_id < 8)  return static_cast<bool>(PORTA & pin_select);
			if (pin_id < 16) return static_cast<bool>(PORTB & pin_select);
			if (pin_id < 24) return static_cast<bool>(PORTC & pin_select);
			if (pin_id < 32) return static_cast<bool>(PORTD & pin_select);
			if (pin_id < 40) return static_cast<bool>(PORTE & pin_select);
			if (pin_id < 48) return static_cast<bool>(PORTF & pin_select);
			if (pin_id < 56) return static_cast<bool>(PORTG & pin_select);
			
			/* this region should never be reached */
			return false; // virtual false on error
		}
		
		
		/* overloaded operators for GPIO Pin Access and Modification ****************************************/
		
		inline operator bool() const { return read_PIN(); }

		inline void operator = (bool value) const { return write_PORT(value); }
		inline void operator << (bool value) const { return write_PORT(value); }
		inline void operator >> (bool& target) const { target = read_PIN(); }
		
		
		/* overloaded operators for object modification *****************************************************/
		
		inline IOPin operator + (uint8_t difference) const { return IOPin(this->pin_id + difference); }
			// make opportunity to check whether we are a NO_PIN associated IOPIN
		inline IOPin operator - (uint8_t difference) const { return IOPin(this->pin_id - difference); }
		
		inline IOPin& operator ++ () { ++pin_id; return *this; }
		inline IOPin operator ++ (int) { IOPin copy(*this); ++pin_id; return copy; }
		inline IOPin& operator -- () { --pin_id; return *this; }
		inline IOPin operator -- (int) { IOPin copy(*this); --pin_id; return copy; }
		
		
		inline IOPin& operator = (IOPin rop) { this->pin_id = rop.pin_id; return *this; }
		inline bool operator == (IOPin rop) const { return this->pin_id == rop.pin_id; }
			
		inline IOPin& operator += (uint8_t difference) { return *this = *this + difference; }
		inline IOPin& operator -= (uint8_t difference) { return *this = *this - difference; }
		
	};
}
#endif //__F_IOPIN_H__
