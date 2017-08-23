/*
 * f_systime.h
 *
 * Created: 22.08.2017 18:41:29
 *  Author: Maximilian Starke
 */ 


#ifndef F_SYSTIME_H_
#define F_SYSTIME_H_

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
what abóut the static and the non-static stuff ???

if singleton, the instance must be a static class member not a static function local var because of this interrupt problem

I want a constructor where you pass the osc_freq and
		neither a macro passing to header file nor changing any constexpr in the library
So I am for a public constructor and
	a way that only one (of maybe n constructed SysTime objects) can be "linked" to the interrupt routine
                                                     */
/************************************************************************/

#include "f_macros.h"
#include "f_time.h"

/************************************************************************/
/* TYPE AND FUNCTION DECLARATION
                                                                        */
/************************************************************************/


namespace scheduler {
	
	class SysTime {
		
		/******************   STATIC STUFF   **********************/
	private /*static*/:
	
			/* pointer to the one and only class instance which is encountered by ISR */
		static SysTime* p_instance;
	
	public /*static*/:
		
			/* return reference to the instance which is / has to be clocked by ISR */
		inline static SysTime& get_instance() {	return *p_instance;	}
		
			/* set reference (pointer) to the SystemTime object which should be clocked by ISR */
			/* deactivates interrupts during replacing of reference */
		inline static void link(SysTime& instance){	macro_interrupt_critical( p_instance = &instance; ); }

			/*	start running SysTime clock and return true, if timer wasn't running and reference is non-null
				returns false, if timer already running from some clock source or instance pointer is nullptr
				in this way... a save maybe-start */
		static bool start();
			
			/* pause the SysTime clock
			   i.e. just deactivate clock source */
		inline static void pause();
		
			/* start SysTime clock again after a pause()
			   i.e. just activate clock source */
		inline static void resume();
		
			/* stop running SysTime clock */
		inline static void stop();
		
			/* returns true if the timer used by SysTime is running (in anyone's context) */
		inline static bool is_running(){	return (TCCR1B & 0b00000111);	}
		
		
		/******************   NON-STATIC STUFF   **********************/
	private:
			/* the real oscillator frequency / Hz, critical since ISR access */
		long double osc_frequency;
		
			/* the truncated / broken part of the last compare match value, critical since ISR access */
		long double residual_ticks;
		
			/* logarithmic value of precision, critical since ISR access */
		uint8_t log_precision;
		
			/* now is accessed by ISR and user/operating system. it is critical */
		time::ExtendedMetricTime now;
		
		// <<< if ready just look how it works. maybe festkomma floats stored in integers are better....
		// <<< try this later.
		
			/* precision / Hz, critical since ISR access */
		inline long double precision(){ return static_cast<long double>(static_cast<uint32_t>(1)<<log_precision); }
			
	public:
	/******* constructors *************************************************************************/
		SysTime(const long double& osc_frequency, uint8_t& log_precision, uint8_t& error_code);
		
		
	/******* modifiers  **************************************************************************/
	
			/* change osc_frequency */
		inline void set_osc_frequency(const long double& osc_frequency) = delete;
		// {	/*macro_interrupt_critical( this->osc_frequency = osc_frequency );*/	} //## reconstruct because of restictions necessary
			
			/* try to change precision to wish-value */
		inline uint8_t change_log_precision() = delete; //{	macro_interrupt_critical( /* */ );	} //#### see set_osc_freq.
		
		
	/****** interrupt methods ******************************************************************/
				
			/* method which tells ISR what compare value should be used next */
			/* only to call by ISR */
		uint16_t get_compare_match_value_only_call_by_IRS();
			
			/* method which is called by ISR every time a compare match occurs */
			/* should only called by ISR */
		void operator++(){}; //###
		
		
	/****** getters for now time *************************************************************/
			
			/* read only access to raw now value. attention. access is interrupt-critical! */
		inline const time::ExtendedMetricTime& get_reference_to_critical_now_time(){	return now;	}
		
			/* get system time (same as operator()) */
		inline time::ExtendedMetricTime get_now_time();
		
			/* get system time (same as get_now_time) */
		inline time::ExtendedMetricTime operator()() { return get_now_time(); }
	};



/************************************************************************/
/* INLINE FUNCTION IMPLEMENTATION
                                                                        */
/************************************************************************/
	
	/* static class member */
	inline void SysTime::pause(){
		TCCR1B &= 0b11111000; // timer stopped. because of no clock source
	}
	
	inline void SysTime::resume(){
		TCCR1B = 0b00001111; // start timer. set clock source to extern oscillator.
		// (also activates CTC again, which is not deactivated by pause() )
	}
	
	inline void SysTime::stop(){
		TCCR1B = 0b00000000; // timer stopped. because of no clock source
		TIMSK &= 0b11000011; // deactivate interrupt flag for CompareMatch A
	}
	
	/* non-static class member */
	
	inline time::ExtendedMetricTime SysTime::get_now_time(){
		time::ExtendedMetricTime copy;
		macro_interrupt_critical(copy = now;);
		return copy;
	}

	
} // namespace
#endif /* F_SYSTIME_H_ */