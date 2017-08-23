/*
 * f_systime.cpp
 *
 * Created: 22.08.2017 18:41:52
 *  Author: Maximilian Starke
 */ 
#include <avr/interrupt.h> // ISR
#include <math.h> // floor

#include "f_systime.h" // corresponding header

namespace {
	inline auto get_cmv() -> decltype(scheduler::SysTime::get_instance().get_compare_match_value_only_call_by_IRS())
	{	return scheduler::SysTime::get_instance().get_compare_match_value_only_call_by_IRS();	}
}
	/* interrupt subroutine */
ISR (TIMER1_COMPA_vect){
	// set the compare-match value for the next interrupt
	// update the *SystemTime::p_activated_instance
	
	OCR1A = get_cmv(); 
	
	// a problem would be if the calculation of the new compare match is not ready until the next osc rising edge
	// ##detect this. ### because gcmv needs heavy floting point division
	
	++scheduler::SysTime::get_instance();
}

namespace scheduler {

	SysTime* SysTime::p_instance {nullptr};

	bool SysTime::start(){
		if (is_running() || (p_instance == nullptr)) return false;
			// timer is already running in any modus
				// maybe because it was already started by SysTime lib
				// or someone else is already using this timer/counter
			// or there is a nullptr reference as p_instance
		
		cli(); // deactivate global interrupts
		TCCR1A = 0x00; // ~ NO PWM
		TCNT1 = 0; // counter starts at zero
		
		TIMSK = (TIMSK & 0b11000011) | 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
		OCR1A = get_cmv(); // register with compare value
		
		TCCR1B = 0b00001111; // CTC (Clear Timer on Compare Match) (bit 3)
							 // start timer running from ext source on triggered rising edge (bit 2:0)
		
		sei();	// activate global interrupts
		return true;
	}
	
	uint16_t SysTime::get_compare_match_value_only_call_by_IRS(){
		long double exact = (osc_frequency / precision()) + residual_ticks; // always positive
		long double floored = floor(exact); // round down
		residual_ticks = exact - floored; // residual ticks left out by round down
		return static_cast<uint16_t>(floored);
	}

}
