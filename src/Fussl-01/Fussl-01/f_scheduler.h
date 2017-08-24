/*
 * f_scheduler.h
 *
 * Created: 24.08.2017 18:13:37
 *  Author: Maximilian Starke
 */ 


#ifndef F_SCHEDULER_H_
#define F_SCHEDULER_H_

#include <stdint.h>

#include "f_concepts.h"

namespace scheduler {

	class SchedulerHandle;
	class UnionCallback;
	class SchedulerMemoryLine;
	//using PSchedulerMemoryLine = SchedulerMemoryLine*;

	extern SchedulerMemoryLine* p_table;
	extern uint8_t table_size;
	
	
	class SchedulerHandle {
	public:
		using HType = uint8_t;	
	private:
		HType handle; // we want to use 8 bit, because 4KB Ram you wont have space for much more than 255 Scheduler lines.
	public:
		inline SchedulerHandle() : handle(0) {}
		inline explicit SchedulerHandle(const HType& id) : handle(id) {}
		
		inline SchedulerHandle& operator = (const SchedulerHandle& r){ handle = r.handle; return *this; }
		inline SchedulerHandle& operator = (const HType& id){ handle = id; return *this; }
		inline explicit operator HType() { return handle; }
		
		inline bool operator == (const SchedulerHandle& r){ return this->handle == r.handle; }
	};
	
	class UnionCallback {
	public:
		using FunctionPtr = void(*)(); // mybe this type def can also be put in f_concepts
	private:
		union InternUnion {
				FunctionPtr function_ptr;
				concepts::Callable* callable_ptr;
			};
		InternUnion _union;
	public:
		inline void set_function_ptr(FunctionPtr ptr){ _union.function_ptr = ptr; }
		inline void set_callable_ptr(concepts::Callable* ptr) { _union.callable_ptr = ptr; }
		inline void call_function(){ _union.function_ptr(); }
		inline void call_callable(){ (*_union.callable_ptr)(); }
		
	};
	
	class Flags {
	private:
		uint8_t container; 
			//	bit:			3					2			1			0
			// meaning:		is_interrupting :: is_callable :: is_timer :: is_valid
			
			// the flags are in such semantic that the unexpected case is '1', the normal/ most occurring case is '0';
	public:
			/* creates an flag object, which contains property is_invalid */
		Flags() : container(0) /* especially invalid */ {}
			
		/* getter */
		
		inline bool is_valid(){ return container&1; }
		inline bool is_invalid() { return !is_valid(); }
		inline bool is_timer(){ return container&2; }
		inline bool is_task(){ return !is_timer(); }
		inline bool is_callable_ptr(){ return container&4; }
		inline bool is_function_ptr(){ return !is_callable_ptr(); }
		inline bool is_interrupting(){ return container&8; }
		inline bool is_not_interrupting(){ return !is_interrupting(); }
			
		/* setter */
		
		inline void set_valid(){ container|=1; }
		inline void set_invalid(){ container&=255-1; }
		inline void set_timer(){ container|=2; }
		inline void set_task(){ container&=255-2; }
	};
	
	class SchedulerMemoryLine {
		SchedulerHandle handle;
		UnionCallback callback;
		
		Flags flags;
	};
	
};



#endif /* F_SCHEDULER_H_ */

#if false
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
	
	
	list of everything.													both		task		timer
		handle	# both													// 1-2
		callback function ptr or callable # both						// 2(?)
		earliest time of execution # timers only												8	
			intervall min time between two executions			// for this we can write a separate object, a multi timer, which stores the interval and repeatings and alway updates with the scheduler by cooperating
			repeatings ... times to repeat a timer callback
		percentage ready # tasks only												1-
		priority # tasks only ??? (or both?)										1-
		execution time # measured by scheduler	# only timer?			// 8
		flags:
			valid
			end-of-list???
			is_task/is_timer
			callable /ptr flag
			is_interrupting # timers only
			
	                                                                     */
	/************************************************************************/
	
#endif