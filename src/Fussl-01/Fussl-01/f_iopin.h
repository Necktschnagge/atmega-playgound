/*
* f_iopin.h
*
* Created: 13.03.2018 17:41:04
* Author: Maximilian Starke
*
*	FPS:
everything seems to be fine, complete and working properly.
doc strings seem to be complete
*/


#ifndef __F_IOPIN_H__
#define __F_IOPIN_H__

#include <stdint.h>
#include <avr/io.h>

#include "f_range_int.h"

namespace fsl {
	namespace hw {
		
		/* an IOPin object is something like an iterator to a GPIO pin */
		/* it only contains an index value referring to one of the controller's GPIO pins */
		class IOPin {
			public:
			
			/*** public constants, types and constexpr functions ***/
			
			/* aviablable pins are {0, .., MAX-1} */
			/* this value should be adapted to the constraints of the controller */
			static constexpr uint8_t MAX {8*6 + 5 - 2};
			/*
			// <<< this should be converted into a template argument,
			// <<< one could define synonyms for each controller
			<<<<< there are 53 programmable I/O-Lines (also see summary of ATMEGA128's data sheet)
			for our project we subtract 2, because PG3, PG4 are used as TOSC1/2 for ext RTC-OSC source (32kHz OSC).
			*/
			
			/* Pin ID type.
			The id is the number interpreted as index over all GPIOPins */
			using id = fsl::lg::range_int<uint8_t,MAX,true,true>;
			
			/* the controller's ports */
			enum class Port : uint8_t {
				A = 0, B = 8, C = 16, D = 24, E = 32, F = 40, G = 48
			};
			
			/* data direction enum class */
			enum class DD : bool {
				IN = false, OUT = true
			};
			
			/* id of the no-IOPin object */
			static constexpr id NO_IOPIN_ID{ id::OUT_OF_RANGE };
			
			/* return the no-IOPin object */
			static constexpr IOPin NO_IOPIN(){ return IOPin(IOPin::NO_IOPIN_ID); }
			
			/* calculates pin_id of given port and bit inside port */
			inline static constexpr id pin_id(Port port, uint8_t bit){
				return (bit>7) ? NO_IOPIN_ID : id(static_cast<uint8_t>(port) + bit);
			}
			
			/* calculates corresponding port to given id
			undefined result if _id is NO_IOPIN_ID */
			inline static constexpr Port port(id _id){
				return static_cast<Port>((_id / 8) * 8); // round down to mutiples of 8
			}
			
			/* returns bit position of _id inside it's port */
			inline static constexpr uint8_t bit_inside_port(id _id){
				return _id % 8;
			}
			
			private:
			
			/* unique id for one particular GPIO Pin,
			the only data of an IOPin instance */
			id _id;
			
			public:
			
			/*** c-tors ***/
			
			/* construct IOPin with given pin id */
			inline constexpr IOPin(id _id): _id(_id) {}
			
			/* construct IOPin with given port and bit of port */
			inline constexpr IOPin(Port port, uint8_t bit_of_port) : _id(pin_id(port,bit_of_port)) {}
			
			/*** Validness and Invalidness of IOPin objects ***/

			/* returns true if and only if this is referring to an IOPin */
			inline constexpr bool valid()	const { return _id != NO_IOPIN_ID; }
			/* returns true if and only if this is referring to none of the IOPins */
			inline constexpr bool invalid()	const { return _id == NO_IOPIN_ID; }
			
			/*** GPIO Accessors and Modifiers ***/
			
			/* read input value that is measured at this IOPin */
			inline bool read_PIN() const {
				const uint8_t pin_select = (static_cast<uint8_t>(1) << bit_inside_port(_id));
				if (_id < 8)  return static_cast<bool>(PINA & pin_select);
				if (_id < 16) return static_cast<bool>(PINB & pin_select);
				if (_id < 24) return static_cast<bool>(PINC & pin_select);
				if (_id < 32) return static_cast<bool>(PIND & pin_select);
				if (_id < 40) return static_cast<bool>(PINE & pin_select);
				if (_id < 48) return static_cast<bool>(PINF & pin_select);
				if (_id < 56) return static_cast<bool>(PING & pin_select);
				
				/* this region should never be reached */
				return true;
			}
			
			/* set DDR (data direction register) value of this IOPin */
			inline void write_DD(DD data_direction) const {
				const uint8_t pin_select = (static_cast<uint8_t>(1) << bit_inside_port(_id));
				if (data_direction == DD::OUT){
					if (_id <  8) { DDRA |= pin_select; return; }
					if (_id < 16) { DDRB |= pin_select; return; }
					if (_id < 24) { DDRC |= pin_select; return; }
					if (_id < 32) { DDRD |= pin_select; return; }
					if (_id < 40) { DDRE |= pin_select; return; }
					if (_id < 48) { DDRF |= pin_select; return; }
					if (_id < 56) { DDRG |= pin_select; return; }
				}
				else // if (data_direction == DD:IN)
				{
					if (_id <  8) { DDRA &= ~pin_select; return; }
					if (_id < 16) { DDRB &= ~pin_select; return; }
					if (_id < 24) { DDRC &= ~pin_select; return; }
					if (_id < 32) { DDRD &= ~pin_select; return; }
					if (_id < 40) { DDRE &= ~pin_select; return; }
					if (_id < 48) { DDRF &= ~pin_select; return; }
					if (_id < 56) { DDRG &= ~pin_select; return; }
				}
				
				/* this region should never be reached */
			}
			
			/* read DDR (data direction register) / data direction of this IOPin */
			inline DD read_DD() const {
				const uint8_t pin_select = (static_cast<uint8_t>(1) << bit_inside_port(_id));
				if (_id < 8)  return static_cast<DD>(static_cast<bool>(DDRA & pin_select));
				if (_id < 16) return static_cast<DD>(static_cast<bool>(DDRB & pin_select));
				if (_id < 24) return static_cast<DD>(static_cast<bool>(DDRC & pin_select));
				if (_id < 32) return static_cast<DD>(static_cast<bool>(DDRD & pin_select));
				if (_id < 40) return static_cast<DD>(static_cast<bool>(DDRE & pin_select));
				if (_id < 48) return static_cast<DD>(static_cast<bool>(DDRF & pin_select));
				if (_id < 56) return static_cast<DD>(static_cast<bool>(DDRG & pin_select));
				
				/* this region should never be reached */
				return DD::OUT; // virtual false (output pin) on error
			}
			
			/* write associated bit in register PORT, i.e.
			set the output value if this->is_output()
			enable(value == true) / disable(value == false) the internal pull_up if this->is_input() */
			inline void write_PORT(bool value) const {
				const uint8_t pin_select = (static_cast<uint8_t>(1) << bit_inside_port(_id));
				if (value){
					if (_id <  8) { PORTA |= pin_select; return; }
					if (_id < 16) { PORTB |= pin_select; return; }
					if (_id < 24) { PORTC |= pin_select; return; }
					if (_id < 32) { PORTD |= pin_select; return; }
					if (_id < 40) { PORTE |= pin_select; return; }
					if (_id < 48) { PORTF |= pin_select; return; }
					if (_id < 56) { PORTG |= pin_select; return; }
					} else {
					if (_id <  8) { PORTA &= ~pin_select; return; }
					if (_id < 16) { PORTB &= ~pin_select; return; }
					if (_id < 24) { PORTC &= ~pin_select; return; }
					if (_id < 32) { PORTD &= ~pin_select; return; }
					if (_id < 40) { PORTE &= ~pin_select; return; }
					if (_id < 48) { PORTF &= ~pin_select; return; }
					if (_id < 56) { PORTG &= ~pin_select; return; }
				}
				/* this region should never be reached */
			}
			
			/* read associated bit in PORT register, see also description of write_PORT() */
			inline bool read_PORT() const {
				const uint8_t pin_select = (static_cast<uint8_t>(1) << bit_inside_port(_id));
				if (_id < 8)  return static_cast<bool>(PORTA & pin_select);
				if (_id < 16) return static_cast<bool>(PORTB & pin_select);
				if (_id < 24) return static_cast<bool>(PORTC & pin_select);
				if (_id < 32) return static_cast<bool>(PORTD & pin_select);
				if (_id < 40) return static_cast<bool>(PORTE & pin_select);
				if (_id < 48) return static_cast<bool>(PORTF & pin_select);
				if (_id < 56) return static_cast<bool>(PORTG & pin_select);
				
				/* this region should never be reached */
				return true;
			}
			
			/*** GPIO Accessors and Manipulators: more high level ***/
			
			inline void set_as_output()			const	{	write_DD(DD::OUT);				}
			inline void set_as_input()			const	{	write_DD(DD::IN);				}
			inline bool is_input()				const	{	return read_DD() == DD::IN;		}
			inline bool is_output()				const	{	return read_DD() == DD::OUT;	}

			inline void set_pull_up(bool is_on)	 const	{	write_PORT(is_on);				}
			inline bool read_input()			 const	{	return read_PIN();				}
			inline void write_output(bool value) const	{	write_PORT(value);				}
			
			/*** overloaded operators for GPIO Pin Access and Modification ***/
			
			/* returns input value */
			inline operator bool()		const { return  read_PIN(); }
			/* returns negated input value */
			inline bool operator ! ()	const { return !read_PIN(); }
			
			/* set associated bit inside associated PORT register */
			inline void operator = (bool value)		const { return write_PORT(value); }
			/* use it like an output stream with operator <<,
			set associated bit inside associated PORT register */
			inline void operator << (bool value)	const { return write_PORT(value); }
			/* use it like an input stream with operator >>,
			reads input value */
			inline void operator >> (bool& target)	const { target = read_PIN(); }
			
			
			/*** overloaded operators for object modification ***/
			
			/* behavior in general:
			* whenever an overflow / underflow happens, NO_IOPIN is returned
			* whenever left operand (i.e. the IOPin) is already NO_IOPIN (call it invalid), result is also NO_IOPIN and parameter difference will be ignored
			*/
			inline constexpr IOPin operator + (uint8_t difference) const { return IOPin(this->_id + difference); }
			inline constexpr IOPin operator - (uint8_t difference) const { return IOPin(this->_id - difference); }
			
			inline IOPin& operator ++ () { ++_id; return *this; }
			inline IOPin operator ++ (int) { IOPin copy(*this); ++_id; return copy; }
			inline IOPin& operator -- () { --_id; return *this; }
			inline IOPin operator -- (int) { IOPin copy(*this); --_id; return copy; }
			
			
			inline IOPin& operator = (IOPin rop) { this->_id = rop._id; return *this; }
			inline constexpr bool operator == (IOPin rop) const { return this->_id == rop._id; }
			inline constexpr bool operator != (IOPin rop) const { return this->_id != rop._id; }
			
			inline IOPin& operator += (uint8_t difference) { return invalid() ? *this : *this = *this + difference; } // think about what behaviour we want
			inline IOPin& operator -= (uint8_t difference) { return invalid() ? *this : *this = *this - difference; }
		};
		
	}

}
#endif //__F_IOPIN_H__
