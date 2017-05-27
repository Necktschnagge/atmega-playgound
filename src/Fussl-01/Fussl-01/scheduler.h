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
 */ 


#ifndef SCHEDULER_H_
#define SCHEDULER_H_
//#include <avr/interrupt.h>

#include <stdint.h> // by the way... it is already included by f_time.h
#include <math.h>
#include "f_time.h"

/* scheduler ----------------------------------------------------------------------------------------------------  */

namespace scheduler {
	
		/* Singleton */
	class SystemTime {
		
			// 32768Hz external oscillator for timing clock
		static constexpr uint32_t DEFAULT_OSC_FREQUENCY { 1u << 15 };
		static constexpr uint8_t DEFAULT_PRECISION {10u};
		static constexpr uint8_t PRECISION_ERROR {0xFF};
		
			/* precision is the logarithmic expression how accurate the time resolution of SystemTime is
				precision says the second is divided into 2^precision equal parts
				so with precision=0 the smallest resolution is a second,
				with precision=10 the resolution will be 1/1024 s
				constraints: 0<= precision <= ld(OSC_FREQUENCY) // natural constraints
				OSC_FREQUENCY / 2^(precision)  < 2^16 // cannot be = 2^16. This is the TCNT OV.
					// We always want to make compare match for better code density and less complexity
				*/
		uint8_t precision;
			/* purpose: changing precision during timer run:
				current timer gap must be calculated with old_precision,
				next must be initiated by precision*/
		uint8_t old_precision;
		
			/* PHASE: a phase is the time gap / time area between two compare matchings */
		
			/* the frequency of the RTC oscillator */
		uint32_t osc_frequency;
	
			/* amount of additional ticks to wait for during one phase,
				given by user or admin 
				it does not contain the ticks which were kicked out be the integer division
				in order to calculate the Compare-Match-Value.
				That (necessary) fix is already treated by get_time_padding() */
		long double refinement;
			// ä## must be changed i.e. adapted when precision is changed!!!
		
			/* when the controller starts (booting process) 'now' should be initialized to zero
			   then it values the distance from now to the begin of computing time */
		time::ExtendedMetricTime now;
			/* last time of setting RTC as refinement reference */
		time::ExtendedMetricTime ref_time;
		
		SystemTime();
					 //## precision gültig machen
		
			/* return the (maybe negative) padding to add to the normal TNCTCompareValue
				*/
		inline int16_t get_time_padding(){
			static long double padding {0};
			padding += refinement
					+ static_cast<long double>(osc_frequency % (1<<precision))
							/ static_cast<long double>(1<<precision);
			// extract the int part of padding
			long double trunced = trunc(padding);
			padding -= trunced;
			return static_cast<int16_t>(trunced); // ### think about test cases how are negative numbers trunced?
		}
		
		inline uint16_t get_normal_TCNT_compare_value(){ // Timer Counter 
			return osc_frequency / (1 << precision); // damit das valid ist, müssen wir immer die gültigkeit von precision prüfen
		}
		
		bool is_precision_possible(uint8_t precision);
			
	public:
	
		SystemTime(const SystemTime&) = delete;
		const SystemTime& operator = (const SystemTime&) = delete;
		SystemTime(SystemTime&&) = delete;
		const SystemTime& operator = (SystemTime&&) = delete;
		
		static inline uint16_t __timer__counter__compare__value__only__called__by__interrupt__() {
			//### check overflows / underflows
			int32_t value;
			
		again:
			value = static_cast<int32_t>(instance.get_normal_TCNT_compare_value()) + static_cast<int32_t>(instance.get_time_padding());
			
		begain:
			if (value == 0){
				// well, I think this is okay. It happens when we use maximum precision and or osc is a slower than it should be.
				// but this case should really not appear far more than one time again during one function call.
				++instance;
				goto again;
			}
			if (value < 0){
				// ### log any kind of error. this case should never happen!!!!
				// we should stop immediately.
				value = 0;
				goto begain;
				// osc frequency is far from its value
			}
			if (value > 0xFFFE){ // FFFF must also be possible
				// osc_frequency far from its value
				// ## log error
				value = 0xFFFF;
			}
			
			return static_cast<uint16_t>(value);
		}

		static SystemTime instance;
		
		
		// such a changing process must wait till the current second is over or not ???###
		/// -> no it neednt but the last phase must be treated with the old precision
		/// ### think about which procedures must deactivate interrupts during their execution	

			/* try to change the precision (and return true).
			   if not possible than don't change anything and return false */
		bool change_precision_aborting(uint8_t precision);
		
			/* try to change the precision to the given value,
			   if not possible the function tries to find a possible precision in the near of given precision
			   returns this->precision after change or 0xFF as an error code for "no possible precision" */
		uint8_t change_precision_anyway(uint8_t precision);

			/* increase the instance's now time by the time delta given indirect by precision */
			/* more exactly: given by old_precision and ser the old_precision to precision */
		const SystemTime& operator ++ ();
		
			/* static version of operator ++ on singleton instance */
		inline static void tick_forward(){
			instance.operator ++();
		}
		
			/* init the SystemTime singleton */
		static bool init(uint8_t precision = DEFAULT_PRECISION, uint32_t osc_frequency = DEFAULT_OSC_FREQUENCY, const long double& clock_precision = 0.0L);
			//##not impl
		
			/* start running SystemTime clock */
		static bool start();
			// ## not impl both
			
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