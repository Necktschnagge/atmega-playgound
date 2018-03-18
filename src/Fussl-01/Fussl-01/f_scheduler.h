/*
 * f_scheduler.h
 *
 * Created: 24.08.2017 18:13:37
 *  Author: Maximilian Starke
 */ 


#ifndef F_SCHEDULER_H_
#define F_SCHEDULER_H_

#include <stdint.h>

#include "f_macros.h"
#include "f_concepts.h"
#include "f_time.h"

namespace scheduler {

	/* forward declaration: intern classes / structs: */
	class SchedulerHandle;
	class UnionCallback;
			class Priority; // as part of TaskSpecifics
			class Progress; // as part of TaskSpecifics
		class TaskSpecifics; // as part of UnionSpecifics
		class TimerSpecifics; // as part of UnionSpecifics
	class UnionSpecifics;
	class Flags;
	class SchedulerMemoryLine;

	/* interface to u-programmer who must provide RAM space */
	extern SchedulerMemoryLine* p_table;
	extern uint8_t s_table; // size of table as the number of table lines.
	
	extern uint32_t watchdog; // contains indirectly a time distance,
		// which stands for the time the software watchdog has to wait before rebooting
						// should not be visible in header
	
	extern uint8_t is_active; // 0 not active
								// 1 just active
								// 2 still active, but stops (and run() returns) next.
								// means the task runner (i.e. scheduler is running, i.e. executes timers as well as tasks)
	extern uint32_t countdown; // the countdown is the intern var of software watchdog.
				// it is set to watchdog on finishing of any task or timer procedure.
				// it is decreased by 1 on every SysTime interrupt.
	
	
	class SchedulerHandle {
	public:
		using HType = uint8_t;
	private:
		HType handle; // we want to use 8 bit, because in 4KB Ram you wont have space for much more than 255 Scheduler lines.
	public:
			/* construct a NO_HANDLE */
		inline constexpr SchedulerHandle() : handle(0) {}
			/* construct handle with number ID */
		inline constexpr explicit SchedulerHandle(const HType& id) : handle(id) {}
		
		
		inline SchedulerHandle& operator = (const SchedulerHandle& r){ handle = r.handle; return *this; }
		// inline SchedulerHandle& operator = (const HType& id){ handle = id; return *this; } // it fools the explicit c-tor kinda way
		inline explicit operator HType() { return handle; }
		
		inline bool operator == (const SchedulerHandle& r){ return this->handle == r.handle; }
	};

	constexpr SchedulerHandle NO_HANDLE{0}; // conc. there are 255 mem lines at max. so 0 can be the no_handle without problems
											// a handle which may be used, if there should be no valid entry. // but first therefore we have a valid flag inside flag class
		
	constexpr time::ExtendedMetricTime NO_WATCHDOG_EMT{0};
		// a time for watchdog is rbequired on initialisation. but if you don't want a watchdog,
		// just set it to 0 (NO_WATCHDIG).
	constexpr int32_t NO_WATCHDOG_COUNTER_FORMAT{0}; // calc by a constexpr function for time-wtchdog -> counter-watchdog
	
	class UnionCallback {
	public:
		using FunctionPtr = void(*)(); // maybe this type def can also be put in f_concepts
	private:
		union InternUnion {
				FunctionPtr function_ptr;
				concepts::Callable* callable_ptr;
			};
		InternUnion _union;
	public:
		inline void set_function_ptr(FunctionPtr ptr){ _union.function_ptr = ptr; }
		inline void set_callable_ptr(concepts::Callable* ptr) { _union.callable_ptr = ptr; }
		inline void call_function(){ if (_union.function_ptr != nullptr) _union.function_ptr(); }
		inline void call_callable(){ if (_union.callable_ptr != nullptr) (*_union.callable_ptr)(); }
		
	};
	
	class Priority {
	public:
			/* 1..200 : least...most important */
			/* 255 means running exclusive */ 
		uint8_t value;
		
		Priority(){}
	};
	
	class Progress {
	public:
		uint8_t value; // 0 ..255 : 0%ready .. 100%ready
		
		Progress(){}
	};
		
	class TaskSpecifics {
	public:
		Priority priority;
		Progress progress;
		
		uint8_t get_importance(){
			uint16_t wPriority {priority.value};
			uint16_t wProgress {progress.value};
			return  wPriority * 255 / 200;
		}
	};
	
	class TimerSpecifics {
	private:
		const time::ExtendedMetricTime* event_time; // earliest time when timer can be executed
	public:
		inline const time::ExtendedMetricTime& get_execution_time() const { return *event_time; }
	};
	
	class UnionSpecifics {
	public:
	private:
		union InternUnion {
			TaskSpecifics task;
			TimerSpecifics timer;
		};
		
		InternUnion _union;
	public:
		TaskSpecifics& task() { return _union.task; } // using direct references makes objects become larger by 2*ptrsize, I tried it
		TimerSpecifics& timer() { return _union.timer; }
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
		inline void set_valid_flag(bool is_valid){ container = (container&(255-1)) + 1 * is_valid; }
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
	
	class GroupCellSize {
	public:
		uint8_t cell_items;
	};
	
	
	class SchedulerMemoryLine {
	public:
		SchedulerHandle handle; // 1
		UnionCallback callback; // 2
		UnionSpecifics specifics; // 2
		Flags flags; // 1
		GroupCellSize group_cell_size; // nur für tasks ...könnte man also woanders hinspeichern
				// Maybe we could reserve some space of the scheduler table for an Byte array for these cell sizes.
	};
	
	/* aufbau der tabelle.
	
	interrupt timer // look on interrupt
	non interrupt timer // look first on schedule loop
	tasks // look second in schedule loop
	
	
	*/
		// intern memory, watchdog time
	bool init(SchedulerMemoryLine* table_ptr, uint8_t table_size,const time::ExtendedMetricTime& watchdog_restart_time = NO_WATCHDOG_EMT){
		if (is_active) return false; // can't init because already running.
		scheduler::p_table = table_ptr;
		scheduler::s_table = table_size;
		watchdog = 0; // watchdog_restart_time; / (zeit pro SysTime tick)
		
		return true;
	}
	
	void set_watchdog(const time::ExtendedMetricTime& watchdog){ scheduler::watchdog = 0; /* watchdog; (zeit pro systime tick ) */ }
	
	void interrupt_handler(){
		if (!is_active) return;
		
		--countdown;
		if ((!watchdog) || (countdown > 0)) {
			// reset hardware watchdog here
		}	// else: after short time hardware watchdog will restart controller
		
		
		
		// called by SysTime, every time an TC Compare Match Interrupt occurs
		// check for interrupting functions in schedule table
	}
	
		// central control loop. called after init and provides the tasks
	bool run();
	
		// make scheduler return after current executed task / timer
	bool stop(){
		if (!is_active) return false;
		is_active = 2;
		return true;
	}
	
		/* true: task was written into scheduler table. */
		/* false: table is full. not written into table */
			// maybe it is because of some disabled timers or tasks, but this has to be checked by u-programmer
	bool new_task(){
		return false;
	}
	
		/* write handles of disabled timers / tasks into array handles which has count cells
		   return count as the number of handles which are disabled. */
		/* if more than array size, a undefined choice of them is stored into array.
			if less than only the first count@after entries are filled with handles */
		/* return also first found handle */
	uint8_t get_disabled_entries(uint8_t* handles, uint8_t& count){
		return 0;
	}
	
}



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