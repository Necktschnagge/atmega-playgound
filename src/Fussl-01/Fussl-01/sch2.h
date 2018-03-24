/* 
* sch2.h
*
* Created: 20.03.2018 13:50:41
* Author: F-NET-ADMIN
*/


#ifndef __SCH2_H__
#define __SCH2_H__

#include <stdint.h>

#include "f_range_int.h"
#include "f_time.h"
#include "f_concepts.h"

template <uint8_t TABLE_SIZE>
class scheduler2 {
public:	
	using SchedulerHandle = range_int<uint8_t,TABLE_SIZE,true,true>;
	
	static constexpr SchedulerHandle NO_HANDLE{ SchedulerHandle::OUT_OF_RANGE };
private:
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
	
	class TaskSpecifics {
		public:
		uint8_t urgency; // Dringlichkeit
		/* 255 .. least important ... 0 ... most important */
		uint8_t task_race_countdown;		
		
	};

	class TimerSpecifics {
		public:
		const time::ExtendedMetricTime* event_time; // earliest time when timer can be executed

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
	
public:
	
		
		// a handle which may be used, if there should be no valid entry. // but first therefore we have a valid flag inside flag class

/*
			class scheduling_countdown;
			class Priority; // as part of TaskSpecifics
			class Progress; // as part of TaskSpecifics
		class TaskSpecifics; // as part of UnionSpecifics 
		class TimerSpecifics; // as part of UnionSpecifics
	class UnionSpecifics;
	class Flags;
	class SchedulerMemoryLine;
*/
private:
	
	static_assert(sizeof(SchedulerHandle) == 1, "SchedulerHandle has not the appropriate size.");
	static_assert(sizeof(UnionCallback) == 2, "UnionCallback has not the appropriate size.");
	static_assert(sizeof(UnionSpecifics) == 2, "UnionSpecifics has not the appropriate size.");
	static_assert(sizeof(concepts::Flags) == 1, "concepts::Flags has not the appropriate size.");
	
	
	struct SchedulerMemoryLine {
		SchedulerHandle handle; // 1
		UnionCallback callback; // 2
		UnionSpecifics specifics; // 2
		concepts::Flags flags; // 1
	};
	
	
	
	/* private data */
	
	/* aufbau der tabelle.
	
	interrupt timer // look on interrupt
	non interrupt timer // look first on schedule loop
	tasks // look second in schedule loop
	
	
	*/
	SchedulerMemoryLine table[TABLE_SIZE];
	
public:
	static constexpr size_t table_line_size(){ return sizeof(SchedulerMemoryLine); }

	concepts::Flags flags;
	static constexpr concepts::Flags::flag_id IS_ACTIVE{ 0 }; // true iff run() runs
	static constexpr concepts::Flags::flag_id STOP_CALLED{ 1 }; // if true, run() should return when current task returns.
	
	uint32_t watchdog; // contains indirectly a time distance,
	// which stands for the time the software watchdog has to wait before rebooting
						// should not be visible in header
	
	uint32_t countdown; // the countdown is the intern var of software watchdog.
				// it is set to watchdog on finishing of any task or timer procedure.
				// it is decreased by 1 on every SysTime interrupt.
	
		
	static constexpr time::ExtendedMetricTime NO_WATCHDOG_EMT{0};
		// a time for watchdog is rbequired on initialisation. but if you don't want a watchdog,
		// just set it to 0 (NO_WATCHDIG).
	static constexpr int32_t NO_WATCHDOG_COUNTER_FORMAT{0}; // calc by a constexpr function for time-wtchdog -> counter-watchdog
	
			
	
	
	
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
	
	
		// intern memory, watchdog time
	bool init(SchedulerMemoryLine* table_ptr, uint8_t table_size,const time::ExtendedMetricTime& watchdog_restart_time = NO_WATCHDOG_EMT){
		if (is_active) return false; // can't init because already running.
		//scheduler::p_table = table_ptr;
		//scheduler::s_table = table_size;
		watchdog = 0; // watchdog_restart_time; / (zeit pro SysTime tick)
		
		return true;
	}
	
	void set_watchdog(const time::ExtendedMetricTime& watchdog){ /*scheduler::watchdog = 0; /* watchdog; (zeit pro systime tick ) */ }
	
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
	
};

#endif //__SCH2_H__
