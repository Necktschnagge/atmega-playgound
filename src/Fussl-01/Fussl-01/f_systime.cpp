/*
 * f_systime.cpp
 *
 * Created: 22.08.2017 18:41:52
 *  Author: Maximilian Starke
 
 FPS: see at header file!
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
	
	if (scheduler::SysTime::get_instance().now_update_interrupt_handler != nullptr) scheduler::SysTime::get_instance().now_update_interrupt_handler();
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
		if (!(osc_frequency_new > 0.0)) return 5;
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
				// why 17?: you may need to ++ or -- n_log_pre 16 times. but then you need to check loop condition once again
		}
		// we found valid values for n_osc_freq and n_log_pre:
		// replace the effective variables by our local try-something-copies:
		macro_interrupt_critical(
			this->osc_frequency = n_osc_freq;
			this->log_precision = n_log_pre;
		);
		return 0; // successful
	}
	
	SysTime::SysTime(const long double& osc_frequency, uint8_t& log_precision, uint8_t& error_code, concepts::void_function handler) : now_update_interrupt_handler(handler) {
		error_code = try_to_set(osc_frequency,log_precision);
		if (error_code){ // mark object as internally invalid
			this->log_precision = INVALID_log_precision;
		}
		log_precision = this->log_precision;
	}

	uint16_t SysTime::get_compare_match_value_only_call_by_IRS(){
		if (log_precision == INVALID_log_precision){
			pause();
			return 0xFFFF;
		}
		long double exact = (osc_frequency / precision()) + residual_ticks; // always positive
		long double floored = floor(exact); // round down
		residual_ticks = exact - floored; // residual ticks left out by round down
		return static_cast<uint16_t>(floored);
	}

}


/************************************************************************/
/* 
history:
	There was a previous half-ready implementation before.
	But I decided to renew this one, because it used a kind of 'refinement'
	which was an additional frequency = real freq. - ideal freq. 
	after all implementing, I found this kinda weird.
	So this is a new implementation with only one frequency value for oscillator


concept:

there are 3 main entities to deal with:
	osc_freq:
		Frequency of the oscillator.
		Unit: Hz
	
	precision:
		Frequency on which the system now time is updated
		Unit: Hz
	
	Compare Match Value (CMV):
		The Value on which the timer must cause an interrupt
		definition equation:
			CMV := round_down( osc_freq / precision + residual_ticks )
		Unit: uint16_t (via TCNT...) 
		
	residual_ticks:
		Ideally CMV would be (osc_freq / precision), but CMV has to be integral.
		So we round_down CMV to be integral and need to carry broken part.
		definition equation:
			residual_ticks@after := trunc(osc_freq / precision + residual_ticks@before)
		it is alway a value in interval [0,1), exactly [0,1] because of technical floating point format
		Unit: 1 (no Unit)

Constraints:
	osc_freq			is given by hardware. programmer has no influence
	
	precision			may be a wish of operating system designer
					(a)	must be chosen such that we can add 1/precision to an ExtendedMetricTime
	
	CMV				(b)	must be an unsigned 16 bit integer [1,...,2^16 -1]
					(c)	it should be not too small (1,2,3..??) because ISR needs some time to be executed.
	residual_ticks		(no restrictions)
	
Evaluation of constraints:
	(d) => (a) where
		(d)	precision in {2^0, 2^1, 2^2, ..., 2^16}
			because:	EMT enables refinement up to 1/2^16 second
						the potencies of 2 are not all possible values but enough, I think,
						and easy to work with when increasing system now time
						1s (=2^0) is no necessary l_border, but nobody wan't schedulers with
						operating in 1s tact.
						
			so we use a var named precision_log of type uint8_t
	(def)		we define precision := 2^precision_log
	(e) => (d)   and   (d) => (a) where
		(e)	precision_log in {0,1,.., 16}
			
	(f) and (g)  =>  (b) and (c) where
		(get f via...)				1*x <= CMV
			<==			x <= round_down( osc_freq / precision + residual_ticks )
			<==			x <= round_down( osc_freq / precision ) // residual is 0..1
			<==			x <= osc_freq / precision
			<==			x * precision <= osc_freq
		(f)		// via x which should be any integer value >=1 (but not too large) we provide
				// the constraint (c) depending on a parameter.
		
		(get g via...)				CMV  <=  2^16 - 1
			<==			round_down( osc_freq / precision + residual_ticks ) <= 2^16 - 1	
			<==			round_down( osc_freq / precision + 1 ) <= 2^16 - 1
			<==			round_down( osc_freq / precision ) + 1 <= 2^16 - 1
			<==			round_down( osc_freq / precision ) <= 2^16 - 2
			<==			osc_freq / precision <= 2^16 - 2
			<==			osc_freq <= (2^16 - 2) * precision
		(g)
		
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
			// no i decided to abort at this point
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
		

C++ DESIGN				
what about the static and the non-static stuff ???

if singleton, the instance must be a static class member not a static function local var because of this interrupt problem

I want a constructor where you pass the osc_freq and
		neither a macro passing to header file nor changing any constexpr in the library
So I am for a public constructor and
	a way that only one (of maybe n constructed SysTime objects) can be "linked" to the interrupt routine
*/