/*
* sch2.h
*
* Created: 20.03.2018 13:50:41
* Author: Maximilian Starke
*/
//FPS: build fps, make comments better

/* lib description: you must not activate interrupts during the execution of some interrupt handler. this will end up in undefined behavior since somemethods, e.g. execute_interrupting_timers() assume they're always running uninterrupted. */

/*
	Watchdogs:
	
	There are two so-called "watchdogs" in the context of the scheduler, the hardware watchdog and the software watchdog.
	With the hardware watchdog you check against exceeding atomic sections.
	With the software watchdog you check against callback routines
	that are scheduled but do not return in the appropriate amount of time (even if they do not use exceeding atomic sections).

	hardware watchdog:
	The hardware watchdog / built-in watchdog of your MCU will be used by the scheduler.
	You just have to tell the scheduler the hardware watchdog timeout on construction. (Therefor see the description of the constructor)
	How it works internally:
	The scheduler resets the built-in watchdog (implemented in hardware) on every now time update (interrupt)
	so that it cannot restart the controller.
	Only in case someone (the scheduler itself or the programmer using the scheduler) uses too long atomic sections
	(where interrupts are turned off) or too long interrupt handlers (..same..) the hardware watchdog will not be reseted
	so it will reset the controller.
	The hardware watchdog will be startet automatically when you call my_scheduler.run() and stopped if run() returns.
	
	software watchdog:
	With the software watchdog you check against callback routines
	that are scheduled but do not return in the appropriate amount of time (even if they do not use exceeding atomic sections).
	The software watchdog is a watchdog fully implemented in software by the class scheduler.
	You may activate or deactivate the software watchdog whenever you want.
	
	Having the software watchdog deactivated the following (probably undesired) scenario may occur:
	There is one task that does never use atomic sections but also does not eventually return.
	Once this task is scheduled by the scheduler, all interrupting timers will eventually be executed
	but neither a non-interrupting timer nor another task will be executed anymore.
	
	

*/


#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>			// uint8_t
#include <avr/wdt.h>		// wdt_enable, wdt_disable, wdt_reset
#include <math.h>			// ceil

#include "f_bytewise.h"
#include "f_callbacks.h"
#include "f_countdown.h"	// software_watchdog is a fsl::lg::countdown
#include "f_exceptional.h"
#include "f_flags.h"		// scheduler::flags, .. flags of table line
#include "f_interrupt.h"
#include "f_macros.h"		// macro_interrupt_critical_begin, macro_interrupt_critical_end, macro_interrupt_critical
#include "f_order.h"		// fsl::lg::min(..) for moving parts of table when reordering
#include "f_range_int.h"	// handle_type
#include "f_resettable.h"	// earliest_interrupting_timer_release
#include "f_system_time.h"	// 
#include "f_time.h"			// ExtendedMetricTime
#include "f_urgency.h"		// class urgency;

namespace fsl {
	namespace os {
		
		template <uint8_t TABLE_SIZE, uint8_t index_cache_size = 2>
		class scheduler {
			
			
			public:		/*** public types ***/
			
			using handle_type = fsl::lg::range_int<uint8_t,TABLE_SIZE,true,true>;
			
			enum class entry_type : uint8_t {
				TASK = 0, TIMER = 1, INTTIMER = 2, INVALID = 255
			};
			
			
			public:		/*** public static constexpr values ***/
			
			/* handle that refers to none of the entries in scheduler table */
			static constexpr handle_type NO_HANDLE{ handle_type::OUT_OF_RANGE };
			
			/* the default urgency for task entries in scheduler table */
			static constexpr urgency DEFAULT_URGENCY{ urgency::normal_urgency() };
			

			public:		/*** public static constexpr functions ***/
			
			/* returns size in bytes of one SchedulerMemoryLine */
			static constexpr uint8_t table_line_size_of(){ return sizeof(entry); }
			
			/* returns size in bytes of the whole scheduler table */
			static constexpr uint16_t table_size_of(){ return sizeof(entry)*TABLE_SIZE; }
			
			
			private:	/*** private types ***/
			
			/* integral type used by the software watchdog countdown object */
			using software_watchdog_base_type = uint32_t;
			
			///### add description!!!!
			struct index_cache {	handle_type handle;		uint8_t index;	};
			
			/* specific data only needed for tasks */
			struct task_specifics {
				
				urgency task_urgency;
				
				/* Always the task with the smallest task_race_countdown will be executed. If this is ambiguous one of them will be executed.
				After a task is executed task_race_countdown will be increased by urgency_inverse.
				At least when a task_race_countdown would overflow if it has to be increased, all task_race_countdowns have to be shorten together.
				To do so, determine the min task_race_countdown and subtract this value from all task_race_countdowns.
				*/
				uint8_t task_race_countdown;
			};

			/* specific data only needed for timers */
			struct timer_specifics {
				/* earliest time when timer can be executed */
				volatile time::ExtendedMetricTime* event_time;
			};
			
			/* class for union storage of timer / task - specific data */
			class union_specifics {
				private:
				union intern_union {
					task_specifics task;
					timer_specifics timer;
					intern_union() : timer({nullptr}) {}
				};
				intern_union _union;
				public:
				volatile task_specifics& task() volatile { return _union.task; }
				volatile timer_specifics& timer() volatile { return _union.timer; }
				task_specifics& task() { return _union.task; }
				timer_specifics& timer() { return _union.timer; }
				
				inline union_specifics& operator= (const union_specifics& rhs)				{	fsl::util::byte_copy(rhs,*this);	}
				inline union_specifics& operator= (const volatile union_specifics& rhs)		{	fsl::util::byte_copy(rhs,*this);	}
				inline void operator= (const union_specifics& rhs) volatile					{	fsl::util::byte_copy(rhs,*this);	}
				inline void operator= (const volatile union_specifics& rhs) volatile		{	fsl::util::byte_copy(rhs,*this);	}
			};
			
			/* class for one scheduler line containing everything needed for one timer / task entry */
			struct entry {
				handle_type handle;
				fsl::str::standard_union_callback callback;
				union_specifics specifics;
				fsl::lg::single_flags flags;
				
				entry(){}

				inline entry& operator=(const entry& rhs) { handle = rhs.handle; callback = rhs.callback; specifics = rhs.specifics; flags = rhs.flags; }
				inline entry& operator=(const volatile entry& rhs) { handle = rhs.handle; callback = rhs.callback; specifics = rhs.specifics; flags = rhs.flags; }
				inline void operator=(const entry& rhs) volatile { handle = rhs.handle; callback = rhs.callback; specifics = rhs.specifics; flags = rhs.flags; }
				inline void operator=(const volatile entry& rhs) volatile { handle = rhs.handle; callback = rhs.callback; specifics = rhs.specifics; flags = rhs.flags; }
			};
			
			
			private:	/*** private static constexpr values ***/
			
			/* flags for each scheduler entry / each scheduler table line */
			static constexpr fsl::lg::single_flags::flag_id ENTRY_VALID{ 0 }; // if an entry is not valid, the entry may be overwritten.
			static constexpr fsl::lg::single_flags::flag_id ENTRY_ENABLED{ 1 }; // if an entry is valid, then the entry will be considered on scheduling decisions iff is_enabled
			// timer will automatically be disabled right before executing, if you want the timer to execute again after a certain time, you have to re-enable its entry from inside the timer procedure.
			// And you might want to update the event_time in normal cases. To do so you can use my_handle() to get your registration handle and pass it to suitable scheduler methods.
			static constexpr fsl::lg::single_flags::flag_id ENTRY_TIMER{ 2 }; // if entry is valid, then entry (containing unions) is treated as a timer iff IS_Timer and as task otherwise.
			static constexpr fsl::lg::single_flags::flag_id ENTRY_INTERRUPTING{ 3 }; // if entry is_valid and is_timer, then callback may be executed as soon as event_time is reached on system time interrupt iff IS_INTTIMER. Otherwise is has to wait until the current task/timer returned.
			static constexpr fsl::lg::single_flags::flag_id ENTRY_CALLABLE{ 4 }; // if entry is_valid, the callback union will be treated as a callable iff IS_CALLABLE, otherwise as function pointer.

			/* flags once for every scheduler object */
			static constexpr fsl::lg::single_flags::flag_id RUNNING{ 0 }; // true iff run() runs // write access only by run()
			static constexpr fsl::lg::single_flags::flag_id STOP_CALLED{ 1 }; // if true, run() should return when current task returns.
			// if it is true no int timers will be executed anymore until scheduler was started again.
			static constexpr fsl::lg::single_flags::flag_id SOFTWARE_INTERRUPTS_ENABLE{ 2 }; // scheduler will only look for int timers at now time update if enabled
			static constexpr fsl::lg::single_flags::flag_id EMPTY_TABLE_DETECTED{ 3 }; // for function-local internal use in run() only.
			static constexpr fsl::lg::single_flags::flag_id ONE_INTERRUPT_TIMER_ONLY{ 4 }; // for function-local internal use in run() only.
				// set and reset of this flag must be available for one from the outside. #########
			
			
			private:	/*** private static constexpr fucntions ***/
			/* {0, 1, .. , TABLE_SIZE - 1} -> {0, 1, .. , TABLE_SIZE - 1} : x |-> (x - 1) mod TABLE_SIZE */
			inline static constexpr uint8_t minus_one_mod_table_size(uint8_t uint){
				return static_cast<uint8_t>( (static_cast<uint16_t>(uint) + static_cast<uint16_t>(TABLE_SIZE -1)) % static_cast<uint16_t>(TABLE_SIZE) );
			}
			
			private:	/*** private data ***/
			
			/* the scheduler object's flags */
			//volatile fsl::lg::single_flags flags;ßßß
			
			/* handle of currently running callback method's corresponding entry in scheduler table. */
			volatile handle_type callback_handle;
			
			/* table containing all timers and tasks
			table layout definition:
			0				:	interrupting timers [no gap]	:	VALID
			:	non-interrupting timers			:	VALID
			:	[maybe gap]						:  INVALID
			:	tasks [no gap]					:	VALID
			TABLE_SIZE		:
			/// <<<< later maybe <<< int task somewhere in table.
			
			
			About the entries' flags:
			An entry is gap if and only if not IS_VALID.
			An entry with IS_VALID but not IS_ENABLED is not gap and has to be inside its section,
			although it will not be treated by the scheduler since is is so-called disabled.
			
			Access to table:
			You must always leave the scheduler table in a layout-definition conform and non corrupted state.
			For modifying you must put any operation inside an maro_interrupt_critical() - environment (atomic section) if you are not the interrupt yourself.
			Of course reading the table won't disturb the table content anyway, but also reading should be done in atomic sections,
			since otherwise there is no particular guaranty for reading from a consistent table, since anyone could interrupt you and modify the table,
			even while you read one multi-byte datum.
			*/
			volatile entry table[TABLE_SIZE];

			// see: https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/Der_Watchdog
			/* timeout value for initialization of hardware watchdog
			must be greater than the time between two now time updates
			must be set to one of the predefined macro constans from avr/wdt.h like WDTO_15MS, WDTO_30MS, ... 
			initialized via ctor
			*/
			uint8_t hardware_watchdog_timeout; // really used in no interrupt environment ???? <<<<<< missing volatile
			
			/* software_watchdog.value() is a value proportional to the time which passes until the software watchdog "resets the controller",
			i.e. stops to reset the hardware watchdog with wdt_reset().
			exactly spoken, the value is the number of time_update_interrupt_handler() calls left until sw watchdog triggers controller reset.
			software_watchdog.reset() must be called on finishing of any non-interrupting called task or timer procedure.
			software_watchdog.count_down() must be called on every SysTime interrupt / now_time_update.
			software_watchdog.XXX_reset_value() {get, set} represents the number of now time updates between fresh reset and trigger of reboot.
			If and only if reset_value is 0 the software_watchdog is disabled.
			Notice in this case that watchdog runs in background to observe behavior of scheduler and of too long interrupt callbacks.
			*/			
			volatile fsl::lg::countdown<software_watchdog_base_type> software_watchdog;
			
			/* event time of the enabled interrupt timer with the smallest event time.
			on construction of scheduler it must be set to EMT::MIN()
			Whenever a new interrupt timers has been added or an existing one has been enabled, it must be updated to min(old, new_int_timer.event_time) */
			//volatile fsl::str::resettable<time::ExtendedMetricTime, 0> earliest_interrupting_timer_release; // <<<< not possible 
			volatile time::ExtendedMetricTime earliest_interrupting_timer_release;
			
			
			private:	/*** private methods ***/
			
			
			/* sets earliest_interrupting_timer_release to the min of earliest_interrupting_timer_release and t
			   to be called if you enable an interrupting timer or change it's execution time */
			inline void adjust_earliest_interrupting_timer_release(const time::ExtendedMetricTime t){ if (t < earliest_interrupting_timer_release) earliest_interrupting_timer_release = t; }
			
			/* returns index of table where given handle can be found
			only call from inside an atomic section
			returns TABLE_SIZE iff there is no valid entry with given handle */
			inline uint8_t get_index_uncached (handle_type handle){
				uint16_t candidate = handle;
				for (uint8_t i = 0; i < (TABLE_SIZE + 1) /2; ++i){
					if ((table[(candidate + i) % TABLE_SIZE].handle == handle) && (table[(candidate + i) % TABLE_SIZE].flags.get(ENTRY_VALID))) return (candidate + i) % TABLE_SIZE;
					if ((table[(candidate + TABLE_SIZE - i - 1) % TABLE_SIZE].handle == handle) && (table[(candidate + TABLE_SIZE - i - 1) % TABLE_SIZE].flags.get(ENTRY_VALID))) return (candidate + TABLE_SIZE - i - 1) % TABLE_SIZE;
				}
				return TABLE_SIZE; // there is no such entry
			}
			
			/* returns index of table where given handle can be found
			only call from inside an atomic section
			returns TABLE_SIZE iff there is no valid entry with given handle */
			uint8_t get_index(handle_type handle){
				static index_cache cache[index_cache_size];
				for(uint8_t i = 0; i < index_cache_size; ++i){
					if ((cache[i].handle == handle) && (table[cache[i].index].handle == handle)){
						const uint8_t index{ cache[i].index };
						if (i != 0) { // push nearer to front
							fsl::util::byte_swap(cache[i], cache[i-1]); //### change back to byte_swap!!!
						}
						return index; 
					}
				}
				uint8_t index = get_index_uncached(handle);
				if (index == TABLE_SIZE) return TABLE_SIZE;
				for (uint8_t i = index_cache_size - 1; i > 1; ++i){
					cache[i] = cache[i-1];
				}
				cache[0].index = index;
				cache[0].handle = handle;
				return index;
			}
			
			/* executes a callback. if callback pointer is nullptr, nothing will be done */
			/* if (cimota) interrupts will be activated before procedure call and deactivated after procedure call */
			template <bool cimota>
			inline void execute_callback(fsl::str::standard_union_callback callback, bool is_callable){
				if (cimota) sei(); // replace with the future lite_cimota implementation!
				(is_callable) ? callback.call_callable() : callback.call_function();
				if (cimota) cli();
				}
			
			/* executes callback of given table index.
			The callback will automatically be checked if it is nullptr, in this case nothing will be done
			Make sure that table is non-volatile during function call - You should own table inside an atomic section or you should be any direct or indirect subroutine of ISR. */
			/* if (cimota) interrupts will be activated before procedure call and deactivated after procedure call */
			template <bool cimota>
			inline void execute_callback(uint8_t table_index){
				return execute_callback<cimota>(table[table_index].callback,table[table_index].flags.get(ENTRY_CALLABLE));
			}
			// both functions are never used with cimota != false
			
			/* first checks earliest_interrupting_timer_release whether following stuff must be done:
			goes through all (valid) int timer entries and executes the callback if execution_time expired and entry is enabled.
			Method must not be interrupted during its execution. It does not explicitly turns off interrupts since it should only be called as interrupt subroutine.
			So it assumes to always run uninterrupted
			returns	false:	earliest_time was not reached, no iteration through table, no callback executed
			returns true:	algorithm went through table, probably executing at least one callback (if earliest_interrupting_timer_release was not earlier than what it should mean), and earliest_interrupting_timer_release was updated.
			*/
			inline bool execute_interrupting_timers(){
				time::ExtendedMetricTime now = fsl::os::system_time::get_instance()();
				if (now < earliest_interrupting_timer_release)		return false;
				time::ExtendedMetricTime new_earliest_interrupting_timer_release = time::ExtendedMetricTime::MAX();
				
				/* @when here:
				We must check for expired int timers and update earliest_interrupting_timer_release
				because earliest_interrupting_timer_release expired.
				We exspect the table to be in a consistent state.
				*/
				uint8_t index = 0;
				
				while(index < TABLE_SIZE){
					if ((table[index].flags.get(ENTRY_VALID) == false) || (table[index].flags.get(ENTRY_TIMER) == false) || (table[index].flags.get(ENTRY_INTERRUPTING) == false)){ // end of int-timer section
						break;
					}
					// @when here: at [index] we see a valid entry that is of type int-timer.
					if (table[index].flags.get(ENTRY_ENABLED)){
						if (now >= *table[index].specifics.timer().event_time){
							table[index].flags.set_false(ENTRY_ENABLED);
							handle_type local_stack = callback_handle;
							callback_handle = table[index].handle;
							execute_callback<false>(index);
							callback_handle = local_stack;
							//ßßßif (flags.get(ONE_INTERRUPT_TIMER_ONLY)){
							if (true){
								// prevention against some infinite loop of executing int timers. Next timer should wait for next now_time_update.
								// earliest_interrupting_timer_release is still in the past.
								return true;
							} else {
								// start from begin again, because table might be modified by callback procedure.
								index = 0;
								continue;
							}
							// <<< one of those two strategies must be chosen.
						} else {
							if (new_earliest_interrupting_timer_release > *table[index].specifics.timer().event_time){ // if found smaller one then replace
								new_earliest_interrupting_timer_release = *table[index].specifics.timer().event_time;
							}
						}
					}
					++index;
				}
				/*@ reached end of int timer section, all expired timers executed */
				earliest_interrupting_timer_release = new_earliest_interrupting_timer_release;
				return true;
			}
			
			/* get entry type of given handle. only call from inside an atomic section */
			inline entry_type get_entry_type_unsave(handle_type handle){
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) return entry_type::INVALID;
				return table[index].flags.get(ENTRY_TIMER) ? (table[index].flags.get(ENTRY_INTERRUPTING) ? entry_type::INTTIMER : entry_type::TIMER) : entry_type::TASK;
			}
			
			/* prepares an unused (= not IS_VALID) line to add a new task or timer or int-timer there
			returns TABLE_SIZE if the table is full. In this case the table is not changed
			otherwise returns the index of the line which was prepared.
			The prepared line comes with a free handle stored inside this line.
			It is already marked as IS_VALID, as task / timer / int-timer as given in parameters and as not IS_ENABLED
			only call from inside of a critical_section which is already save against interrupts
			*/
			uint8_t new_table_line(bool is_task, bool is_interrupting){
				/* is_task :<=> false for any timer type, true for any task, is_interrupting: irrelevant for any task */
				/* search for invalid entry to replace: */
				uint8_t index;
				// we look from max index to min when we want to add a new task, because of table layout [int timer, timer, gaps, tasks]
				// we look from min index to max when we want to add a new timer, because of table layout.
				for(index = is_task * (TABLE_SIZE -1); (index < TABLE_SIZE); is_task ? --index : ++index){
					if (table[index].flags.get(ENTRY_VALID) == false) goto found_invalid_entry;
				}
				/*@when here: The table is already full */
				return TABLE_SIZE;
				
				found_invalid_entry:
				uint8_t free_index{ index };
				/*@when here: The table has a free line, we stored that index as free_index,
				there must be also a free handle since there is at least one free line in the table */
				handle_type free_handle{ NO_HANDLE };
				
				// search for free handle
				constexpr uint8_t c_candidates{ 3 };
				handle_type candidates[c_candidates] = {handle_type(index), table[index].handle, handle_type((index + 1) % TABLE_SIZE)};
				
				/* strategies:
				
				1:	prefer handle == index in table:
				update rule on found forbidder: decrement
				description: task index may grow up, timer index my become smaller during handler life, when task with greater / timer with smaller index is deleted
				
				2:	handle = handle of last entry that was written here.
				update rule on found forbidder: new index = index of conflict, i.e. index of the forbidder.
				description: because this handle is possibly free since the entry was marked disabled
				
				3:	[same like 1, but different update strategy]
				prefer handle == index in table:
				update rule on found forbidder: increment
				description: task index may grow up, timer index my become smaller during handler life, when task with greater / timer with smaller index is deleted
				
				*/
				uint8_t check_until_index[c_candidates] = {index, index, index};
				
				while (true){
					index = (index + 1) % TABLE_SIZE; // increment table index
					// check that there is no crash between candidate and the current line:
					if (table[index].flags.get(ENTRY_VALID)){ // if current line is valid, any candidate must not equal the current line's handle.
						for(uint8_t i = 0; i < c_candidates; ++i){
							if (table[index].handle == candidates[i]){
								// candidate is already used at current index, use different update strategies for candidates:
								if (i == 0){
									// idea: go through all possible handles by moving --
									candidates[i] = minus_one_mod_table_size(candidates[i]);
									} else if (i == 1){
									// idea: if we wanted an handle that equals it's entry's index but we found this forbidder here, try the forbidders index
									candidates[i] = index;
									} else { /* (i == 2) */
									//idea: just go ++, opposite direction compared to case (i == 0)
									candidates[i] = (static_cast<uint8_t>(candidates[i]) + 1) % TABLE_SIZE; // <<< should candidates rather be array of uint8_t????
								}
								check_until_index[i] = minus_one_mod_table_size(index);
							}
						}
					}
					// look whether there is a candidate for that we have a proof that it is indeed unused.
					for(uint8_t i = 0; i < c_candidates; ++i){
						if (check_until_index[i] == index){
							// this candidate is a free handle
							free_handle = candidates[i];
							goto found_free_handle;
						}
					}
				}
				
				found_free_handle:
				/* @when here: free_index is an index of an unused line, free_handle is an unused handle.
				if we want to create an interrupting timer we need to swap position with first non_int timer
				if we want to create a task or a non-int timer, then free_index is already in the right table section.
				*/
				if ((!is_task) && (is_interrupting)){ // we need to swap a new int timer line to the section of int timers!
					// Check whether predecessors of our int-timer are non-int timers.
					for(index = free_index; true; --index ){ // check about a swap of index and free_index
						if ((index == 0) || (table[index-1].flags.get(ENTRY_INTERRUPTING))) break; // swap with current index
					}
					// we need to swap:
					// move from index to free_index
					
					// if (free_index != index):
					table[free_index] = table[index]; 
					free_index = index;
				}
				table[free_index].handle = free_handle;
				table[free_index].flags.set_true(ENTRY_VALID);
				table[free_index].flags.set(ENTRY_TIMER, ! is_task);
				table[free_index].flags.set(ENTRY_INTERRUPTING, is_interrupting);
				table[free_index].flags.set_false(ENTRY_ENABLED);
				return free_index;
			}
			
			// <<<<< maybe offer a second, unsave version
			/* activate hardware watchdog and set the timeout to hardware_watchdog_timeout */
			inline void activate_hardware_watchdog(){
				macro_interrupt_critical(
				wdt_enable(hardware_watchdog_timeout);
				wdt_reset();
				);
			}
			
			// <<<<<< maybe offer a second, unsave version
			/* deactivate hw watchdog */
			inline void deactivate_hardware_watchdog(){
				macro_interrupt_critical(
				wdt_disable();
				);
			}
			
			
			public:		/*** public constructor ***/
			
			/* for hardware_watchdog_timeout use the predefined macros from avr/wdt.h like WDTO_15MS, WDTO_30MS, ... or consult your MCUs data sheet. */
			/* the value must be greater than the time between two now time updates. */
			//### bad description
			inline scheduler(bool software_interrupt_enable, uint8_t hardware_watchdog_timeout, const time::ExtendedMetricTime& watchdog) /*: ßßß
				flags(0), callback_handle(NO_HANDLE), hardware_watchdog_timeout(hardware_watchdog_timeout), earliest_interrupting_timer_release(time::ExtendedMetricTime::MIN())*/ {
				//clear_table();ßßß
				//activate_software_watchdog(watchdog);ßßß
				}
			
			
			public:		/*** public methods ***/
			
			/* activate software interrupts / activate interrupting timers
			notice that these functions will also be executed, but only by non-interrupting calls */
			void enable_software_interrupts(){ macro_interrupt_critical( flags.set_true(SOFTWARE_INTERRUPTS_ENABLE);); }

			/* deactivate software interrupts / deactivate interrupting timers
			notice that these functions will also be executed, but only by non-interrupting calls */
			void disable_software_interrupts(){ macro_interrupt_critical( flags.set_false(SOFTWARE_INTERRUPTS_ENABLE);); }
			
			inline handle_type my_handle(){ return callback_handle; }
			
			inline bool is_running(){ return flags.get(RUNNING); }
			
			inline bool was_stop_called(){ return flags.get(STOP_CALLED); }
			
			/* get entry type of given handle */
			inline entry_type get_entry_type(handle_type handle){
				entry_type result;
				macro_interrupt_critical( result = get_entry_type_unsave(handle); );
				return result;
			}
			
			/* set task_race_countdown to 0. The task will be executed at least before any task that has race_countdown > 0.
			returns		0:	reset done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t push_task_to_front(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().task_race_countdown = 0;
				);
				return error_code;
			}
			/* <<<<<<
			duplicate description: $598
			We may use accessor object in future versions to avoid multiple duplications of 
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				...
			accessor classes may be derived from an atomic class
			
			*/
			
			/* set task_urgency_inverse. This value must not be 0. 0 will replaced by 1.
			returns		0:	done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t set_task_urgency(handle_type handle, urgency urg){
				uint8_t error_code{ 0 };
				macro_interrupt_critical( // duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().task_urgency = urg;
				);
				return error_code;
			}
			
			/* reset task_urgency_inverse to standard urgency.
			returns		0:	done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t reset_task_urgency_inverse(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical( // duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().urgency_inverse = DEFAULT_URGENCY;
				);
				return error_code;
			}
			
			/* increase task_urgency / decrease_task_urgency_inverse.
			returns		0:	done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t increase_task_urgency(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(// duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().increase_urgency();
				);
				return error_code;
			}

			/* decrease task_urgency / increase_task_urgency_inverse.
			returns		0:	done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t decrease_task_urgency(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(// duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().decrease_urgency();
				);
				return error_code;
			}
			
			/* Set callback of entry associated with given handle.
			returns 0 if successful, returns 1 if handle does not meet a valid entry. */
			inline uint8_t set_callback(handle_type handle, decltype(nullptr) callback){	return reset_callback(handle,fsl::str::void_function(nullptr));	}
			
			/* Set callback of entry associated with given handle.
			returns 0 if successful, returns 1 if handle does not meet a valid entry. */
			inline uint8_t set_callback(handle_type handle, fsl::str::void_function callback){
				macro_interrupt_critical_begin; // duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].callback.set_function_ptr(callback);
				table[index].flags.set_false(ENTRY_CALLABLE);
				macro_interrupt_critical_end;
				return 0;
			}
			
			/* Set callback of entry associated with given handle.
			returns 0 if successful, returns 1 if handle does not meet a valid entry. */
			inline uint8_t set_callback(handle_type handle, fsl::str::callable* callback){
				macro_interrupt_critical_begin;// duplicate: see push_task_to_front $598
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].callback.set_callable_ptr(callback);
				table[index].flags.set_true(ENTRY_CALLABLE);
				macro_interrupt_critical_end;
				return 0;
			}
			
			/* set callback procedure to nullptr */
			inline uint8_t reset_callback(handle_type handle){ return set_callback(handle,nullptr); }
			
			/* Write the associated standard_union_callback into callback, set is_callable flag of entry matching to given handle
			returns 0 if successful,
			returns 1 if there was no matching entry for given handle, anything remains unchanged
			*/
			inline uint8_t get_callback(handle_type handle, fsl::str::standard_union_callback& callback, bool& is_callable){
				// callback.set_function_ptr(nullptr); // unchanged
				// is_callable = false; // unchanged
				uint8_t index;
				macro_interrupt_critical(
				index = get_index(handle);
				if (index != TABLE_SIZE) {
					is_callable = table[index].flags.get(ENTRY_CALLABLE);
					callback = table[index].callback;
				}
				);
				return !(index != TABLE_SIZE);
			}
			
			/* execute callback of given handle, if handle is valid and callback is nun-nullptr 
			return 0: everything successful
			return 1: no matching entry for given handle, nothing done */
			inline uint8_t execute_callback(handle_type handle){
				fsl::str::standard_union_callback callback;
				bool is_callable;
				/*@when here: callback is nullptr. */
				if (get_callback(handle,callback,is_callable)) return 1;
				handle_type local_stack = callback_handle;
				callback_handle = handle;
				execute_callback<false>(callback,is_callable);
				callback_handle = local_stack;
				return 0;
			}
			
			/* delete entry corresponding to given handle (and reorder table)*/
			/* return 0: successful
			   return 1: no matching scheduler table entry
			*/
			inline uint8_t clear_entry(handle_type handle){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].flags.set_false(ENTRY_VALID);
				/* reorder table: */
				while ((index != TABLE_SIZE - 1) && (table[index + 1].flags.get(ENTRY_VALID)) && (table[index + 1].flags.get(ENTRY_TIMER)) ){
					fsl::util::byte_swap(table[index], table[index+1]);
					++index;
				}
				while ((index != 0) && (table[index - 1].flags.get(ENTRY_VALID)) && (table[index + 1].flags.get(ENTRY_TIMER) == false) ){
					fsl::util::byte_swap(table[index], table[index-1]);
					--index;
				}
				macro_interrupt_critical_end;
				return 0;
			}
			
			/* returns true (1) if entry corresponding to handle is enabled, otherwise false (0),
			   returns 255 (~true) if no entry matches handle */
			inline uint8_t get_entry_enable(handle_type handle){
				uint8_t result;
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				result = (index == TABLE_SIZE) ? 255 : table[index].flags.get(ENTRY_ENABLED);
				);
				return result;
			}
			
			/* set enable flag of entry corresponding to handle
			return 0: successful
			return 1: no matching entry, aborted
			*/
			inline uint8_t set_entry_enable(handle_type handle, bool enable){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].flags.set(ENTRY_ENABLED,enable);
				macro_interrupt_critical_end;
				return 0;
			}
			
			/* see description of set_entry_enable */
			inline uint8_t enable_entry(handle_type handle){ return set_entry_enable(handle,true); }
			
			/* see description of set_entry_enable */
			inline uint8_t disable_entry(handle_type handle){ return set_entry_enable(handle,false); }
			
			/* returns the number of interrupting timers.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of int timers in between. */
			uint8_t count_int_timers(){
				fsl::hw::simple_atomic atomic; // open atomic section
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (table[pivot].flags.get(ENTRY_VALID) && table[pivot].flags.get(ENTRY_TIMER) && table[pivot].flags.get(ENTRY_INTERRUPTING)){
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return lower_bound;
			}
			// <<< to avoid much dups one could use some function for counting table sections, giving some evaluation function that decribes the section.
			// count_entry_section(   bool(scheduler<..>::* evaluator)(uint8_t index)   )
			
			/* returns the number of all timers.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of int timers in between. */
			uint8_t count_all_timers(){
				fsl::hw::simple_atomic atomic; // open atomic section
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (table[pivot].flags.get(ENTRY_VALID) && table[pivot].flags.get(ENTRY_TIMER)){
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return lower_bound;
			}
			
			/* returns the number of tasks.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of int timers in between. */
			uint8_t count_tasks(){
				fsl::hw::simple_atomic atomic; // open atomic section
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (!(table[pivot].flags.get(ENTRY_VALID) && (!table[pivot].flags.get(ENTRY_TIMER)))){ /* not a valid task */
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return TABLE_SIZE - lower_bound;
			}
			
			/* returns the number of non-interrupting timers.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of int timers in between. */
			inline uint8_t count_non_int_timers(){	fsl::hw::simple_atomic atomic; return count_all_timers()-count_int_timers();	}
			// here we have duplicated simple_atomics because of function calls, maybe create private unsave versions und public save wrappers for functions. <<<<<
			
			/* returns the number of gap lines.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of int timers in between. */
			inline uint8_t count_gaps(){ fsl::hw::simple_atomic atomic; return TABLE_SIZE - count_all_timers() - count_tasks(); }
			// here we have duplicated simple_atomics because of function calls, maybe create private unsave versions und public save wrappers for functions. <<<<<
			
			/* returns true iff table is full.
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of entries in between. */
			inline bool table_full(){ return count_gaps() == 0; }
			
			/* returns true iff table is empty
			   you may call this function from anywhere,
			   but call from inside an atomic section, if you want guaranty that no interrupt changes number of entries in between. */
			inline bool table_empty(){ return count_gaps() == TABLE_SIZE; }
			
			/* convert timer to int-timer.
			returns		0:	conversion done without any errors
						1:	invalid handle, nothing done
						2:	handle is associated with a task entry
						4:	handle is already listed as int-timer. no conversion necessary */
			uint8_t convert_to_int_timer(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				if (!error_code)	if (table[index].flags.get(ENTRY_INTERRUPTING)) error_code = 4;
				if (!error_code){
					const uint8_t swap { count_int_timers() };
					byte_swap(table[index],table[swap]);
					table[swap].flags.set_true(ENTRY_INTERRUPTING);
				}
				);
				return error_code;
			}
			
			/* convert timer to non-int-timer.
			returns		0:	conversion done without any errors
						1:	invalid handle, nothing done
						2:	handle is associated with a task entry
						4:	handle is already listed as normal timer. no conversion necessary
			*/
			uint8_t convert_to_normal_timer(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				if (!error_code)	if (!table[index].flags.get(ENTRY_INTERRUPTING)) error_code = 4;
				if (!error_code){
					const uint8_t swap { count_int_timers() - 1 };
					byte_swap(table[index],table[swap]);
					table[swap].flags.set_false(ENTRY_INTERRUPTING);
				}
				);
				return error_code;
			}
			
			/* set entry-internal time object pointer to a new time object.
			returns		0:	done without any errors
						1:	invalid handle, nothing done
						2:	handle is associated with a task entry
			*/
			inline uint8_t set_event_time_object(handle_type handle, volatile time::ExtendedMetricTime& time_object){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(ENTRY_TIMER)) error_code = 2;
				if (!error_code)	{
					table[index].specifics.timer().event_time = &time_object;
					if ((table[index].flags.get(ENTRY_INTERRUPTING)) && (table[index].flags.get(ENTRY_ENABLED))){
						adjust_earliest_interrupting_timer_release(time_object);
					}
				}
				);
				return error_code;
			}
			
			/* return pointer to time object associated to given timer handle (if possible)
			   returns nullptr if there is no valid entry matching handle or matching entry is not of type timer
			   You may cast the 'const' away of the returned value to modify the time despite this is deprecated,
			   but after modifying event_time you should update / reset earliest_interrupting_timer_release in case it is an interrupting timer and you change time to earlier.
			   You can do this update by disabling and enabling entry again. Or call reset_int_timer_prefetcher(); .
			*/
			inline const volatile time::ExtendedMetricTime* get_event_time(handle_type handle){
				volatile time::ExtendedMetricTime* result{ nullptr };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index != TABLE_SIZE) if (table[index].flags.get(ENTRY_TIMER)) result = table[index].specifics.timer().event_time;
				);
				return result;
			}
			/* set event time referenced by the entry associated with handle to given time t */
			/* return 0: successful
			   return 1: no matching timer entry for given handle */
			inline uint8_t set_event_time(handle_type handle, const time::ExtendedMetricTime& t){
				volatile time::ExtendedMetricTime* event_time = const_cast<volatile time::ExtendedMetricTime*>(get_event_time(handle));
				if (!event_time) return 1;
				macro_interrupt_critical(
				if (t < *event_time) reset_int_timer_prefetcher();
				// for better performance: just consider the effected table entry and check whether we got an int_timer, if so update earliest_... to min of self and t.
				// this can be easily done as soon as we have accessor objects / (classes). Then this method would become method of timer_entry-specific accessor class.
				*event_time = t;
				);
				return 0;
			}
			
			/* resets the earliest_interrupting_timer_release value to its minimum to invoke its calculation by scheduler.
			   this function should not be called unless some description tells you to do so. */
			/* <<< as soon as accessor classes are available this method is supposed to become useless. */
			inline void reset_int_timer_prefetcher(){ earliest_interrupting_timer_release = time::ExtendedMetricTime::MIN(); }
			
			/* returns true if given handle matches to valid task entry */
			inline bool is_task(handle_type handle){ return get_entry_type(handle) == entry_type::TASK; }
			
			/* returns true iff given handle matches to valid non-int timer entry */
			inline bool is_non_int_timer(handle_type handle){ return get_entry_type(handle) == entry_type::TIMER; }
			
			/* returns true iff given handle matches to valid int-timer entry */
			inline bool is_int_timer(handle_type handle){ return get_entry_type(handle) == entry_type::INTTIMER; }
			
			/* returns true iff given handle matches to valid (int- or non-int) timer entry */
			inline bool is_any_timer(handle_type handle){ return is_non_int_timer() || is_int_timer(); }
			// is inefficient since one flag check would be enough, also get_index will be called twice <<<<<
			// but improving should wait until we have accessor classes that cover this feature method.
			
			/* invalidate all table entries */
			void clear_table(){	macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) table[i].flags.set_false(ENTRY_VALID);); }
			
			/* invalidate all task entries */
			inline void clear_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(ENTRY_TIMER)) table[i].flags.set_false(ENTRY_VALID););}
			
			/* invalidate all non-int-timer and all int-timer entries */
			inline void clear_all_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(ENTRY_TIMER)) table[i].flags.set_false(ENTRY_VALID););}

			/* invalidate all int-timer entries */
			inline void clear_int_timers(){
				macro_interrupt_critical(
				uint8_t count_int_timer{ count_int_timers() };
				uint8_t count_timer{ count_non_int_timers() };
				// reorder table and overwrite / delete int_timers :
				const uint8_t move_size{ fsl::lg::min(count_int_timer, count_timer) };
				const uint8_t move_src_begin{ count_int_timer + count_timer - move_size };
				for(uint8_t i = 0; i < move_size; ++i){
					const uint8_t src{ move_src_begin + i };
					table[i] = table[src];
					table[src].flags.set_false(ENTRY_VALID);
				}
				);
			}

			/* invalidate all non-int-timer entries */
			inline void clear_non_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(ENTRY_TIMER) && (!table[i].flags.get(ENTRY_INTERRUPTING))) table[i].flags.set_false(ENTRY_VALID););}
			
			/* disable all table entries */
			void disable_table(){	macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) table[i].flags.set_false(ENTRY_ENABLED);); }
			
			/* disable all task entries */
			inline void disable_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(ENTRY_TIMER)) table[i].flags.set_false(ENTRY_ENABLED););}
			
			/* disable all non-int-timer and all int-timer entries */
			inline void disable_all_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(ENTRY_TIMER)) table[i].flags.set_false(ENTRY_ENABLED););}

			/* disable all int-timer entries */
			inline void disable_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(ENTRY_TIMER) && table[i].flags.get(ENTRY_INTERRUPTING)) table[i].flags.set_false(ENTRY_ENABLED););}

			/* disable all non-int-timer entries */
			inline void disable_non_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(ENTRY_TIMER) && (!table[i].flags.get(ENTRY_INTERRUPTING))) table[i].flags.set_false(ENTRY_ENABLED););}
			
			/* enable all task entries */
			inline void enable_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(ENTRY_TIMER)) table[i].flags.set_true(ENTRY_ENABLED););}
			
			/* executes all stuff that should be done on now_time update, including hardware watchdog reset, int-timer execution e.a.
			should be called once on every update of the now time and only called by interrupt, children function assume interrupt-freedom */
			inline void time_update_interrupt_handler(){
				//ßßßif (flags.get(RUNNING)){
					if (true){
					if ((software_watchdog.get_reset_value() == 0) /*software watchdog disabled*/ || (software_watchdog.value() > 0) /*software watchdog not exceeded*/)
					{
						--software_watchdog;
						wdt_reset();
					}
					//ßßßif (flags.get(SOFTWARE_INTERRUPTS_ENABLE) && (flags.get(STOP_CALLED) == false)) execute_interrupting_timers();
				}
			}
			
			/* set value of timeout of hardware, use predefined macros WDTO_15MS, WDTO_30MS ... 
				stops and starts hardware watchdog to apply change */
			inline void set_hardware_watchdog_timeout(uint8_t value){
				hardware_watchdog_timeout = value;
				//ßßßif (flags.get(RUNNING)){
					if (true){
					deactivate_hardware_watchdog(); // <<<< inefficient because twice open and close atomic section, and please check, if it is valid just to do an enable of hw wd (even if it is already enabled) and whether an atomic section is necessary around all of the calls to activate deactivate or reset the watchdog.
					activate_hardware_watchdog();
				}
			}
			
			/* deactivates software watchdog
			note that hardware watchdog is still in use and reboots controller if
			* you use too long interrupt timer handlers or
			* you use atomic sections that are far too long
			* if the Scheduler itself would be corrupt and deactivates interrupts for too long
			*/
			inline void deactivate_software_watchdog(){		macro_interrupt_critical(software_watchdog.set_reset_value(0););	}
			
			/* activate the software watchdog for running scheduler.
			software watchdog will be startet when you call run() and stopped before run() returns,
			but may be enabled before you call run() and really should be if you want to have such a software watchdog.
			Set the template parameters this way you want the time to be rounded or use their defaults.
			If any time <= 0 is given the software watchdog will be disabled.
			*/
			template <bool round_to_ceiling = false, bool truncate = false, bool truncate_and_add_one = true>
			inline void activate_software_watchdog(const time::ExtendedMetricTime& watchdog){
				static_assert(0 + round_to_ceiling + truncate + truncate_and_add_one == 1, "There must be one template argument being true and two being false");
				if (watchdog <= 0) return deactivate_software_watchdog();
				macro_interrupt_critical(
				if (round_to_ceiling){ // constexpr
					software_watchdog.set_reset_value(
						static_cast<software_watchdog_base_type>(// <<<< use type as param
							ceil(
								watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
							) // <<<<<< what about overflows here ????
						)
					);
				}
				if (truncate){ // constexpr
					software_watchdog.set_reset_value(
						static_cast<software_watchdog_base_type>(
							watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
						)
					);
				}
				if (truncate_and_add_one){ // constexpr
					software_watchdog.set_reset_value(
						static_cast<software_watchdog_base_type>(
							watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
						)
						+ 1
					);
				}
				software_watchdog.reset();
				); // macro_interrupt_critical
				// <<<<< this function could be written in a different way where we just declare a template function and implement 3 variants with specific set template parameters
			}
			
			/* central control loop / scheduling loop to call after construction  of scheduler, executes all tasks and timers.
			return code:
			0	:	scheduler terminated because of empty table
			1	:	scheduler terminated because of STOP call from inside
			starts the hardware watchdog at the beginning of run() and stops it before run() returns (if it returns)
			hardware watchdog is set up with the hardware_watchdog_timeout.
			Since it activates the hardware watchdog, make sure that the now_time_update_interrupt_handles is called again and again.
			*/
			uint8_t run(){
				static_assert(TABLE_SIZE >= 1, "Table declared with size 0.");
				/*flags.set_false(STOP_CALLED); // delete previous stop command
				flags.set_true(RUNNING); // set running flag
				flags.set_false(EMPTY_TABLE_DETECTED); // check for empty table
				ßßß*/
				activate_hardware_watchdog();
				software_watchdog.reset();// muss diese zeile in eine crit section???? <<<<<< wo wird der watchdog initialisiert?
				
				central_control_loop:
				while(true){
					
					uint8_t choice; // index inside
					macro_interrupt_critical_begin;
					software_watchdog.reset();
					
					//if (flags.get(STOP_CALLED)){
						if (true){
						// @when here: while loop must terminated because of a stop() call:
						deactivate_hardware_watchdog();// wdt_disable(); // stop watchdog
						//ßßßflags.set_false(RUNNING); // delete running flag
						macro_interrupt_critical_end;
						return 1;
					}
					
					/*** look for a timer that can be executed ***/
					
					time::ExtendedMetricTime now = fsl::os::system_time::get_instance()();
					for(choice = 0; (choice < TABLE_SIZE) && (! ((table[choice].flags.get(ENTRY_VALID) == false) || (table[choice].flags.get(ENTRY_TIMER) == false)) ); ++choice){
						// at current index we see a valid timer (int or non-int)
						if (table[choice].flags.get(ENTRY_ENABLED)){
							// timer is enabled.
							//ßßßflags.set_false(EMPTY_TABLE_DETECTED);
							if (now > *table[choice].specifics.timer().event_time) {
								// timer has expired.
								table[choice].flags.set_false(ENTRY_ENABLED); // disable timer to avoid execution twice
								goto execute;
							}
						}
						// side affect of timer order in table: interrupting timers are always executed first
					}
					/*** end of looking for timer ***/
					
					// @when here: table may have or may not have timers or may consist only of timers inside, but no of the possibly present timers in table expired
					// EMPTY_TABLE_DETECTED was set to false when looking through timers if there was at least one enabled timer
					
					//ßßßif (flags.get(EMPTY_TABLE_DETECTED)){ // table empty, nothing to schedule anymore
						if (true){
						deactivate_hardware_watchdog();
						///ßßßflags.set_false(RUNNING);
						macro_interrupt_critical_end;
						return 0;
					}
					
					/*** look for a task that can be executed ***/
					{
						/*
						different strategies for task selection:
						
						-> TIDY_UP_STRATEGY
						1. go through all task until you find one with 0 racing, replace 0 racing by reset value and choose this task goto execute.
						   remember always the index with lowest racing value.
						2. [else] if no value == 0 found go through all entries again and subtract the min racing value that was detected.
						   take the one entry with the minimum as choice, reset it to its reset value and execute it.
						evaluation:
						=> in up to the half of all cases we wont find 0, => we go twice through the list, the maybe-only 0 will be shifted away.
						=> perhaps at next iteration we wont find a 0 again.

						-> REBASE_WHEN_OVERFLOW_STRATEGY
						1. alway search for the minimum, global minimum, if found zero we can break earlier.
						   execute this minimum and try to add its distance to its racing value.
						2. (only) if rancing value is overflowing that we need to go through all entries and make them all smaller.
						evaluation:
						=> in comparison to TIDY_UP we still need to do such "subtract x from all entries", but less often than with TIDY_UP

						->	MOVING_ZERO_STRATEGY
						1. define a static variable last_minimum.
						2. determine x = argmin{x}{last_minimum + x mod YYY = race_coundown of any task T}
						3. execute a task which is a witness for x to be argmin.
						4. set last_minimum = x.
						evaluation:
						=> we never need to rebase coundowns
						=> we always go just once through the whole task section.
						=> we need one more byte to store last_minimum

						//// ###think about volatile stuff in this class
						*/

						uint8_t min_entry_index{ TABLE_SIZE }; // means no entry
						uint16_t min_entry_race_countdown{ 0xFFFF }; // means no entry
						
						/** look for a task with task_race_countdown == 0 **/
						for (choice = TABLE_SIZE - 1; (choice < TABLE_SIZE) && (table[choice].flags.get(ENTRY_VALID) == true) && (table[choice].flags.get(ENTRY_TIMER) == false); --choice){
							// choice is an index where we see a valid task entry
							if (table[choice].flags.get(ENTRY_ENABLED)){
								// current task has to be considered as it is enabled
								if (table[choice].specifics.task().task_race_countdown == 0){
									goto task_execute;
								}
								if (static_cast<uint16_t>(table[choice].specifics.task().task_race_countdown) < min_entry_race_countdown){
									min_entry_index = choice;
									min_entry_race_countdown = table[choice].specifics.task().task_race_countdown;
								}
							}
						}
						
						/** end of looking for task with countdown == 0 **/
						
						// @when here: reached end of (maybe empty, maybe max-sized, maybe between) task section, but no task_race_countdown was zero
						
						if (min_entry_index == TABLE_SIZE){ // <==> section has no enabled task <==> task section is called empty (but not 0-sized i.g. because of possibly disabled task entries)
							//ßßßflags.set_true(EMPTY_TABLE_DETECTED);
							macro_interrupt_critical_end;
							goto central_control_loop; // the task section is empty at the moment, run scheduler from begin.
						}
						
						// @when here: reached end of non-empty task section (at least one task is valid and enabled), but no task_race_countdown was zero
						
						/** subtract minimum task_race_countdown from all count_downs in task section and let the min-countdown task be executed afterwards **/
						for (choice = TABLE_SIZE - 1; (choice < TABLE_SIZE) && (table[choice].flags.get(ENTRY_VALID) == true) && (table[choice].flags.get(ENTRY_TIMER) == false); --choice){
							if (table[choice].flags.get(ENTRY_ENABLED)){
								table[choice].specifics.task().task_race_countdown -= static_cast<uint8_t>(min_entry_race_countdown);
							}
						}
						choice = min_entry_index;
						
						task_execute:
						table[choice].specifics.task().task_race_countdown = table[choice].specifics.task().task_urgency.inverse_value();
					}
					execute:
					fsl::str::standard_union_callback callback = table[choice].callback;
					handle_type local_stack = callback_handle;
					callback_handle = table[choice].handle;
					choice = table[choice].flags.get(ENTRY_CALLABLE);
					macro_interrupt_critical_end;
					
					execute_callback<false>(callback,choice);
					callback_handle = local_stack;
				}
			}

			/* make scheduler return after current executed task / timer */
			//ßßßinline void stop(){		macro_interrupt_critical(flags.set_true(STOP_CALLED););		}
				inline void stop(){}

			/*
			try to add a new task to the table.
			returns SchedulerHandle of created task
			if not successful NO_HANDLE is returned. In this case the table is already full with valid entries.
			If both possible callback variants are non-nullptr the callable will be preferred.
			*/
			handle_type new_task(fsl::str::callable* callable = nullptr, fsl::str::void_function function = nullptr, urgency task_urgency = DEFAULT_URGENCY, bool enable = true){
				macro_interrupt_critical_begin;
				
				uint8_t free_index = new_table_line(true,false);
				if (free_index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return NO_HANDLE;
				}
				// table[free_index].flags.set_false(ENTRY_ENABLED); this is already the default value coming from new_table_line
				if (enable) table[free_index].flags.set_true(ENTRY_ENABLED);
				// callback and callback flag:
				if (callable == nullptr){
					table[free_index].callback.set_function_ptr(function);
					table[free_index].flags.set_false(ENTRY_CALLABLE);
					} else {
					table[free_index].callback.set_callable_ptr(callable);
					table[free_index].flags.set_true(ENTRY_CALLABLE);
				}
				// task specific information: task urgency and countdown:
				table[free_index].specifics.task().task_urgency = task_urgency;
				table[free_index].specifics.task().task_race_countdown = task_urgency.inverse_value(); // or start with 0;
				handle_type free_handle = table[free_index].handle;
				macro_interrupt_critical_end;
				return free_handle;
			}
			
			/* creates a new timer entry.
			for the event_time, the scheduler only saves a pointer to the given time object.
			You have to provide a time object in its memory space yourself in the background.
			returns NO_HANDLE if table was already full, then table was not modified.
			otherwise: returns handle of created timer.
			If both, callable and void_function are non-null, the callable will be preferred */
			handle_type new_timer(volatile time::ExtendedMetricTime& time, fsl::str::callable* callable = nullptr, fsl::str::void_function function = nullptr, bool is_interrupting = false, bool enable = true){
				macro_interrupt_critical_begin;
				
				uint8_t free_index = new_table_line(false,is_interrupting);
				if (free_index == TABLE_SIZE){
					macro_interrupt_critical_end;
					return NO_HANDLE;
				}
				// some flags:
				if (enable) table[free_index].flags.set_true(ENTRY_ENABLED);
				
				// callback and callback flag:
				if (callable == nullptr){
					table[free_index].callback.set_function_ptr(function);
					table[free_index].flags.set_false(ENTRY_CALLABLE);
					} else {
					table[free_index].callback.set_callable_ptr(callable);
					table[free_index].flags.set_true(ENTRY_CALLABLE);
				}
				// timer specific information: timer event time
				table[free_index].specifics.timer().event_time = &time;
				if (is_interrupting) adjust_earliest_interrupting_timer_release(time);
				handle_type free_handle = table[free_index].handle;
				macro_interrupt_critical_end;
				return free_handle;
			}
			
			/* returns the handle of the first disabled valid entry in table (returns NO_HANDLE iff there is no such entry)
			writes the first [min(count@before, count@after)] handles with the outlined property (ENTRY_VALID and not ENTRY_ENABLE) into the [handle_array].
			[count@before] determines the array size provided by the calling instance.
			After function return [count@after] is the number of entries in table with the outlined property.
			*/
			handle_type get_disabled_entries(uint8_t& count, handle_type* handle_array = nullptr){
				if (handle_array == nullptr) count = 0;
				handle_type return_value = NO_HANDLE;
				macro_interrupt_critical(
				uint8_t counter{ 0 }; // next free array position
				for(uint8_t index = 0; index < TABLE_SIZE; ++index){
					if (table[index].flags.get(ENTRY_VALID) && !table[index].flags.get(ENTRY_ENABLED)){
						if (counter < count){
							handle_array[counter] = table[index].handle;
						}
						if (counter == 0){
							return_value = table[index].handle;
						}
						++counter;
					}
				}
				count = counter;
				);
				return return_value;
			}
			
			
			/*** static assertions ***/
			
			static_assert(sizeof(handle_type) == 1, "SchedulerHandle has not the appropriate size.");
			static_assert(sizeof(fsl::str::standard_union_callback) == 2, "UnionCallback has not the appropriate size.");
			static_assert(sizeof(union_specifics) == 2, "UnionSpecifics has not the appropriate size.");
			static_assert(sizeof(fsl::lg::single_flags) == 1, "fsl::lg::single_flags has not the appropriate size.");
			static_assert(sizeof(entry) == 6, "SchedulerMemoryLine has not the appropriate size.");
/*

			class table_entry_accessor : private fsl::hw::simple_atomic {
				inline table_entry_accessor(handle_type handle) : fsl::hw::simple_atomic() { }
				
				uint8_t table_index;
				public:
				static fsl::str::exceptional<table_entry_accessor, void> get_table_entry(handle_type handle){}
					// this is dangerous: if you use an union overlapping two classes. which of the destructors will be called?
					// the one will deal with SREG the other one might overwrite the sreg-copy.
					

				void set_callback(){
					
				}
			};

			class Task_Entry : Table_Entry {
				
				public:
				
				
			};

			class Timer_Entry : Table_Entry {
				
			};
			*/
		};
		
	}
}


#endif //__SCHEDULER_H__


/******** REFACTORING ISSUES **************/
/*


-2. Think about inlining.

-1. Add all reasons for including the include files.


-0.5 there are different strategies for task selection in method run(). just implement the other ones and compare or make it possible to decide by conditioned compilation.

-0. implement accessor classes to avoid the multiple duplications in functions like $598

--100.  try unordered tyble layout:
	// advantages:
	// ->	much work to find free handles
	//		use indices as handles, less memory consumption for table (1 byte per line)		<-
	// disadvantages:
	//	searching in method run() will take more time, I think								<-

*/


/************************************************************************/
/* ideas for new features                                                 

- int-timers may be executed with an int-call or with an non-int-call
  we may add an entry-wise flag INT-CALL-ONLY to deny non-int calls on int-timers
  (some programmers might use it if they want actions to be executed in certain time intervals
  and don't want such a procedure to come earlier than the next system_time_update. 
  
- task entries may get an additional flag that determines whether the task need some kind of finalization,
  if you want do shutdown the controller.
  So it might be possible to define such a flag and offer besides the stop() method of the scheduler another
  function shutdown() which first executes all tasks that have a positive fincalization flag once and stop scheduling right after.


- There is the possibility to find out whether a controller reset was caused by exceeding watchdog or not.
  (beim ATmega16 z. B. Bit WDRF in MCUCSR). Diese Information sollte auch genutzt werden, falls ein WD-Reset in der Anwendung nicht planmäßig implementiert wurde. Zum Beispiel kann man eine LED an einen freien Pin hängen, die nur bei einem Reset durch den WD aufleuchtet oder aber das "Ereignis WD-Reset" im internen EEPROM des AVR absichern, um die Information später z. B. über UART oder ein Display auszugeben (oder einfach den EEPROM-Inhalt über die ISP/JTAG-Schnittstelle auslesen).
  Bei neueren AVR-Typen bleibt der Watchdog auch nach einem Reset durch den Watchdog aktiviert. Wenn ein Programm nach dem Neustart bis zur erstmaligen Rückstellung des Watchdogs länger braucht, als die im Watchdog eingestellte Zeit, sollte man den Watchdog explizit möglichst früh deaktivieren. Ansonsten resetet der Watchdog den Controller immerfort von Neuem. Die frühe Deaktivierung sollte durch eine Funktion erfolgen, die noch vor allen anderen Operationen (insbesondere vor dem mglw. länger andauernden internen Initialisierungen vor dem Sprung zu main()) ausgeführt wird. Näheres zur Implementierung mit avr-gcc/avr-libc findet sich in der Dokumentation der avr-libc (Suchbegriffe: attribut, section, init).
  fsl::os sollte diesen wert auslesen, os loader sollte hw wd zuerst deaktivieren.

  */
/************************************************************************/


/************************************************************************/
/* testing issues:

check whether interrupt handling needs more time than the system_time precision!!!!
at bottom of function time_update_interrupt_handler.





                                                                     */
/************************************************************************/


/************************************************************************/
/* checking issues:

  there should be mo option to read a callback, and execute it
  outside the context of the scheduler since [my_handle] would not be correct
                                                                       */
/************************************************************************/