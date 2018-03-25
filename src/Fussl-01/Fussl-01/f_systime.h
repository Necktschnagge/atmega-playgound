/*
 * f_systime.h
 *
 * Created: 22.08.2017 18:41:29
 *  Author: Maximilian Starke
 
 FPS:	Lib is at RC state. It is suposed to be stable. No bugs found yet.
		do the following enhancements:
 
		<<< see modifier in class: they are declkared asa = delete. this can be done in future
		<<< make descriptions a bit better / more details
		<<< make another theoretical check of lib.
		<<< docu seem somewhere lost, also cponcept stuff in *cpp file.
		
 */ 


#ifndef F_SYSTIME_H_
#define F_SYSTIME_H_

#include "f_concepts.h"

 /*                                                   
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
			/* don't call, if you did not link an instance */
			/* this will end in undefined behavior */
		inline static SysTime& get_instance() { return *p_instance; }
		
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
			// <<<< using fixed point number we need to define the exponent, either as constant, or as template parameter
		
			/* the truncated / broken part of the last compare match value, critical since ISR access */
		long double residual_ticks {0};
			// <<< if we store this as an fixed point number, is should be [0, 2), so 100000* -> 1,  111111111* ->  max value, <ca 2
		
			/* logarithmic value of precision, critical since ISR access */
			/* a SysTime object is marked as invalid if log_precision == 255 */
		uint8_t log_precision {0};
		static constexpr uint8_t INVALID_log_precision {255};
		
			/* the actual system time, which is updated by this lib / the timer */
			/* begins counting at 0 when SysTime is initialized. for encoding see time::EMT */
			/* now is accessed by ISR and user/operating system. it is critical */
		time::ExtendedMetricTime now {0};
			
		// <<< if ready just look how it works. maybe fixed point floats stored in integers are better....
		// <<< try this later.
		
			/* precision / Hz, critical since ISR access */
		inline long double precision() const { return static_cast<long double>(static_cast<uint32_t>(1)<<log_precision); }
		
			/* try to set osc_frequency and log_precision to given values */
			/* returns error codes:
				0	...		no error
				1	...		given log_precision out of range {0,1,...,16}
				2	...		no possible adapted log_precision, more detail: log_p is toggled between two values either (f) or (g) evaluates false, but !f and !g are not overlapping
				3	...		no possible adapted log_precision, more detail: taboo zones of (!f) and (!g) overlapping
				4	...		no possible adapted log_precision, more detail: either !f or !g covers whole range of log_precision
				5	...		non-positive osc_frequency given
			
			   if 0 was returned osc_frequency and log_precision are set to your given osc_frequency and some
			   log_precision that is valid for the osc_freq and is nearest to your wished value
			   otherwise osc_frequency and log_precision are not modified. */
			/* this function is not interrupt critical. It cares about this itself. */
		uint8_t try_to_set(const long double& osc_frequency_new, uint8_t log_precision_wish);
		
	public:

		concepts::void_function now_update_interrupt_handler;
		
	/******* constructors *************************************************************************/
		
			/* tries to construct a SysTime object with given values 
			   "returns" an error code. for detail see description of try_to_set(..).
			   the constructed object is only internally valid if error_code returns 0,
			   but it behaves also stable if you created an intern invalid one, then it does just nothing.
			   the log_precision passed might be changed in order to construct a valid SysTime
			   so the final value of log_precision is passed back. */
			/* on error log_precision is INVALID_log_precision */
		SysTime(const long double& osc_frequency, uint8_t& log_precision, uint8_t& error_code, concepts::void_function handler = nullptr);
		
		
	/******* modifiers  **************************************************************************/
	
			/* change osc_frequency */
		inline uint8_t set_osc_frequency(const long double& new_osc_frequency){
			return try_to_set(new_osc_frequency,log_precision);
			// directly using log_precision is non-critical since (it is one byte only and) ISR only reads it
		}
			
			/* try to change precision to wish-value */
		inline uint8_t change_log_precision(uint8_t& new_log_precision){
			return try_to_set(osc_frequency,new_log_precision); // read - read access means no interrupt conflicts
		}
				
	/****** interrupt methods ******************************************************************/
				
			/* method which tells ISR what compare value should be used next */
			/* only to call by ISR */
			/* if object marked invalid it pause()s timer and returns 0xFFFF */
		uint16_t get_compare_match_value_only_call_by_IRS();
			
			/* method which is called by ISR every time a compare match occurs */
			/* should only be called by ISR */
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
		this->now = this->now + (1LL << (16 - log_precision)); // <<<< wait for += operator in f_time and change it then
	}
	
	inline time::ExtendedMetricTime SysTime::get_now_time(){
		time::ExtendedMetricTime copy;
		macro_interrupt_critical(copy = now;);
		return copy;
	}

} // namespace
#endif /* F_SYSTIME_H_ */