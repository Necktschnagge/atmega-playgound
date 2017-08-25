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
#include "f_time.h"

namespace scheduler {

	class SchedulerHandle;
	class UnionCallback;
	class Specifics;
	class Flags;
	class SchedulerMemoryLine;

	extern SchedulerMemoryLine* p_table;
	extern uint8_t table_size;
	
	
	class SchedulerHandle {
	public:
		using HType = uint8_t;	
	private:
		HType handle; // we want to use 8 bit, because in 4KB Ram you wont have space for much more than 255 Scheduler lines.
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
	
	class Priority {
		uint8_t value;
	};
	
	class Progress {
		uint8_t value;	
	};
	
	class TaskSpecifics {
		Priority priority;
		Progress progress;
	};
	
	class TimerSpecifics {
		const time::ExtendedMetricTime* event_time;
	};
	
	class UnionSpecifics {
	public:
	private:
		union InternUnion {
			TaskSpecifics task;
			TimerSpecifics timer;
		};
	};
	
	class Flags {
	private:
		uint8_t container; 
			//	bit:			4				3				2				1			0
			// meaning:		is_enable :: is_interrupting :: is_callable :: is_timer :: is_valid
			
			// the flags are in such semantic that the unexpected case is '1', the normal/ most occurring case is '0';
	public:
			/* creates an flag object, which contains property is_invalid */
			/* other flags are in an valid configuration, so you can set_valid() */
		Flags() : container(0) /* especially invalid */ {}
			
		/* getter */
		
		inline bool is_valid(){ return container&1; }
		inline bool is_invalid() { return !is_valid(); }
		inline bool is_enabled(){ return container&16; }
		inline bool is_disabled(){ return !is_enabled(); }
		inline bool is_timer(){ return container&2; }
		inline bool is_task(){ return !is_timer(); }
		inline bool is_callable_ptr(){ return container&4; }
		inline bool is_function_ptr(){ return !is_callable_ptr(); }
		inline bool is_interrupting(){ return container&8; }
		inline bool is_not_interrupting(){ return !is_interrupting(); }
			
		/* setter */
		
		inline void set_valid(){ container|=1; }
		inline void set_invalid(){ container&=255-1; }
		inline void set_enabled(){ container|=16; }
		inline void set_disabled(){ container&=255-16; }
		inline void set_timer(){ container|=2; }
		inline void set_task(){ container&=255-2; set_not_interrupting(); }
		inline void set_callable_ptr(){ container|=4; }
		inline void set_function_ptr(){ container&=255-4; }
		inline void set_non_interrupting_timer(){ set_timer(); set_not_interrupting(); }
		inline void set_interrupting_timer(){ set_timer(); container|=8; }
		inline void set_not_interrupting(){ container&=255-8; }
	};
	
	class SchedulerMemoryLine {
	public:
		SchedulerHandle handle;
		UnionCallback callback;
		UnionSpecifics specifics;
		Flags flags;
	};
	
};



#endif /* F_SCHEDULER_H_ */

	
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
		## never:  execution time
			 # measured by scheduler	# only timer?	
			 		// 8
		flags:
			valid
			end-of-list???
			is_task/is_timer
			callable /ptr flag
			is_interrupting # timers only
			
	                                                                     */
	/************************************************************************/