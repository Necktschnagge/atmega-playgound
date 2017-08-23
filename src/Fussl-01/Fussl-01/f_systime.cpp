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
		
	inline constexpr long double to_precision(uint8_t log_precision){
		return static_cast<long double>(static_cast<uint32_t>(1)<<log_precision);
	} // implemented second time, see precision#() at SysTime.... <<<<<
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
		
	uint8_t SysTime::anti_racing_factor {4}; // <<<< this factor should be measured, it should be püossible to change this value.
		// it should have an std value like here, but the os programmer might want to change it.
		// the factor may be renamed. it is not only about that interrupt handling lasts longer than gap between two interrupts
		// also the time which is wasted in non-necessary time counting can have negative side effect to the actual task of the controller

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
	
	uint8_t SysTime::try_to_set(const long double& osc_frequency_new, uint8_t log_precision_wish){
			/**	
				SUMMARY:::
				with (e) ^ (f) ^ (g) we get a, b and c.
				
				select any precision_log such that (e) is fulfilled // wish of OS programmer / user
				if not g:	precision(_log) must be chosen greater
				if not f:	precision(_log) must be chosen smaller

				this all has to be checked when creating a SystemTime object!
				algorithm:
				get osc_freq (as copy);
				get wish_precision_log;
				
				if not e:
				log illegal precision_log wish
				set e := max := 16 // max precision
				2^8: counter
				while (!e or !f or !g):
				if not e or counter > 16:
				"return" error, no possible systime object.
				if not f and not g:
				"return" error, no possible systime object.
				if !f:
				--precision_log // this might change value of !g
				if !g: (else to !f)
				++precision_log
				++counter;
				whileEND
				
				construction complete
				"return" resulting precision_log
				
				*/
		/* local copies: */
		long double n_osc_freq = osc_frequency_new;
		uint8_t& n_log_pre = log_precision_wish; // call-by-value in function. no copy necessary
		
		/* given log_precision out of range */
		if (n_log_pre > 16) return 1;
		
		/* look how to fit the wish-log_precision: */
		uint8_t cloop {0}; // avoid an infinite loop, which we can not exclude and is potentially possible
		while(	(n_log_pre > 16) // not (e)
					||
				(static_cast<long double>(anti_racing_factor) * to_precision(n_log_pre) > n_osc_freq) // not (f)
					||
				(n_osc_freq > to_precision(n_log_pre) * static_cast<long double>((1LL<<16) - 2)) // not (g)
		){	// there is at least one violated constraint
			if (n_log_pre > 16) return 4;
			if (
				(static_cast<long double>(anti_racing_factor) * to_precision(n_log_pre) > n_osc_freq) // not f
					&&
				(n_osc_freq > to_precision(n_log_pre) * static_cast<long double>((1LL<<16) - 2)) // not g
			) return 3;
			
			if /* not f */ (static_cast<long double>(anti_racing_factor) * to_precision(n_log_pre) > n_osc_freq)
					--n_log_pre;
				else /* it est f and (not g)*/
					++n_log_pre;
			++cloop;
			if (cloop > 20){ return 2; } // could be about 16. I think exactly 17. 20 to avoid off by one
				// why 17?: you may need to ++ or -- n_log_pre 16 times. but then you need to check loop comndition once again
		}
		// we found valid values for n_osc_freq and n_log_pre:
		// replace the effective variables by our local try-something-copies:
		this->osc_frequency = n_osc_freq;
		this->log_precision = n_log_pre;
		return 0; // successful
	}
	
	uint16_t SysTime::get_compare_match_value_only_call_by_IRS(){
		long double exact = (osc_frequency / precision()) + residual_ticks; // always positive
		long double floored = floor(exact); // round down
		residual_ticks = exact - floored; // residual ticks left out by round down
		return static_cast<uint16_t>(floored);
	}

}
