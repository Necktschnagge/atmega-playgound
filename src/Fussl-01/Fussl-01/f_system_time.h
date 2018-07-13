/*
* f_system_time.h
*
* Created: 22.08.2017 18:41:29
*  Author: Maximilian Starke

FPS:	Lib is at RC state. It is suposed to be stable. No bugs found yet.
do the following enhancements:

<<< think about the error that comes when we always add a rounded const to now time to increase it. the error sums up. better multiply if it is really a problem.
/// for this I may write some test code, whether there comes up a difference.
// i don not think so since we add potences of 2
// but we could also: store a constexpr time zero, store the uint64_t number of increases. and calculate the the via mutiply on each get_now_time.

<<< you should not but actually can call the static methods pause, resume, start, stop in arbitrary order, and nothing is checked for integrity.
please check where we should prevent mistakes of using programmer.

<<< 	use fixed point comma integer value fro frequency, etc., osc_fequency, residual_ticks
// <<<< using fixed point number we need to define the exponent, either as constant, or as template parameter


<< we use compare match a for our interrupts, we should check ovverrun -> min_compare_match_value
<< therefor we may use compare match b and check, whether we get interrupted there?
// a problem would be if the calculation of the new compare match is not ready until the next osc rising edge
// ##detect this. ### because gcmv needs heavy floting point division
// the bigger problem is the time consumed by now_update_interrupt_handler!#####
// it might be possible to auto-detect if this happens.
next four line: description at definition in *cpp file:
// <<<< this factor should be measured, it should be püossible to change this value.
// it should have an std value like here, but the os programmer might want to change it.
// the factor may be renamed. it is not only about that interrupt handling lasts longer than gap between two interrupts
// also the time which is wasted in non-necessary time counting can have negative side effect to the actual task of the controller

<<< description: it is only for my controller model. make stuff more exchangable.




<<<< conclusion of FPS:
only make a documentation of the stuff written at top of header and bottom of cpp.
AND only check once the try_to_set method with the reviewed documentation.

done: all checks about variables being volatile

*/


#ifndef F_SYSTIME_H_
#define F_SYSTIME_H_

#include "f_callbacks.h"
#include "f_macros.h"
#include "f_time.h"

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

set min_compare_macth_value if necessary.

2	use the (static) link(..) method to tell the timer engine about your SysTime object.
it gets a persistent reference to your object.
3	start() the timer with static start() method
(! check the return value of start() and it's description of course )
4	Get the sysTime value whenever you want by using operator() of your SysTime object.
e.g.
time::EMT before = my_sysTime();
some_method_which_needs_some_time();
auto after  my_sysTime();
// duration of function: after - before

5
There are a few other methods you may use, but in normal / easy cases,
where you just want to start a SysTime once and recurrently get the now time,
you will not need them.

*/
/************************************************************************/



/************************************************************************/
/* TYPE AND FUNCTION DECLARATION
*/
/************************************************************************/


namespace fsl {
	namespace os {
		
		class system_time {
			
			/******************   STATIC STUFF   **********************/
			private /*static*/:
			
			/* pointer to the one and only SysTime instance which is affected ISR */
			/* ISR is accessing this. -> every access is critical. */
			/* design: it is only a volatile pointer to a non-volatile object since
			all SysTime properties care itself about their volatileness */
			static system_time* volatile p_instance;
			
			public /*static*/:

			/* the minimum allowed value for the compare match register of timer1 */
			/* if you allow arbitrary small Compare Match Values, among other possible side effects
			ISR may take too much time in comparison to the time between two Compare Matches */
			/* must not be used by any interrupt routine since it is not volatile */
			static uint8_t min_compare_match_value;
			
			/* return reference to the instance which is / has to be clocked by ISR */
			/* don't call, if you did not link an instance before, this may cause undefined behaviour */
			/* access is interrupt-critical. */
			inline static system_time& get_instance() { return *p_instance; }
			
			/* set reference (pointer) to the SystemTime object which should be clocked by ISR */
			/* deactivates interrupts during replacing of reference, just in case timer is already running
			and someone tries to change the affected SysTime object (despite this should not be done) */
			inline static void link(system_time& instance){	macro_interrupt_critical( p_instance = &instance; ); }

			/*	start running SysTime clock and return true, if timer wasn't running before and reference ptr is non-null
			(only) returns false, if timer already running from some clock source or instance pointer is nullptr
			start is more like a save maybe-start */
			/*  as clock source an extern oscillator is chosen, please ensure to have one connected to the controller */
			static bool start();
			
			/* pause the SysTime clock
			i.e. just deactivate clock source
			you should know in general in what state your timer is, since it doesn't check before situation */
			/* pause [may be used / should be used only] sometimes after a start(), but before stop() */
			inline static void pause();
			
			/* start SysTime clock again after a pause()
			i.e. just activate clock source to external input
			you should know in general in what state your timer is, since it doesn't check before situation */
			inline static void resume();
			
			/* stop running SysTime clock (if running or also if paused) */
			/* i.e. deactivate clock source and clear the flags for CompareMatch Interrupt
			it is not possible to restart a stop()ped SysTime via resume()
			but you can call start() again, it will do this "resume after a stop()" */
			inline static void stop();
			
			/* returns true if timer1, the timer used by SysTime is running (in anyone's context) */
			inline static bool is_running(){	return (TCCR1B & 0b00000111);	}
			
			
			/******************   NON-STATIC STUFF   **********************/
			private:
			/* the real oscillator frequency / Hz, critical since ISR access */
			volatile long double osc_frequency {1};
			
			/* the truncated / broken part of the last compare match value, critical since ISR access */
			volatile long double residual_ticks {0};
			
			/* logarithmic value of precision, precision = 2^log_precision, critical since ISR access */
			/* a SysTime object is marked as invalid :<==> iff log_precision == 255 */
			/* the precision is the frequency of now-time updates
			(the compare match values will be calculated such way) */
			volatile uint8_t log_precision {0};
			
			static constexpr uint8_t INVALID_log_precision {255};
			
			/* the actual system time, which is updated by this lib / the timer */
			/* begins counting at 0 when SysTime is initialized. for encoding see time::EMT */
			/* now is accessed by ISR and user/operating system. it is critical */
			volatile time::ExtendedMetricTime now {0};

			/* calculate a precision from a given log_precision */
			inline static constexpr long double log_precision_to_precision(uint8_t log_precision){
				return static_cast<long double>(static_cast<uint32_t>(1)<<log_precision);
			}
			
			/* returns precision / Hz */
			/* the precision is the frequency of now updates */
			inline long double precision() const { uint8_t log_precision2 = log_precision;  return log_precision_to_precision(log_precision2); }
			
			public:
			/* pointer to now_update software interrupt handler,
			critical since ISR access. */
			volatile fsl::str::void_function now_update_interrupt_handler;
			// why is this public???? <<<<<#####
			
			/******* constructors *************************************************************************/
			
			/* tries to construct a SysTime object with given values
			"returns" an error code. for detail see description of try_to_set(..).
			the constructed object is only internally valid if error_code returns 0,
			but it behaves also stable if you created an intern invalid one, then it does just nothing.
			the log_precision passed might be changed in order to construct a valid SysTime
			so the final value of log_precision is passed back. */
			/* on error log_precision is INVALID_log_precision */
			system_time(const long double& osc_frequency, uint8_t& log_precision, uint8_t& error_code, fsl::str::void_function handler = nullptr);
			
			
			/******* modifiers  **************************************************************************/
			
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
			
			/* try to change osc_frequency, not interrupt critical */
			/* returns error code: see at try_to_set(...) */
			inline uint8_t set_osc_frequency(const long double& new_osc_frequency){
				return try_to_set(new_osc_frequency,log_precision);
			}
			
			/* try to change precision to wish-value, not interrupt critical */
			/* returns error code: see at try_to_set(...) */
			inline uint8_t change_log_precision(uint8_t& new_log_precision){
				long double osc_frequency_old;
				macro_interrupt_critical( osc_frequency_old = osc_frequency; );
				uint8_t result = try_to_set(osc_frequency_old,new_log_precision);
				return result;
			}
			
			/****** interrupt methods ******************************************************************/
			
			/* method which tells ISR what compare value should be used next */
			/* only to call by ISR */
			/* if object marked invalid it pause()s timer and returns 0xFFFF */
			uint16_t get_compare_match_value_only_call_by_ISR();
			
			/* method which is called by ISR every time a compare match occurs */
			/* should only be called by ISR */
			/* increases now time by a value depending on (log_)precision */
			inline void operator++();
			
			
			/****** getters for now time *************************************************************/
			
			/* read only access to raw now value. attention. access is interrupt-critical! */
			/* for safety: use operator() or get_now_time() instead */
			inline const volatile time::ExtendedMetricTime& get_reference_to_critical_now_time(){	return now;	}
			
			/* get system time (same as operator()), non-critical */
			inline time::ExtendedMetricTime get_now_time();
			
			/* get system time (same as get_now_time), non-critical */
			inline time::ExtendedMetricTime operator()() { return get_now_time(); }
			
		};



		/************************************************************************/
		/* INLINE FUNCTION IMPLEMENTATION
		*/
		/************************************************************************/
		
		/* static class member */
		inline void system_time::pause(){
			TCCR1B &= 0b11111000; // timer stopped. because of no clock source
		}
		
		inline void system_time::resume(){
			TCCR1B = 0b00001111; // start timer. set clock source to extern oscillator.
			// (also activates CTC again, which is not deactivated by pause() )
		}
		
		inline void system_time::stop(){
			TCCR1B = 0b00000000; // timer stopped. because of no clock source
			TIMSK &= 0b11000011; // deactivate interrupt flag for CompareMatch A
		}
		
		/* non-static class member */
		inline void system_time::operator ++(){
			if (log_precision == INVALID_log_precision) return; // necessary since we cannot << to a negative number .... i suppose
			this->now = const_cast<time::ExtendedMetricTime&>(this->now) + (1LL << (static_cast<uint8_t>(16 - log_precision)));
		}
		
		inline time::ExtendedMetricTime system_time::get_now_time(){
			time::ExtendedMetricTime copy;
			macro_interrupt_critical(
			copy = const_cast<time::ExtendedMetricTime&>(now);
			);
			return copy;
		}
	}
} // namespace
#endif /* F_SYSTIME_H_ */