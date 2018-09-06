/*
* sch2.h
*
* Created: 20.03.2018 13:50:41
* Author: Maximilian Starke
*/
/* lib description: you must not activate interrupts during the execution of some interrupt handler. this will end up in undefined behavior since somemethods, e.g. execute_interrupting_timers() assume they're always running uninterrupted. */


#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>			// uint8_t
#include <avr/wdt.h>		// wdt_enable, wdt_disable, wdt_reset
#include <math.h>			// ceil

#include "f_callbacks.h"
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
		
		template <uint8_t TABLE_SIZE>
		class scheduler {
			
			
			public:		/*** public types ***/
			
			using handle_type = fsl::lg::range_int<uint8_t,TABLE_SIZE,true,true>;
			
			enum class entry_type : uint8_t {
				TASK = 0, TIMER = 1, INTTIMER = 2, INVALID = 255
			};
			
			
			public:		/*** public constexpr values ***/
			
			/* handle that refers to none of the entries in scheduler table */
			static constexpr handle_type NO_HANDLE{ handle_type::OUT_OF_RANGE };
			
			/* the default urgency for task entries in scheduler table */
			static constexpr urgency DEFAULT_URGENCY{ urgency::normal_urgency() };
			
			
			private:		/*** private types ***/
			
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
				};
				intern_union _union;
				public:
				task_specifics& task() { return _union.task; }
				timer_specifics& timer() { return _union.timer; }
			};
			
			/* class for one scheduler line containing everything needed for one timer / task entry */
			struct entry {
				handle_type handle;
				fsl::str::standard_union_callback callback;
				union_specifics specifics;
				fsl::lg::single_flags flags;
			};
			
			/*** private constexpr constants ***/
			
			/* flags for each scheduler line */
			static constexpr fsl::lg::single_flags::flag_id IS_VALID{ 0 }; // if an entry is not valid, the entry may be overwritten.
			static constexpr fsl::lg::single_flags::flag_id IS_ENABLED{ 1 }; // if an entry is valid, then the entry will be considered on scheduling decisions iff is_enabled
			// timer will automatically be disabled right before executing, if you want the timer to execute again after a certain time, you have to re-enable its entry from inside the timer procedure.
			// And you might want to update the event_time in normal cases. To do so you can use my_handle() to get your registration handle and pass it to suitable scheduler methods.
			static constexpr fsl::lg::single_flags::flag_id IS_TIMER{ 2 }; // if entry is valid, then entry (containing unions) is treated as a timer iff IS_Timer and as task otherwise.
			static constexpr fsl::lg::single_flags::flag_id IS_INTTIMER{ 3 }; // if entry is_valid and is_timer, then callback may be executed as soon as event_time is reached on system time interrupt iff IS_INTTIMER. Otherwise is has to wait until the current task/timer returned.
			static constexpr fsl::lg::single_flags::flag_id IS_CALLABLE{ 4 }; // if entry is_valid, the callback union will be treated as a callable iff IS_CALLABLE, otherwise as function pointer.

			/* flags once for every scheduler object */
			static constexpr fsl::lg::single_flags::flag_id IS_RUNNING{ 0 }; // true iff run() runs // write access only by run()
			static constexpr fsl::lg::single_flags::flag_id STOP_CALLED{ 1 }; // if true, run() should return when current task returns.
			// if it is true no int timers will be executed anymore until scheduler was started again.
			static constexpr fsl::lg::single_flags::flag_id SOFTWARE_INTERRUPTS_ENABLE{ 2 }; // scheduler will only look for int timers at now time update if enabled
			static constexpr fsl::lg::single_flags::flag_id EMPTY_TABLE_DETECTED{ 3 }; // for function-local internal use in run() only.
			
			/*** private data ***/
			
			/* the scheduler object's flags */
			volatile fsl::lg::single_flags flags;
			
			/* stack for handles of callbacks that were called*/
			//volatile fsl::con::stack<handle_type,2> my_handle_stack; delete
			
			volatile handle_type callback_handle;
			
			/* table containing all timers and tasks
			table layout definition:
			0				:	interrupting timers [no gap]	:	VALID
			:	non-interrupting timers			:	VALID
			:	[maybe gap]						:  INVALID
			:	tasks [no gap]					:	VALID
			TABLE_SIZE		:
			
			About the entries' flags:
			An entry is gap if and only if not IS_VALID.
			An entry with IS_VALID but not IS_ENABLED is not gap and has to be inside its section,
			although it will not be treated by the scheduler since is is so-called disabled.
			
			Access to table:
			You must always leave the scheduler table in a layout-definition conform and non corrupted state.
			For modifying you must put any operation inside an maro_interrupt_critical() - environment (atomic section).
			Of course reading the table won't disturb the table content anyway, but also reading should be done in atomic sections,
			since otherwise there is no particular guaranty for reading from a consistent table, since anyone could interrupt you and modify the table,
			even while you write one multi-byte datum.
			*/
			volatile entry table[TABLE_SIZE];

			// see: https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/Der_Watchdog
			/* timeout value for initialization of hardware watchdog
			must be greater than the time between two now time updates */
			uint8_t hardware_watchdog_timeout;
			
			/* contains a value proportional to the time which passes until the software watchdog "resets the controller",
			i.e. stops to reset the hardware watchdog with wdt_reset().
			exactly spoken, the value is the number of time_update_interrupt_handler() calls left until watchdog triggers controller reset.
			it is set to software_watchdog_reset_value on finishing of any task or timer procedure.
			it is decreased by 1 on every SysTime interrupt./  now_time update
			*/
			volatile uint32_t software_watchdog_countdown_value;

			/* the number of time_update_interrupt_handler() calls, that the software watchdog waits from fresh reset until trigger reboot */
			/* iff it is zero, the software watchdog is disabled. notice that hardware watchdog runs in background to observe behavior of scheduler */
			volatile uint32_t software_watchdog_reset_value;
			
			/* event time of the enabled interrupt timer with the smallest event time.
			on construction of scheduler it must be set to EMT::MIN()
			Whenever a new interrupt timers has been added or an existing one has been enabled, it must be updated to min(old, new_int_timer.event_time) */
			//volatile fsl::str::resettable<time::ExtendedMetricTime, 0> earliest_interrupting_timer_release;
			volatile time::ExtendedMetricTime earliest_interrupting_timer_release;

			/*** private methods ***/
			
			/* returns [mathematical meaning of] ((uint - 1) mod TABLE_SIZE) \in {0,1,2, ... ,TABLESIZE-1} */
			static constexpr uint8_t minus_one_mod_table_size(uint8_t uint){
				return static_cast<uint8_t>( (static_cast<uint16_t>(uint) + static_cast<uint16_t>(TABLE_SIZE -1)) % static_cast<uint16_t>(TABLE_SIZE) );
			}
			
			/* returns index of table where given handle can be found
			only call from inside an atomic section
			returns TABLE_SIZE iff there is no valid entry with given handle */
			uint8_t get_index(handle_type handle){
				uint16_t candidate = handle;
				for (uint8_t i = 0; i < (TABLE_SIZE + 1) /2; ++i){
					if ((table[(candidate + i) % TABLE_SIZE].handle == handle) && (table[(candidate + i) % TABLE_SIZE].flags.get(IS_VALID))) return (candidate + i) % TABLE_SIZE;
					if ((table[(candidate + TABLE_SIZE - i - 1) % TABLE_SIZE].handle == handle) && (table[(candidate + TABLE_SIZE - i - 1) % TABLE_SIZE].flags.get(IS_VALID))) return (candidate + TABLE_SIZE - i - 1) % TABLE_SIZE;
				}
				return TABLE_SIZE; // there is no such entry
			}
			
			/* reset the software watchdog. should be called after any completion of a task or timer */
			inline void software_watchdog_reset(){ macro_interrupt_critical(software_watchdog_countdown_value = software_watchdog_reset_value;); }
			
			/* executes a callback. if callback pointer is nullptr, nothing will be done */
			inline void execute_callback(fsl::str::standard_union_callback callback, bool is_callable){	(is_callable) ? callback.call_callable() : callback.call_function();	}
			
			/* executes callback of given table index.
			The callback will automatically be checked if it is nullptr, in this case nothing will be done
			Make sure that table is non-volatile during function call - You should own table inside an atomic section. */
			// <<< corruption: call will be made in an atomic region, but we want calls not to be there
			inline void execute_callback(uint8_t table_index){
				return execute_callback(table[table_index].callback,table[table_index].flags.get(IS_CALLABLE));
			}
			/* comment on read through print edit: execute_callback offers all functiobnality we need. 
			   opverload get_callback for table index too: then if someone wants this functionality he must use this funxction tpgether with execute_callback taking the callback copy. and between both function alls the atomic must be closed. 
			   mark this function as deprecated and comment out.
			   */
			
			/* goes through all (valid) int timer entries and executes the callback if execution_time expired and entry is enabled.
			Method must not be interrupted during its execution. It does not explicitly turns off interrupts since it should only be called as interrupt subroutine.
			So it assumes to always run uninterrupted
			returns	false:	earliest_time was not reached, no iteration through table, no callback executed
			returns true:	algorithm went through table, probably executing at least one callback, and earliest_interrupting_timer_release was updated.
			*/
			inline bool execute_interrupting_timers(){
				time::ExtendedMetricTime new_earliest_interrupting_timer_release = time::ExtendedMetricTime::MAX();
				time::ExtendedMetricTime now = fsl::os::system_time::get_instance()();
				
				if (now < earliest_interrupting_timer_release)		return false;
				
				/* @when here: table has a valid state, because we are the interrupt, no one other can interrupt us (configuration assumption)
				therefore we don't need an own atomic region. */
				// <<<< maybe just to be sure we could do it in spite of our assumption
				
				uint8_t index = 0;
				
				while(index < TABLE_SIZE){
					if ((table[index].flags.get(IS_VALID) == false) || (table[index].flags.get(IS_TIMER) == false) || (table[index].flags.get(IS_INTTIMER) == false)){
						break;
					}
					// @when here: at [index] we see a valid entry that is of type int-timer.
					if (table[index].flags.get(IS_ENABLED)){
						if (now >= *table[index].specifics.timer().event_time){
							table[index].flags.set_false(IS_ENABLED);
							handle_type local_stack = callback_handle;
							callback_handle = table[index].handle;
							execute_callback(index);
							callback_handle = local_stack;
							//	index = 0;	continue; // start from begin again again, because table might be modified by callback procedure.
							return true; // prevention against some infinite loop of executing int timers. Next timer should wait for next now_time_update.
							// <<< one of those two strategies must be chosen.
							} else {
							new_earliest_interrupting_timer_release =
							new_earliest_interrupting_timer_release > *table[index].specifics.timer().event_time ? // if found smaller one
							*table[index].specifics.timer().event_time : // replace
							new_earliest_interrupting_timer_release; // do nothing
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
				return table[index].flags.get(IS_TIMER) ? (table[index].flags.get(IS_INTTIMER) ? entry_type::INTTIMER : entry_type::TIMER) : entry_type::TASK;
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
					if (table[index].flags.get(IS_VALID) == false) goto found_invalid_entry;
				}
				/*@when here: The table is already full */
				return TABLE_SIZE;
				
				found_invalid_entry:
				uint8_t free_index{ index };
				/*@when here: The table has a free line, we stored that index as free_index,
				there must be also a free handle since there is at least one free line in the table */
				handle_type free_handle{ NO_HANDLE };
				
				// search for free handle
				constexpr uint8_t c_candidates {3};
				handle_type candidates[c_candidates] = {index, table[index].handle, index};
				// first  idea: prefer handle == index in table, task index may grow up, timer index my become smaller during handler life, when task with greater / timer with smaller index is deleted
				// second idea: handle = handle of last entry that was written here. // because this handle is possibly free since the entry was marked disabled
				// third  idea: same as first idea, but different update strategy.
				uint8_t check_until_index[c_candidates] = {index, index, index};
				
				while (true){
					index = (index + 1) % TABLE_SIZE;
					// check that there is no crash between candidate and the current line
					if (table[index].flags.get(IS_VALID)){ // if current line is valid, any candidate must not equal the current line's handle.
						for(uint8_t i = 0; i < c_candidates; ++i){
							if (table[index].handle == candidates[i]){
								// candidate is already used at current index, use different update strategies for candidates:
								if (i == 0){
									// idea: go through all possible handles by moving --
									candidates[i] = minus_one_mod_table_size(candidates[i]);
									} else if (i == 1){
									// idea: if we wanted an handle that equals it's entry's index but we found this forbidder here, try the forbidders index
									candidates[i] = index;
									} else {
									//idea: just go ++, opposite direction compared to case (i == 0)
									candidates[i] = (candidates[i] + 1) % TABLE_SIZE;
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
							goto found_free_handle1; // <<<< rename this goto label
						}
					}
				}
				
				found_free_handle1:
				/* @when here: free_index is an index of an unused line, free_handle is an unused handle.
				if we want to create an interrupting timer we need to swap position with first non_int timer
				if we want to create a task or a non-int timer, then free_index is already in the right table section.
				*/
				if ((!is_task) && (is_interrupting)){ // we need to swap a new int timer line to the section of int timers!
					// Check whether predecessors of our int-timer are non-int timers.
					for(index = free_index; true; --index ){ // check about a swap of index and free_index
						if ((index == 0) || (table[index-1].flags.get(IS_INTTIMER))) break; // swap with current index
					}
					// we need to swap:
					// move from index to free_index
					
					// if (free_index != index):
					table[free_index] = table[index];
					free_index = index;
				}
				table[free_index].handle = free_handle;
				table[free_index].flags.set_true(IS_VALID);
				table[free_index].flags.set(IS_TIMER, ! is_task);
				table[free_index].flags.set(IS_INTTIMER, is_interrupting);
				table[free_index].flags.set_false(IS_ENABLED);
				return free_index;
			}
			
			/* activate hardware watchdog and set the timeout to hardware_watchdog_timeout */
			inline void activate_hardware_watchdog(){
				macro_interrupt_critical(
				wdt_enable(hardware_watchdog_timeout);
				wdt_reset();
				);
			}
			
			/* deactivate hw watchdog */
			inline void deactivate_hardware_watchdog(){
				macro_interrupt_critical(
				wdt_disable();
				);
			}
			
			public:
			
			/*** public constructor ***/
			
			inline scheduler(bool software_interrupt_enable, uint8_t hardware_watchdog_timeout, const time::ExtendedMetricTime& watchdog) : 
				flags(0), callback_handle(NO_HANDLE),  hardware_watchdog_timeout(hardware_watchdog_timeout), earliest_interrupting_timer_release(time::EMT::MIN()){
				clear_table();
				activate_software_watchdog(watchdog);
				}
						
			/*** public methods ***/
			
			/* activate software interrupts / activate interrupting timers */
			void enable_software_interrupts(){ macro_interrupt_critical( flags.set_true(SOFTWARE_INTERRUPTS_ENABLE);); }

			/* deactivate software interrupts / deactivate interrupting timers */
			void disable_software_interrupts(){ macro_interrupt_critical( flags.set_false(SOFTWARE_INTERRUPTS_ENABLE);); }
			
			inline const handle_type& my_handle(){ return callback_handle; }
			
			inline bool is_running(){ return flags.get(IS_RUNNING); }
			
			inline bool was_stop_called(){ return flags.get(STOP_CALLED); }
			
			/* get entry type of given handle */
			inline entry_type get_entry_type(handle_type handle){
				entry_type result;
				macro_interrupt_critical( result = get_entry_type_unsave(handle); );
				return result;
			}
			
			/* set task_race_countdown to 0. The task will be on top of the tasks with low countdown.
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
					if (table[index].flags.get(IS_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().task_race_countdown = 0;
				);
				return error_code;
			}
			
			/* set task_urgency_inverse. This value must not be 0. 0 will replaced by 1.
			returns		0:	done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a timer / int-timer entry
			*/
			uint8_t set_task_urgency_inverse(handle_type handle, uint8_t urgency_inverse){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(IS_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().urgency_inverse = urgency_inverse + !(urgency_inverse);
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
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(IS_TIMER)) error_code = 2;
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
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(IS_TIMER)) error_code = 2;
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
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code){
					if (table[index].flags.get(IS_TIMER)) error_code = 2;
				}
				if (!error_code) table[index].specifics.task().decrease_urgency();
				);
				return error_code;
			}
			
			//##effizientere get_:index suche nach handle implementieren.
			
			/* Set callback of entry associated with given handle.
			returns 0 if successful, returns 1 if handle does not meet a valid entry. */
			inline void set_callback(handle_type handle, decltype(nullptr) callback){	set_callback(handle,fsl::str::void_function(callback));	}
			
			inline void set_callback(handle_type handle, fsl::str::void_function callback){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].callback.set_function_ptr(callback);
				macro_interrupt_critical_end;
				return 0;
			}
			
			inline void set_callback(handle_type handle, fsl::str::callable* callback){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].callback.set_callable_ptr(callback);
				macro_interrupt_critical_end;
				return 0;
			}
			
			inline void reset_callback(handle_type handle){ return set_callback(handle,nullptr); }
			
#if false
			/* Write callback pointer into [callback], returns the callback_type written to callback. */
			// <<< mark the void_ptr() of callback class as deprecated and this function too.
			// <<< maybe delete this implementation:
			inline callback_type get_callback(handle_type handle, void*& callback){
				callback_type result{ callback_type::INVALID };
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					callback = nullptr;
					return callback_type::INVALID;
				}
				result = table[index].flags.get(IS_CALLABLE) ? callback_type::CALLABLE : callback_type::VOID_FUNCTION;
				callback = table[index].callback.void_ptr();
				macro_interrupt_critical_end;
				return result;
			}
#endif
			
			// <<< better version of this function:
			// <<<< remove callback_type
			inline void get_callback(handle_type handle, fsl::str::standard_union_callback& callback, bool& is_callable){
				callback.set_function_ptr(nullptr);
				is_callable = false;
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index != TABLE_SIZE) {
					is_callable = table[index].flags.get(IS_CALLABLE);
					callback = table[index].callback;
				}
				);
			}
			
			/* execute callback of given handle, if handle is valid and callback is nun-nullptr */
			inline void execute_callback(handle_type handle){
				fsl::str::standard_union_callback callback;
				bool is_callable;
				get_callback(handle,callback,is_callable);
				execute_callback(callback,is_callable);
			}
			
			inline uint8_t clear_entry(handle_type handle){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].flags.set_false(IS_VALID);
				macro_interrupt_critical_end;
				return 0;
			}
			
			inline uint8_t get_entry_enable(handle_type handle){
				uint8_t result{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				result = (index == TABLE_SIZE) ? 255 : table[index].flags.get(IS_ENABLED);
				);
				return result;
			}
			
			inline uint8_t set_entry_enable(handle_type handle, bool enable){
				macro_interrupt_critical_begin;
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) {
					macro_interrupt_critical_end;
					return 1;
				}
				table[index].flags.set(IS_ENABLED,enable);
				macro_interrupt_critical_end;
				return 0;
			}
			
			inline uint8_t enable_entry(handle_type handle){ return set_entry_enable(handle,true); }
			
			inline uint8_t disable_entry(handle_type handle){ return set_entry_enable(handle,false); }
			
			uint8_t count_int_timers(){
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (table[pivot].flags.get(IS_VALID) && table[pivot].flags.get(IS_TIMER) && table[pivot].flags.get(IS_INTTIMER)){
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return lower_bound;
			}
			
			uint8_t count_all_timers(){
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (table[pivot].flags.get(IS_VALID) && table[pivot].flags.get(IS_TIMER)){
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return lower_bound;
			}
			
			inline uint8_t count_non_int_timers(){	return count_all_timers()-count_int_timers();	}

			uint8_t count_tasks(){
				uint8_t lower_bound{ 0 };
				uint8_t upper_bound{ TABLE_SIZE };
				while (lower_bound != upper_bound){
					const uint8_t pivot = (static_cast<uint16_t>(lower_bound) + upper_bound) / 2;
					if (!(table[pivot].flags.get(IS_VALID) && (!table[pivot].flags.get(IS_TIMER)))){ /* not a valid task */
						lower_bound = pivot + 1;
						} else {
						upper_bound = pivot;
					}
				}
				return TABLE_SIZE - lower_bound;
			}
			
			inline uint8_t count_gaps(){ return TABLE_SIZE - count_all_timers() - count_tasks(); }
			
			inline bool table_full(){ return count_gaps() == 0; }
			inline bool table_empty(){ return count_gaps() == TABLE_SIZE; }
			
			/* convert timer to int-timer.
			returns		0:	reset done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a task entry
			4:	handle is already listed as int-timer. no conversion necessary
			*/
			uint8_t convert_to_int_timer(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(IS_TIMER)) error_code = 2;
				if (!error_code)	if (table[index].flags.get(IS_INTTIMER)) error_code = 4;
				if (!error_code){
					const uint8_t swap { count_int_timers() };
					byte_swap(table[index],table[swap]);
					table[swap].flags.set_true(IS_INTTIMER);
				}
				);
				return error_code;
			}
			
			/* convert timer to non-int-timer.
			returns		0:	reset done without any errors
			1:	invalid handle, nothing done
			2:	handle is associated with a task entry
			4:	handle is already listed as normal timer. no conversion necessary
			*/
			uint8_t convert_to_normal_timer(handle_type handle){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(IS_TIMER)) error_code = 2;
				if (!error_code)	if (!table[index].flags.get(IS_INTTIMER)) error_code = 4;
				if (!error_code){
					const uint8_t swap { count_int_timers() - 1 };
					byte_swap(table[index],table[swap]);
					table[swap].flags.set_false(IS_INTTIMER);
				}
				);
				return error_code;
			}
			
			inline void set_event_time_object(handle_type handle, volatile time::ExtendedMetricTime& time_object){
				uint8_t error_code{ 0 };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index == TABLE_SIZE) error_code = 1;
				if (!error_code)	if (!table[index].flags.get(IS_TIMER)) error_code = 2;
				if (!error_code)	table[index].specifics.timer().event_time = &time_object;
				);
				return error_code;
			}
			
			// if you change time to earlier, you should update earliest int timer time, if it is an inttimer.!!!
			// better return only as const
			inline volatile time::ExtendedMetricTime* get_event_time(handle_type handle){
				time::ExtendedMetricTime* result{ nullptr };
				macro_interrupt_critical(
				uint8_t index = get_index(handle);
				if (index != TABLE_SIZE) if (!table[index].flags.get(IS_TIMER)) result = table[index].specifics.timer().event_time;
				);
				return result;
			}

			inline bool set_event_time(handle_type handle, time::ExtendedMetricTime& t){
				volatile time::ExtendedMetricTime* event_time = get_event_time(handle);
				if (!event_time) return false;
				macro_interrupt_critical(
				if (t < *event_time) reset_int_timer_prefetcher();
				*event_time = t;
				);
				return true;
			}
			
			inline void reset_int_timer_prefetcher(){ earliest_interrupting_timer_release = time::ExtendedMetricTime::MIN(); }
			
			inline bool is_task(handle_type handle){ return get_entry_type(handle) == entry_type::TASK; }
			
			inline bool is_non_int_timer(handle_type handle){ return get_entry_type(handle) == entry_type::TIMER; }
			
			inline bool is_int_timer(handle_type handle){ return get_entry_type(handle) == entry_type::INTTIMER; }
			
			inline bool is_any_timer(handle_type handle){ return is_non_int_timer() || is_int_timer(); }
			
			/* invalidate all table entries */
			void clear_table(){	macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) table[i].flags.set_false(IS_VALID);); }
			
			/* invalidate all task entries */
			inline void clear_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(IS_TIMER)) table[i].flags.set_false(IS_VALID););}
			
			/* invalidate all non-int-timer and all int-timer entries */
			inline void clear_all_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(IS_TIMER)) table[i].flags.set_false(IS_VALID););}

			/* invalidate all int-timer entries */
			inline void clear_int_timers(){
				macro_interrupt_critical(
				uint8_t count_int_timer{ 0 };
				uint8_t count_timer{ 0 };
				for(uint8_t i = 0; i < TABLE_SIZE; ++i){
					if (table[i].flags.get(IS_TIMER)){
						if (table[i].flags.get(IS_INTTIMER)) ++count_int_timer; else ++count_timer;
					}
				}
				// reorder table and overwrite / delete int_timers :
				const uint8_t move_size{ fsl::lg::min(count_int_timer, count_timer) };
				const uint8_t move_src_begin{ count_int_timer + count_timer - move_size };
				for(uint8_t i = 0; i < move_size; ++i){
					const uint8_t src{ move_src_begin + i };
					table[i] = table[src];
					table[src].flags.set_false(IS_VALID);
				}
				);
			}

			/* invalidate all non-int-timer entries */
			inline void clear_non_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(IS_TIMER) && (!table[i].flags.get(IS_INTTIMER))) table[i].flags.set_false(IS_VALID););}
			
			/* disable all table entries */
			void disable_table(){	macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) table[i].flags.set_false(IS_ENABLED);); }
			
			/* disable all task entries */
			inline void disable_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(IS_TIMER)) table[i].flags.set_false(IS_ENABLED););}
			
			/* disable all non-int-timer and all int-timer entries */
			inline void disable_all_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(IS_TIMER)) table[i].flags.set_false(IS_ENABLED););}

			/* disable all int-timer entries */
			inline void disable_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(IS_TIMER) && table[i].flags.get(IS_INTTIMER)) table[i].flags.set_false(IS_ENABLED););}

			/* disable all non-int-timer entries */
			inline void disable_non_int_timers(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (table[i].flags.get(IS_TIMER) && (!table[i].flags.get(IS_INTTIMER))) table[i].flags.set_false(IS_ENABLED););}
			
			/* enable all task entries */
			inline void enable_tasks(){ macro_interrupt_critical(for(uint8_t i = 0; i < TABLE_SIZE; ++i) if (!table[i].flags.get(IS_TIMER)) table[i].flags.set_true(IS_ENABLED););}
			
			/* executes all stuff that should be done on now_time update, including hardware watchdog reset, int-timer execution e.a.
			should be called once on every update of the now time and only called by interrupt, children function assume interrupt-freedom */
			inline void time_update_interrupt_handler(){
				if (flags.get(IS_RUNNING)){
					if ((software_watchdog_reset_value == 0) /*software watchdog disabled*/ || (software_watchdog_countdown_value > 0) /*software watchdog not exceeded*/)
					{
						--software_watchdog_countdown_value;
					}
					wdt_reset();
					if (flags.get(SOFTWARE_INTERRUPTS_ENABLE) && (flags.get(STOP_CALLED) == false)) execute_interrupting_timers();
				}
				// ### here would be the right position to check whether interrupt handling needs more time than the system_time precision!!!!
			}
			
			/* returns size in bytes of one SchedulerMemoryLine */
			static constexpr uint8_t table_line_size_of(){ return sizeof(entry); }
			
			/* returns size in bytes of the whole scheduler table */
			static constexpr uint16_t table_size_of(){ return sizeof(entry)*TABLE_SIZE; }
			
			/* set value of timeout of hardware, use predefined macros WDTO_15MS, WDTO_30MS ... */
			inline void set_hardware_watchdog_timeout(uint8_t value){
				hardware_watchdog_timeout = value; //###### do this
			}
			
			/* deactivates software watchdog
			note that hardware watchdog is still in use and reboots controller if
			* you use too long interrupt timer handlers or
			* you use atomic sections that are far too long
			* if the Scheduler itself is corrupt and deactivates interrupts for too long
			*/ // <<<<< these arealso general hint that should appear at user manual for this header on top of file!!!!
			inline void deactivate_software_watchdog(){		macro_interrupt_critical(software_watchdog_reset_value = 0;);	}
			
			/* activate the software watchdog for running scheduler.
			software watchdog will be startet when you call run() and stopped before run() returns,
			but may be enabled before you call run() and really should be if you want to have such a software watchdog.
			Set the template parameters this way you want the time to be rounded.
			If any time <= 0 is given the software watchdog will be disabled.
			*/
			template <bool round_to_ceiling = false, bool truncate = false, bool truncate_and_add_one = true>
			inline void activate_software_watchdog(const time::ExtendedMetricTime& watchdog){
				static_assert(0 + round_to_ceiling + truncate + truncate_and_add_one == 1, "There must be one template argument being true and two being false");
				if (watchdog <= 0) return deactivate_software_watchdog();
				macro_interrupt_critical(
				if (round_to_ceiling){ // constexpr
					software_watchdog_reset_value =
					static_cast<decltype(software_watchdog_reset_value)>(
					ceil(
					watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
					)
					);
				}
				if (truncate){ // constexpr
					software_watchdog_reset_value = static_cast<decltype(software_watchdog_reset_value)>
					(
					watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
					);
				}
				if (truncate_and_add_one){ // constexpr
					software_watchdog_reset_value = static_cast<decltype(software_watchdog_reset_value)>
					(
					watchdog.get_in_seconds() * (fsl::os::system_time::get_instance().precision())
					)
					+ 1;
				}
				software_watchdog_countdown_value = software_watchdog_reset_value; // create own private method for that!!!s
				); // macro_interrupt_critical
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
				flags.set_false(STOP_CALLED); // delete previous stop command
				flags.set_true(IS_RUNNING); // set running flag
				flags.set_false(EMPTY_TABLE_DETECTED); // check for empty table
				activate_hardware_watchdog();
				software_watchdog_reset();
				
				central_control_loop:
				while(true){
					
					software_watchdog_reset();
					uint8_t choice; // index inside
					macro_interrupt_critical_begin;
					
					if (flags.get(STOP_CALLED)){
						// @when here: while loop must terminated because of a stop() call:
						deactivate_hardware_watchdog();// wdt_disable(); // stop watchdog
						flags.set_false(IS_RUNNING); // delete running flag
						macro_interrupt_critical_end;
						return 1;
					}
					
					/*** look for a timer that can be executed ***/
					
					time::ExtendedMetricTime now = fsl::os::system_time::get_instance()();
					for(choice = 0; (choice < TABLE_SIZE) && (! ((table[choice].flags.get(IS_VALID) == false) || (table[choice].flags.get(IS_TIMER) == false)) ); ++choice){
						// at current index we see a valid timer (int or non-int)
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
					/*** end of looking for timer ***/
					
					// @when here: table may have or may not have timers or may consist only of timers inside, but no of the possibly present timers in table expired
					// EMPTY_TABLE_DETECTED was set to false when looking through timers if there was at least one enabled timer
					
					if (flags.get(EMPTY_TABLE_DETECTED)){ // table empty, nothing to schedule anymore
						deactivate_hardware_watchdog();
						flags.set_false(IS_RUNNING);
						macro_interrupt_critical_end;
						return 0;
					}
					
					/*** look for a task that can be executed ***/
					{
						uint8_t min_entry_index{ TABLE_SIZE }; // means no entry
						uint16_t min_entry_race_countdown{ 0xFFFF }; // means no entry
						
						/** look for a task with task_race_countdown == 0 **/
						for (choice = TABLE_SIZE - 1; (choice < TABLE_SIZE) && (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false); --choice){
							// choice is an index where we see a valid task entry
							if (table[choice].flags.get(IS_ENABLED)){
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
							flags.set_true(EMPTY_TABLE_DETECTED);
							macro_interrupt_critical_end;
							goto central_control_loop; // the task section is empty at the moment, run scheduler from begin.
						}
						
						// @when here: reached end of non-empty task section (at least one task is valid and enabled), but no task_race_countdown was zero
						
						/** subtract minimum task_race_countdown from all count_downs in task section and let the min-countdown task be executed afterwards **/
						for (choice = TABLE_SIZE - 1; (choice < TABLE_SIZE) && (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false); --choice){
							if (table[choice].flags.get(IS_ENABLED)){
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
					choice = table[choice].flags.get(IS_CALLABLE);
					macro_interrupt_critical_end;
					
					execute_callback(callback,choice);
					callback_handle = local_stack;
				}
			}

			/* make scheduler return after current executed task / timer */
			inline void stop(){		macro_interrupt_critical(flags.set_true(STOP_CALLED););		}

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
				table[free_index].flags.set_false(IS_ENABLED);
				if (enable) table[free_index].flags.set_true(IS_ENABLED);
				// callback and callback flag:
				if (callable == nullptr){
					table[free_index].callback.set_function_ptr(function);
					table[free_index].flags.set_false(IS_CALLABLE);
					} else {
					table[free_index].callback.set_callable_ptr(callable);
					table[free_index].flags.set_true(IS_CALLABLE);
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
				if (free_index = TABLE_SIZE){
					macro_interrupt_critical_end;
					return NO_HANDLE;
				}
				// some flags:
				table[free_index].flags.set(IS_ENABLED,enable);
				
				// callback and callback flag:
				if (callable == nullptr){
					table[free_index].callback.set_function_ptr(function);
					table[free_index].flags.set_false(IS_CALLABLE);
					} else {
					table[free_index].callback.set_callable_ptr(callable);
					table[free_index].flags.set_true(IS_CALLABLE);
				}
				// timer specific information: timer event time
				table[free_index].specifics.timer().event_time = &time;
				if (is_interrupting) earliest_interrupting_timer_release = earliest_interrupting_timer_release < time ? earliest_interrupting_timer_release : time;
				handle_type free_handle = table[free_index].handle;
				macro_interrupt_critical_end;
				return free_handle;
			}
			
			/* returns the handle of the first disabled valid entry in table (returns NO_HANDLE iff there is no such entry)
			writes the first [count] handles with the outlined property into the [handle_array] (if there are enough of those).
			[count] determines the array size provided by the calling instance.
			After function return [count] is the number of entries in table with the outlined property.
			Writes exactly max(count@before, count@after) handles into [handle_array]. */
			handle_type get_disabled_entries(uint8_t& count, handle_type* handle_array = nullptr){
				if (handle_array == nullptr) count = 0;
				handle_type return_value = NO_HANDLE;
				macro_interrupt_critical(
				uint8_t counter{ 0 }; // next free array position
				for(uint8_t index = 0; index < TABLE_SIZE; ++index){
					if (table[index].flags.get(IS_VALID) && !table[index].flags.get(IS_ENABLED)){
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


			class table_entry_accessor : private fsl::hw::simple_atomic {
				inline table_entry_accessor(handle_type handle) : fsl::hw::simple_atomic() { }
				
				uint8_t table_index;
				public:
				static fsl::str::exceptional<table_entry_accessor, void> get_table_entry(handle_type handle){}

				void set_callback(){
					
				}
			};
/*
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

*/





////////////////////////////////////////////////////////////////////////// conceptional:

/*
//static scheduler2<TABLE_SIZE>* instance; /// here is a big problem: our class is template. // someone (interrupt by now time update) cannot know which function to call
// should be no problem if behaviour is like: each template instanciation has its own static instance pointer.
// this is the case that every tempolate instanciation has its own static member, you have to extern define these members, otherwise linkage errors will come up.
// user has to write an own function that is called by systime now update and only propergates i.e. calls MY_SCHEDULER::interrupt handler(); where
// MY_SCHEDULER has to be defined with using ~ = scheduler2<20>; or similar

// i think it is better to do not have this static member. user must provide one self-written function as callback handler which calls instance->time_update_interrupt_handler()
// über den stack nachdenken der my_handle bereit hält:
am besten sollte der stack nicht existieren, da man ja auch manuell callbacks aufrufen kann.
stattdessen sollte der my_handle-stack in den function call stack integriert werden,
d.h. wann immer wir ein callback nach handle ausführen, dann sollte my_handle auf das assozierte handle des callbacks gesetzt werden, der alte wert soll zwischengespeichert und nach dem return zurückgesetzt werden.
das setzten und rücksetzen des callbacks muss evetuell atomar gemacht werden, falls während dieses vorgangs ein interrupt kommt.

*/////
/*
ideas for further improvements

always save a copy of earliest time a int timer has to be executed: This way it is no longer necessary to go through whole table on each timer interrupt


// replace hd watchdog calls in run() by calls to software watchdog. ####
hw watchdog timeout must be set in c-tor


viel später::: nachdem diese lib fertig ist:
Es besteht die Möglichkeit herauszufinden, ob ein Reset durch den Watchdog ausgelöst wurde (beim ATmega16 z. B. Bit WDRF in MCUCSR). Diese Information sollte auch genutzt werden, falls ein WD-Reset in der Anwendung nicht planmäßig implementiert wurde. Zum Beispiel kann man eine LED an einen freien Pin hängen, die nur bei einem Reset durch den WD aufleuchtet oder aber das "Ereignis WD-Reset" im internen EEPROM des AVR absichern, um die Information später z. B. über UART oder ein Display auszugeben (oder einfach den EEPROM-Inhalt über die ISP/JTAG-Schnittstelle auslesen).

Bei neueren AVR-Typen bleibt der Watchdog auch nach einem Reset durch den Watchdog aktiviert. Wenn ein Programm nach dem Neustart bis zur erstmaligen Rückstellung des Watchdogs länger braucht, als die im Watchdog eingestellte Zeit, sollte man den Watchdog explizit möglichst früh deaktivieren. Ansonsten resetet der Watchdog den Controller immerfort von Neuem. Die frühe Deaktivierung sollte durch eine Funktion erfolgen, die noch vor allen anderen Operationen (insbesondere vor dem mglw. länger andauernden internen Initialisierungen vor dem Sprung zu main()) ausgeführt wird. Näheres zur Implementierung mit avr-gcc/avr-libc findet sich in der Dokumentation der avr-libc (Suchbegriffe: attribut, section, init).

-> to do: fsl::os soll den wert auslesen können.
-> os loader soll als erstes einen evtl ajktivierten watchdog deaktivieren.







/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
// <<<<<< furthermore finding free handlers will become much easier and faster
// <<<<<< searching in method run() will last a little bit longer, I think,


*/

/*
task selection in run() {
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
nstead of "re baseing" we could also use an extra variable as last minimum, new minimum will be lowest number >= last min.

// just implement one strategy and later implement the others and make it possible to choose by conditioned compiling.
}

//// ###think about volatile stuff in this class

*/
