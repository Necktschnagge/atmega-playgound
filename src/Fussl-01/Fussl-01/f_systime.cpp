/*
 * f_systime.cpp
 *
 * Created: 22.08.2017 18:41:52
 *  Author: Maximilian Starke
 */ 
#include <avr/interrupt.h>
#include "f_systime.h"

namespace {
	inline auto get_cmv() -> decltype(scheduler::SystemTime::get_instance().get_compare_match_value())
	{	return scheduler::SystemTime::get_instance().get_compare_match_value();	}
}
	/* interrupt subroutine */
ISR (TIMER1_COMPA_vect){
	// set the compare-match value for the next interrupt
	// update the *SystemTime::p_activated_instance
	
	OCR1A = get_cmv(); 
	
	// a problem would be if the calculation of the new compare match is not ready until the next osc rising edge
	// ##detect this.
	
	++scheduler::SystemTime::get_instance();
}

bool scheduler::SystemTime::start(){
	if (TCCR1B & 0b00000111) return false; // already started
		
	cli(); // deactivate global interrupts
	
	TCCR1A = 0x00; // ~ NO PWM
	TCNT1 = 0; // counter starts at zero
	
	TIMSK = (TIMSK & 0b11000011) | 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
	
	OCR1A = get_cmv(); // register with compare value
	
	TCCR1B = 0b00001111; // CTC (Clear Timer on Compare Match) and start timer running from ext source on,
	//triggered rising edge
	
	sei();	// activate global interrupts
	return true;
}


void scheduler::SystemTime::stop(){
	TCCR1B = 0b00000000; // timer stopped / no clock signal
	//	TIMSK &= 0b11000011;	// cancel the ISR reference
}
