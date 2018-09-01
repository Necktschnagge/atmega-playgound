/*
* f_interrupt.h
*
* Created: 28.08.2018 21:36:28
*  Author: F-NET-ADMIN
*/


#ifndef F_INTERRUPT_H_
#define F_INTERRUPT_H_

#include <avr/interrupt.h>

namespace fsl {
	
	namespace hw {
		
		// <<< we may jsut read the global interrupts enable flag bit and save this together with the active in one byte. This way we get standard_atomic to 1 byte size jsut like simple_atomic.
		class lite_atomic {
			
		};
		
		/* An standard_atomic is a more advanced variant of a simple_atomic. You may see the description of simple_atomic to get familiar with the concept.
		   An standard_atomic can on the one hand disable interrupts and save the MCUs interrupt configuration register SREG in a copy
		   and on the other hand restore the MCUs interrupt configuration register from the copy saved.
		   An standard_atomic (in difference to simple_atomic) may block interrupts and unblock them again as often as desired.
		*/
		class standard_atomic {
			
			bool _active;
			uint8_t sreg;
			
			inline void initialize(){ sreg = SREG; cli(); }
			inline void finalize(){ if (_active) SREG = sreg; }
			
			public:
			
			inline standard_atomic() : _active(true), sreg(SREG) { cli(); }
			inline standard_atomic(bool active) : _active(active) { if (active) initialize(); }
			inline ~standard_atomic() { finalize(); }
			
			standard_atomic(const standard_atomic&) = delete;
			standard_atomic& operator = (const standard_atomic&) = delete;
			
			inline standard_atomic(standard_atomic&& obj) : _active(obj._active), sreg(obj.sreg) { obj._active = false; }
			inline standard_atomic& operator = (standard_atomic&& obj) { finalize(); _active = obj._active; sreg = obj.sreg; obj._active = false; return *this; }
			
			inline void open(){ if (!_active) { initialize(); _active = true; } }
			inline void p(){ open(); }
			inline void close(){ finalize(); _active = false; }
			inline void v(){ close(); }
			
			inline bool active(){ return _active; }
		};

		/* If you need an atomic section, i.e. a snipped of code that is guaranteed to not be preempted from any interrupt, you may use an simple_atomic object.
		   The only things you can do with an simple_atomic is to create it and let it become destroyed.
		   On creation the simple_atomic turns off interrupts and saves the MCUs configuration before turning off.
		   On destruction the simple_atomic restores the MCUs configuration about interrupts, i.e. reenables the interrupts if it has really disabled them on creation.
		   */
		class simple_atomic {
			
			uint8_t sreg;
			
			public:
			
			inline simple_atomic() : sreg(SREG) { cli(); }
			inline ~simple_atomic() { SREG = sreg; }
			
			simple_atomic(const simple_atomic&) = delete;
			simple_atomic& operator = (const simple_atomic&) = delete;
			
			inline simple_atomic(simple_atomic&& obj) = delete;
			inline simple_atomic& operator = (simple_atomic&& obj) = delete;
			
		};
		
		/* anti-atomic */
		// << variant to turn on interrupts for some section inside a section where interrupts are disabled (perhaps by an atomic.)
		class lite_cimota {
			// standard with one_byte solution;
		};
		
		class standard_cimota {
			
		};
		
		class simple_cimota {
			
		};
	}
}


#endif /* F_INTERRUPT_H_ */