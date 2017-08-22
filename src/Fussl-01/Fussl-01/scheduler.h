/*
 * scheduler.h
 *
 *  Created: 07.03.2017 18:16:16
 *  Author: Maximilian Starke
 *
 *
 *
 *
 *
 FPS:		everything needs to be completed of course....
			
			... make a short prototype of the scheduler to follow our issues.
			
				/// ### think about which procedures must deactivate interrupts during their execution

 */ 


#ifndef SCHEDULER_H_
#define SCHEDULER_H_
//#include <avr/interrupt.h>

#include <stdint.h>
#include <math.h>
#include "f_time.h"

/* scheduler ----------------------------------------------------------------------------------------------------  */

namespace scheduler {
	
		/* Singleton */
	class SystemTime_OLD {
		
			// 32768Hz external oscillator for timing clock
		static constexpr uint32_t DEFAULT_OSC_FREQUENCY { 1u << 15 };
		static constexpr uint8_t DEFAULT_PRECISION {10u};
		static constexpr uint8_t PRECISION_ERROR {0xFF};
		
		/* PHASE: a phase is the time gap / time area between two compare matchings */
		
		
			/* precision is the logarithmic expression how accurate the time resolution of SystemTime[.now] is
				precision says the second is divided into 2^precision equal parts
						so with precision=0 the smallest resolution is a second,
						with precision=10 the resolution will be 1/1024 s
					constraints: 0<= precision <= ld(OSC_FREQUENCY) // natural constraints
					OSC_FREQUENCY / 2^(precision)  < 2^16 // cannot be = 2^16. This is the TCNT OV.
						// We always want to make compare match for better code density and less complexity
				since precision is read by ISR but it is one byte only,
					I suppose it is not necessary to put access into critical() environment */
		uint8_t precision;
		
			/* purpose: changing precision during timer run:
				current timer gap must be calculated with old_precision,
				next must be initiated by precision
				since it is read by ISR but it is one byte only,
				I suppose it is not necessary to put access into critical() */
		uint8_t old_precision;
		
			/* the frequency of the RTC oscillator
				note: writing osc_frequency is critical since ISR reads it */
		uint32_t osc_frequency;
	
			/* amount of additional ticks to wait for during one phase,
				given by user or admin 
				it does not contain the ticks which were kicked out be the integer division
				in order to calculate the Compare-Match-Value.
				That (necessary) fix is already treated by get_time_padding() */
		long double refinement;
			// ## must be changed i.e. adapted when precision is changed!!!
		
			/* when the controller starts (booting process) 'now' should be initialized to zero
			   then it values the distance from now to the begin of computing time
			   since now is changed by an ISR you must deactivate interrupts when accessing now */
		time::ExtendedMetricTime now; 
			/* last time of setting RTC as refinement reference */
		time::ExtendedMetricTime ref_time;
		
		SystemTime_OLD();
		
		inline uint16_t get_normal_TCNT_compare_value(){ // Timer Counter
			return osc_frequency / (static_cast<uint16_t>(1) << precision);
			// damit das valid ist, müssen wir immer die gültigkeit von precision prüfen
		}

		/* return the (maybe negative) padding to add to the normal TNCTCompareValue */
		inline int16_t get_time_padding();
		
		/* looks at the instance's osc frequency and says if it allows given precision */
		bool is_precision_possible(uint8_t precision);
			
	public:
		
		/* singleton instance */
		static SystemTime_OLD instance;
	
		/* deleted constructors / operators */
		SystemTime_OLD(const SystemTime_OLD&) = delete;
		const SystemTime_OLD& operator = (const SystemTime_OLD&) = delete;
		SystemTime_OLD(SystemTime_OLD&&) = delete;
		const SystemTime_OLD& operator = (SystemTime_OLD&&) = delete;
		
		/* only to call by ISR (interrupt routine) or timer start function
			meta: even if declared public it is for lib internal purpose.
				it is inline despite defined in the cpp because of that
		   calculates the Timer Compare Value, respecting instantly (periodically) changing paddings
		   in order to realize the refinement */
		/// <<< refactor: make this a friend and put it cpp local (???)
		static inline uint16_t __timer__counter__compare__value__only__called__by__interrupt__();

		/* get the now time copied in your local variable */
		void get_now(time::ExtendedMetricTime& target) const;

			/* try to change the precision (and return true).
			   if not possible than don't change anything and return false */
		bool change_precision_aborting(uint8_t precision);
		
			/* try to change the precision to the given value,
			   if not possible the function tries to find a possible precision in the near of given precision
			   returns this->precision after change or 0xFF as an error code (PRECISION_ERROR) for "no possible precision" */
		uint8_t change_precision_anyway(uint8_t precision);
		
		// changing refinement...
		// changing ref_time
		
		/* returns a reference to the refinement. only change, if you know what you do. */
		inline long double& raw_refinement(){
			return refinement;
		}
		
		/* update, i.e. enhance the refinement by getting the
			time offset that summed up since the reference time when your RTC-time was reset (ref_time)
			offset must be the value to add scaled to refinement to correct the time error
			so case SystemTime is too fast: offset must be negative to correct
			   case SystemTime is too slow: offset must be positive to correct 
			   offset = view to real time from SystemTime */
		void update_refinement(const time::ExtendedMetricTime& offset);
		
		/* to call to when your RTC-Time was totally reset.
			set the refinement reference time to now @ function call */
		void reset_ref_time();
		
		/* only use if you know what you're doing
			returns reference to the reference time for refinement enhancing*/
		inline time::ExtendedMetricTime& raw_ref_time(){
			return ref_time;
		}

			/* increase the instance's now time by the time delta given indirect by precision */
			/* more exactly: given by old_precision and set the old_precision to precision */
		const SystemTime_OLD& operator ++ ();
				
			/* stops the timer / SystemTime if it is running (anyway)
			   tries to set frequency, precision and refinement */
		static bool init(const long double& refinement, uint32_t osc_frequency, uint8_t precision);
			
			/* start running SystemTime clock
				true: precision is valid and SystemTime started
				false: precision invalid or SystemTime already running */
		static bool start();
		
			/* stop running SystemTime clock */
		static void stop();
		
	};
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	typedef uint16_t SCHEDULE_HANDLE; /// no make a wrapper, a class with one uint16_t
	
	constexpr SCHEDULE_HANDLE NO_HANDLE {0};
	
	//constexpr uint16_t PARTS_OF_SECOND_LOG {5}; // 32 parts of second
	
	//constexpr uint16_t PARTS_OF_SECOND {1 << PARTS_OF_SECOND_LOG};
	
	class Priority{
		uint8_t state;
		
		void setState(uint8_t priority, uint8_t percentage){
			
		}
		
	};
	
	extern uint16_t divisions_of_second; // 0 ... PARTS_OF_SECOND - 1
//	extern time::HumanTime now;
	extern uint16_t nextFreeHandle;
	extern void* taskTable;
	extern uint16_t taskTableSize;
	
	void init(void* taskTableSpaceJunk, uint16_t taskTableSize);
	
	//SCHEDULE_HANDLE addTimer(void (*function)(), const Time& interval, uint16_t repeat /*, Priority priority*/){return 0;};
	
	//bool cancelTimer(SCHEDULE_HANDLE handler){return false;};
	
		// should return: whether time was increased, exact time after last full second
	uint16_t updateNowTime();
	
		/* central control loop */
	void run();
	
	class SchedulingRecord{
		SCHEDULE_HANDLE handle;
		// put the callback union from gui to another header
		uint8_t flags; //{ valid, Timer/Task, Callable/procedure, }
		
	};
	
	class UCallback {
		uint16_t dummy;
		};
	
	class ExecutionTime {
		float dummy;
		};
	
	class BackgroundTaskMemLine {
		SCHEDULE_HANDLE handle;
		UCallback callback;
		uint8_t percentage;
		uint8_t priority;
		//ExecutionTime exeTime;
		uint8_t flags;
		};
		
	class TimerMemLine {
		SCHEDULE_HANDLE handle;
		UCallback callback;
		
		uint8_t priority;
		//ExecutionTime exeTime;
		uint8_t flags;
		};
	class TimerMemLineTime {
		//ExactTime time;
		};
	class TimerMemLineTime2 {
		//ExactTime time; // but we can leave out the year.
		uint16_t repeatings;
	};
	
	
	
	/************************************************************************/
	/* 
	ideas:
	
	list of timers:
		2				2								2											2					2				1								1
		handle,		task procedure or callable,		time when to execute*, +part of second ,		intervall*,		repeatings,		priority,		exe time,		some flags (valid etc, end of list-flag, is timer/task falg, isBackground- flag, callable/ptr flag)
													// put this directly after the first "line" in memory. then we do not need pointer.
	list of background tasks:
		2					2																		1				1								1
		handle,		callback,																	percentage,		priority,		exe time,		some flags
	
	                                                                     */
	/************************************************************************/
	
}

#ifdef hack
uint16_t test(){
	uint16_t x[2];
	return x[sizeof(HumanTime)];
}
#endif

#endif /* SCHEDULER_H_ */