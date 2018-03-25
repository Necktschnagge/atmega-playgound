/*
* sch2.h
*
* Created: 20.03.2018 13:50:41
* Author: F-NET-ADMIN
*/


#ifndef __SCH2_H__
#define __SCH2_H__

#include <stdint.h>
#include <avr/wdt.h>

#include "f_range_int.h"
#include "f_concepts.h"
#include "f_time.h"
#include "f_systime.h"

template <uint8_t TABLE_SIZE>
class scheduler2 {
	public:
	using SchedulerHandle = range_int<uint8_t,TABLE_SIZE,true,true>;
	
	static constexpr SchedulerHandle NO_HANDLE{ SchedulerHandle::OUT_OF_RANGE };
	
	static constexpr uint8_t STANDARD_URGENCY{ 50 };
	
	private:
	class UnionCallback {
		private:
		union InternUnion {
			concepts::void_function function_ptr;
			concepts::Callable* callable_ptr;
		};
		InternUnion _union;
		public:
		inline void set_function_ptr(concepts::void_function ptr){ _union.function_ptr = ptr; }
		inline void set_callable_ptr(concepts::Callable* ptr) { _union.callable_ptr = ptr; }
		inline void call_function() const { if (_union.function_ptr != nullptr) _union.function_ptr(); }
		inline void call_callable() const { if (_union.callable_ptr != nullptr) (*_union.callable_ptr)(); }
		
	};
	
	struct TaskSpecifics {
		uint8_t urgency; // Dringlichkeit
		/* 255 .. least important ... 0 ... most important */
		uint8_t task_race_countdown;
	};

	struct TimerSpecifics {
		const time::ExtendedMetricTime* event_time; // earliest time when timer can be executed
	};
	
	class UnionSpecifics {
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
	
	static constexpr concepts::Flags::flag_id IS_VALID{ 0 };
	static constexpr concepts::Flags::flag_id IS_ENABLED{ 1 };
	static constexpr concepts::Flags::flag_id IS_TIMER{ 2 };
	static constexpr concepts::Flags::flag_id IS_INTTIMER{ 3 };
	static constexpr concepts::Flags::flag_id IS_CALLABLE{ 4 };

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
	
	concepts::Flags flags;
	static constexpr concepts::Flags::flag_id IS_RUNNING{ 0 }; // true iff run() runs
	static constexpr concepts::Flags::flag_id STOP_CALLED{ 1 }; // if true, run() should return when current task returns.
	static constexpr concepts::Flags::flag_id TABLE_LOCKED{ 2 }; // if scheduler is accessing scheduling table, it must not be modified by interrupt timers
	static constexpr concepts::Flags::flag_id EMPTY_TABLE_DETECTED{ 3 };

	
	/* private data */
	
	/* aufbau der tabelle.
	
	interrupt timer // look on interrupt
	non interrupt timer // look first on schedule loop
	tasks // look second in schedule loop
	
	*/
	/*
	begin
	interrupting timers
	[no gap]
	noninterrupting timers
	[maybe gap]
	tasks
	[no gap]
	end
	*/
	
	/*
	an entry is gap, when it is INVALID.
	an entry that is VALID but DISABLED, is not gap and has to be inside its section.
	
	*/
	SchedulerMemoryLine table[TABLE_SIZE];

	uint8_t hardware_watchdog_timeout;

	uint32_t watchdog_countdown_value;
	// it is set to watchdog_reset_value on finishing of any task or timer procedure.
	// it is decreased by 1 on every SysTime interrupt.

	uint32_t watchdog_reset_value; // watchdog_countdown_value is reset to this value every time a timer or task procedure returns
	// contains indirectly a time distance, which stands for the time the software watchdog has to wait before rebooting
	
	scheduler::SysTime& system_time;
	
	public:
	static constexpr uint8_t table_line_size(){ return sizeof(SchedulerMemoryLine); }


	
	static constexpr time::ExtendedMetricTime NO_WATCHDOG_EMT{0};
	// a time for watchdog is required on initialization. but if you don't want a watchdog,
	// just set it to 0 (NO_WATCHDOG).
	static constexpr int32_t NO_WATCHDOG_COUNTER_FORMAT{0}; // calc by a constexpr function for time-wtchdog -> counter-watchdog

	
	// intern memory, watchdog time
	bool init(SchedulerMemoryLine* table_ptr, uint8_t table_size,const time::ExtendedMetricTime& watchdog_restart_time = NO_WATCHDOG_EMT){
		// if (is_active) return false; // can't init because already running.
		//scheduler::p_table = table_ptr;
		//scheduler::s_table = table_size;
		watchdog_reset_value = 0; // watchdog_restart_time; / (zeit pro SysTime tick)
		
		return true;
	}
	
	void set_watchdog(const time::ExtendedMetricTime& watchdog){ /*scheduler::watchdog = 0; watchdog; (zeit pro systime tick ) */ }
	/*
	void interrupt_handler(){
	//if (!is_active) return;
	
	--watchdog_countdown_value;
	if ((!watchdog_reset_value) || (watchdog_countdown_value > 0)) {
	// reset hardware watchdog here
	}	// else: after short time hardware watchdog will restart controller
	
	
	
	// called by SysTime, every time an TC Compare Match Interrupt occurs
	// check for interrupting functions in schedule table
	}
	*/
	// central control loop. called after init and provides the tasks
	bool run();
	//
	
	//
	/*
	// make scheduler return after current executed task / timer
	void stop(){
	// if (flags.get(IS_ACTIVE) == false) return false;
	flags.set_true(STOP_CALLED);
	// return true;
	}
	*/
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

/************************************************************************/
/* FUNCTION IMPLEMENTATION                                              */
/************************************************************************/


template <uint8_t TABLE_SIZE>
bool scheduler2<TABLE_SIZE>::run()
{
	flags.set_false(STOP_CALLED); // delete previous stop command
	flags.set_true(IS_RUNNING); // set running flag
	flags.set_false(EMPTY_TABLE_DETECTED); // check for empty table
	wdt_enable(hardware_watchdog_timeout); // start hardware watchdog
	
	central_control_loop:
	while(! flags.get(STOP_CALLED)){ // run central control loop until someone called stop() from inside (or table became empty -> break from inside).
		
		wdt_reset(); // reset hardware watchdog
		uint8_t choice; // index inside
		flags.set_true(TABLE_LOCKED);
		
		/*** look for a timer that can be executed ***/
		{
			time::ExtendedMetricTime now = system_time();
			for(choice = 0; choice < TABLE_SIZE; ++choice){
				if (	(table[choice].flags.get(IS_VALID) == false)	||	(table[choice].flags.get(IS_TIMER) == false)){
					goto no_timer_expired; // we reached the end of the timer section, but no timer expired
				}
				// at current index we see a valid timer
				if (table[choice].flags.get(IS_ENABLED)){
					// timer is enabled.
					flags.set_false(EMPTY_TABLE_DETECTED);
					if (now > *table[choice].specifics.timer().event_time) {
						// timer has expired.
						table[choice].flags.set_false(IS_ENABLED); // disable timer to avoid execution twice
						goto execute;
					}
				}
				// side affect of timer order in table: interrupting timers are always executed first
			}
		} /*** end of looking for timer ***/
		
		// when here: table is full with timers, but no one expired
		
		no_timer_expired:
		
		// when here: table may have timers inside, but no one expired
		
		if (flags.get(EMPTY_TABLE_DETECTED)){ // table is empty we will always have nothing to execute
			goto exit_run_because_of_empty_table;
		}
		
		/*** look for a task that can be executed ***/
		{
			uint8_t min_entry_index{ TABLE_SIZE }; // means no entry
			uint16_t min_entry_race_countdown{ 0xFFFF }; // means no entry
			bool section_empty{ true }; // stays true if and only if there is no enabled task in the task section
			
			/** look for a task with task_race_countdown == 0 **/
			for (choice = TABLE_SIZE - 1; choice < TABLE_SIZE; --choice){
				if ((table[choice].flags.get(IS_VALID) == false) || (table[choice].flags.get(IS_TIMER) == true)){
					// reached end of task section in table
					goto no_zero_countdown; /* same as break; */
				}
				if (table[choice].flags.get(IS_ENABLED)){
					// current task has to be considered
					section_empty = false; // there is at least one task that may eventually be executed
					if (table[choice].specifics.task().task_race_countdown == 0){
						goto task_execute;
					}
					if (static_cast<uint16_t>(table[choice].specifics.task().task_race_countdown) < min_entry_race_countdown){
						min_entry_index = choice;
						min_entry_race_countdown = table[choice].specifics.task().task_race_countdown;
					}
				}
			} /** end of looking for task with countdown == 0 **/
			
			// when here: the whole table is filled with tasks, but no one's task_race_countdown was at zero
			
			no_zero_countdown:
			
			// when here: reached end of (maybe empty, maybe max-sized) task section, but no task_race_countdown was zero
			
			if (section_empty){
				 flags.set_true(EMPTY_TABLE_DETECTED);
				 goto central_control_loop; // the task section is empty at the moment, run scheduler from begin.
			}
			
			// reached end of non-empty task section, but no task_race_countdown was zero
			
			/** subtract minimum task_race_countdown from all count_downs in task section and let the min-countdown task be executed afterwards **/
			for (choice = TABLE_SIZE - 1; (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false) && (choice < TABLE_SIZE); --choice){			
			/*	if ((table[choice].flags.get(IS_VALID) == false) || (table[choice].flags.get(IS_TIMER) == true) || (choice >= TABLE_SIZE)){
					// reached end of task section in table
					// execute the min entry that we found one loop before:
					goto task_execute;
				}
			*/ // << replaced by new loop condition
				if (table[choice].flags.get(IS_ENABLED)){
					table[choice].specifics.task().task_race_countdown -= static_cast<uint8_t>(min_entry_race_countdown);
				}
			}
			choice = min_entry_index;
			table[choice].specifics.task().task_race_countdown = table[choice].specifics.task().urgency;
			// goto execute; // implicitly done
		} /*** end of looking for task to be executed ***/
		
		execute:
		{
			const bool is_callable = table[choice].flags.get(IS_CALLABLE);
			const UnionCallback& callback = table[choice].callback;
			flags.set_false(TABLE_LOCKED);
			
			if (is_callable){
				callback.call_callable();
				} else {
				callback.call_function();
			}
		}
	}
	uint8_t exit_code;
	exit_run_because_of_stop_call: /* by leaving central control loop by violating loop invariant */
	{
		// when here: central control loop ended because stop was called
		exit_code = 1;
		goto exit_run;	
	}
	exit_run_because_of_empty_table:
	{
		// when here via goto: central control loop exited because table is empty
		exit_code = 0;
		// goto exit_run; implicitly done
	}
	exit_run:
	{
		wdt_disable(); // stop watchdog
		flags.set_false(IS_RUNNING); // delete running flag
		flags.set_false(TABLE_LOCKED); // table unlocked
	}
	/*
	task selection {
	TIDY_UP_STRATEGY
	go through all task until you find one with 0 racing, replace 0 racing by reset value and choose this task goto execute.
	remember always the index with lowest racing value.
	if no value == 0 found go through all entries again and subtract the min racing value that was detected.
	take the one entry with the minimum as choice, reset it to its reset value and execute it.
	=> in most cases we wont find 0, => we go twice through the list, the maybe-only 0 will be shifted away.
	=> perhaps at next iteration we wont find a 0 again.
	
	better idea:
	REBASE_WHEN_OVERFLOW_STRATEGY
	alway search for the minimum, global minimum, if found zero we can break earlier.
	execute this minimum and try to add its distance to the racing value.
	if rancing value is overflowing that we need to go through all entries and make them all smaller.
	
	
	third idea:
	MOVING_ZERO_STRATEGY
	instead of "re baseing" we could also use an extra variable as last minimum, new minimum will be lowest number >= last min.
	
	// just implement one strategy and later implement the others and make it possible to choose by conditioned compiling.
	}
	
	*/
	return exit_code;
	// reasons of breaking scheduling:
	/*
		0: table got empty
		1: stop was called
		
		3.???? not all requirements for starting were fulfilled
	*/
}



#endif //__SCH2_H__
