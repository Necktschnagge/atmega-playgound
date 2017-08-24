/*
 * f_systime.h
 *
 * Created: 22.08.2017 18:41:29
 *  Author: Maximilian Starke
 
 FPS:
		<<< see modifier in class: they are declkared asa = delete. this can be done in future
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
                                                     
documentation:

how to use this library:

This library offers two main things.
	I	creating / handling a SysTime object
	II	activate / deactivate 16bit Timer1 of the controller and link it to a SysTime object.

(I) is represented by all non-static members of class SysTime, (II) by all static ones.

Steps to do, in order to build a running SysTime clock:
	1	First create a SysTime object
		(! check ctor description and check error information returned )
	2	use the (static) link(..) method to tell the timer engine about your SysTime object.
		it gets a persistent reference to your object.
	3	start() the time with static start() method
		(! check the return value of start() and it's description of course )
	4	Get the sysTime value whenever you want by using operator() of your SysTime object.
		e.g.
		time::EMT before = my_sysTime();
		some_method_which_needs_some_time();
		auto after  my_sysTime();
		// duration of function: after - before

There are a few other methods you may use, but in normal / easy cases,
where you just want to start a SysTime once and recurrently get the now time,
you will not need them.
		
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
		
			/* a factor which makes a condition stronger for a SysTime to be valid.
			   it says what the Compare Match Value can be at minimum */
		static uint8_t anti_racing_factor;
	
			/* return reference to the instance which is / has to be clocked by ISR */
			/* don't call, if you did not link an instance, it is called by running timer */
			/* so don't run timer before you linked a SysTime object (this is checked by start() too) */
		inline static SysTime& get_instance() {	return *p_instance;	}
			// <<< what about making this save
		
			/* set reference (pointer) to the SystemTime object which should be clocked by ISR */
			/* deactivates interrupts during replacing of reference, just in case timer is already running
			   and someone tries to change the affected SysTime object (despite this should not be done) */
		inline static void link(SysTime& instance){	macro_interrupt_critical( p_instance = &instance; ); }

			/*	start running SysTime clock and return true, if timer wasn't running and reference ptr is non-null
				(only) returns false, if timer already running from some clock source or instance pointer is nullptr
				start is more like a save maybe-start */
		static bool start();
			
			/* pause the SysTime clock
			   i.e. just deactivate clock source
			   you should know in general in what state your timer are, since it doesn't check before situation */
			/* pause [may be used / should be used only] sometimes after a start(), but before stop() */
		inline static void pause();
		
			/* start SysTime clock again after a pause()
			   i.e. just activate clock source to external input
			   you should know in general in what state your timer are, since it doesn't check before situation */
		inline static void resume();
		
			/* stop running SysTime clock (if running or also if paused) */
			/* i.e. deactivate clock source and clear the flags for CompareMatch Interrupt
			   it is not possible to restart a stop()ped SysTime via resume()
			   but you can call start() again, it will do this "resume after a stop()" */
		inline static void stop();
		
			/* returns true if the timer used by SysTime is running (in anyone's context) */
		inline static bool is_running(){	return (TCCR1B & 0b00000111);	}
		
		
		/******************   NON-STATIC STUFF   **********************/
	private:
			/* the real oscillator frequency / Hz, critical since ISR access */
		long double osc_frequency {1};
		
			/* the truncated / broken part of the last compare match value, critical since ISR access */
		long double residual_ticks {0};
		
			/* logarithmic value of precision, critical since ISR access */
			/* a SysTime object is marked as invalid if log_precision == 255 */
		uint8_t log_precision {0};
		static constexpr uint8_t INVALID_log_precision {255};
		
			/* the actual system time, which is updated by this lib / the timer */
			/* begins counting at 0 when SysTime is initialized. for encoding see time::EMT */
			/* now is accessed by ISR and user/operating system. it is critical */
		time::ExtendedMetricTime now {0};
		
		// <<< if ready just look how it works. maybe festkomma floats stored in integers are better....
		// <<< try this later.
		
			/* precision / Hz, critical since ISR access */
		inline long double precision(){ return static_cast<long double>(static_cast<uint32_t>(1)<<log_precision); }
		
			/* try to set osc_frequency and log_precision to given values */
			/* returns error codes:
				0	...		no error
				1	...		given log_precision out of range {0,1,...,16}
				2	...		no possible adapted log_precision, more detail: log_p is toggeled between two values either (f) or (g) evaluates false, but !f and !g are not overlapping
				3	...		no possible adapted log_precision, more detail: tabu zones of (!f) and (!g) overlapping
				4	...		no possible adapted log_precision, more detail: either !f or !g covers whole range of log_precision
				5	...		non-positiv osc_frequency given
			
			   if 0 was returned osc_frequency and log_precision are set to your given osc_frequency and some
			   log_precision that is valid for the osc_freq and is nearest to your wished value
			   otherwise osc_frequency and log_precision are not modified. */
		uint8_t try_to_set(const long double& osc_frequency_new, uint8_t log_precision_wish);
		
	public:
	/******* constructors *************************************************************************/
		
			/* tries to construct a SysTime object with given values 
			   "returns" an error code. for detail see description of try_to_set(..).
			   the constructed object is only internally valid if error_code returns 0,
			   but it behaves also stable if you created an intern invalid one, then it does just nothing.
			   the log_precision passed might be changed in order to construct a valid SysTime
			   so the final value of log_precision is passed back. */
			/* on error log_precision is INVALID_log_precision */
		SysTime(const long double& osc_frequency, uint8_t& log_precision, uint8_t& error_code);
		
		
	/******* modifiers  **************************************************************************/
	
			/* change osc_frequency */
		inline uint8_t set_osc_frequency(const long double& new_osc_frequency){
			return try_to_set(new_osc_frequency,log_precision);
		}
			
			/* try to change precision to wish-value */
		inline uint8_t change_log_precision() = delete; //{	macro_interrupt_critical( /* */ );	} //<<<<<see set_osc_freq.
		
		
	/****** interrupt methods ******************************************************************/
				
			/* method which tells ISR what compare value should be used next */
			/* only to call by ISR */
			/* if object marked invalid it pause()s timer and returns 0xFFFF */
		uint16_t get_compare_match_value_only_call_by_IRS();
			
			/* method which is called by ISR every time a compare match occurs */
			/* should only called by ISR */
			/* increases now time by a value depending on (log_)precision */
		inline void operator++();
		
		
	/****** getters for now time *************************************************************/
			
			/* read only access to raw now value. attention. access is interrupt-critical! */
			/* for safety: use operator() or get_now_time() instead */
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
	
	inline void SysTime::operator ++(){
		if (log_precision == INVALID_log_precision) return; // <<< make an inline is_valid function!
			// necessary since we cannot << to a negative number .... i suppose
		this->now += 1LL << (16 - log_precision);
	}
	
	inline time::ExtendedMetricTime SysTime::get_now_time(){
		time::ExtendedMetricTime copy;
		macro_interrupt_critical(copy = now;);
		return copy;
	}

} // namespace
#endif /* F_SYSTIME_H_ */