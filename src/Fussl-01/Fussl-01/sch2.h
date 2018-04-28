/*
* sch2.h
*
* Created: 20.03.2018 13:50:41
* Author: Maximilian Starke
*/


#ifndef __SCH2_H__
#define __SCH2_H__

#include <stdint.h>
#include <avr/wdt.h>

#include "f_macros.h"
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
		uint8_t urgency_inverse; // Dringlichkeit
		/* 255 .. least important ... 0 ... most important */
		uint8_t task_race_countdown;
		// always the task with the smallest task_race_countdown will be executed. If this is ambigious one of them will be executed.
		// after a task is executed task_race_countdown will be increased by urgency_inverse.
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
	
	/* flags for each scheduler line */
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
	// if it is true no int timers will be executed anymore until scheduler was started again.
	static constexpr concepts::Flags::flag_id TABLE_LOCKED{ 2 }; // if scheduler is accessing scheduling table, it must not be modified by interrupt timers
	static constexpr concepts::Flags::flag_id EMPTY_TABLE_DETECTED{ 3 }; // for function-local internal use in run() only.
	static constexpr concepts::Flags::flag_id SOFTWARE_INTERRUPTS_ENABLE{ 4 }; // scheduler will only look for int timers at now time update if enabled

	
	/* private data */
	
	/* table layout definition:

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
	an entry, that is VALID but DISABLED, is not gap and has to be inside its section.
	
	*/
	SchedulerMemoryLine table[TABLE_SIZE]; // the timer and task table

	uint8_t hardware_watchdog_timeout; // value for hardware watchdog // must be greater than the time betweeen two now time updates.

	uint32_t software_watchdog_countdown_value;
	// it is set to watchdog_reset_value on finishing of any task or timer procedure.
	// it is decreased by 1 on every SysTime interrupt.

	uint32_t software_watchdog_reset_value; // watchdog_countdown_value is reset to this value every time a timer or task procedure returns
	// contains indirectly a time distance, which stands for the time the software watchdog has to wait before rebooting
	
	// scheduler::SysTime& system_time; not needed because there is a static get_instance() in f_systime.h
	
	/*** private methods ***/
	
	inline void execute_callback(uint8_t table_index){
		if (table[table_index].flags.get(IS_CALLABLE)){
			table[table_index].callback.call_callable();
			} else {
			table[table_index].callback.call_function();
		}
	}
	
	inline void execute_interrupting_timers(){
		time::ExtendedMetricTime now = scheduler::SysTime::get_instance()(); // = system_time() // .get_now_time();
		for(uint8_t index = 0; (index < TABLE_SIZE); ++index){ // check at all other positions where we have merged the loop condition, that we do not access before checking out of range like in next line: #####
			if ((table[index].flags.get(IS_VALID) == false) || (table[index].flags.get(IS_TIMER) == false) || (table[index].flags.get(IS_INTTIMER) == false))
			break; // we reached the end of the interrupting timer section.
			if (table[index].flags.get(IS_VALID) && (now >=  *table[index].specifics.timer().event_time)){
				table[index].flags.set_false(IS_ENABLED);
				return execute_callback(index);
			}
		}
	}

	inline void non_static_interrupt_handler(){
		if ((software_watchdog_reset_value == 0) || (software_watchdog_countdown_value > 0)) {
			--software_watchdog_countdown_value;
			wdt_reset();
			// replace hd watchdog calls in run() by calls to software watchdog. ####
		}
		if (flags.get(SOFTWARE_INTERRUPTS_ENABLE) && (flags.get(STOP_CALLED) == false)) return execute_interrupting_timers();
	}
	
	public:
	
	static scheduler2<TABLE_SIZE>* instance; /// here is a big problem: our class is template. // someone (interrupt by now time update) cannot know which function to call
	// should be no problem if behaviour is like: each template instanciation has its own static instance pointer.
	
	
	static constexpr uint8_t table_line_size(){ return sizeof(SchedulerMemoryLine); }


	
	static constexpr time::ExtendedMetricTime NO_WATCHDOG_EMT{0};
	// a time for watchdog is required on initialization. but if you don't want a watchdog,
	// just set it to 0 (NO_WATCHDOG).
	
	static constexpr int32_t NO_WATCHDOG_COUNTER_FORMAT{0}; // calc by a constexpr function for time-wtchdog -> counter-watchdog

	void set_watchdog(const time::ExtendedMetricTime& watchdog){ /*scheduler::watchdog = 0; watchdog; (zeit pro systime tick ) */ }
	
	/*
	start engine:
	reset hardware watchdog.
	activate hw watchdog.
	set software watchdog countdown to watchdog reset value
	
	give system_time the pointer to our interrupt handler.
	make sure that our static instance pointer is up to date.
	
	
	
	just reset the now_update_handler of system_time to nullptr.
	// -> we wont be called from the systemclock again.
	
	stop engine:
	deactivate hardware watchdog
	
	*/
	//template <uint8_t TABLE_SIZEP>
	static void interrupt_handler(){
		/*scheduler2<TABLE_SIZEP>::*/ instance->non_static_interrupt_handler();
	}
	// called by SysTime, every time an TC Compare Match Interrupt occurs
	// check for interrupting functions in schedule table
	
	
	// central control loop. called after init and provides the tasks
	uint8_t run();

	/* make scheduler return after current executed task / timer */
	inline void stop(){
		flags.set_true(STOP_CALLED);
	}
	
	// <<< think about behaviour of deleting timer entries after they were executed
	
	/*
	try to add new task to table.
	returns SchedulerHandle of created task
	if not successful NO_HANDLE is returned.
	*/

	static constexpr uint8_t minus_one_mod_table_size(uint8_t uint){ // returns (uint - 1) mod TABLE_SIZE
		return static_cast<uint8_t>( (static_cast<uint16_t>(uint) + static_cast<uint16_t>(TABLE_SIZE -1)) % static_cast<uint16_t>(TABLE_SIZE) );
	}
	
	SchedulerHandle new_task(uint8_t urgency = STANDARD_URGENCY, concepts::Callable* callable = nullptr, concepts::void_function function = nullptr, bool enable = true);
	
	
	inline bool is_table_locked(){ return flags.get(TABLE_LOCKED); }
	
	
	/*inline bool is_table_full(){ return true; } */
	
	/* prepares an unused (= IS_INVALID) line to add a new task or timer there */
	/* returns TABLE_SIZE if the table is either locked or full. In this case the table is not changed */
	/* you can determine whether "table locked" was a reason of abortion by checking this->flags.get(TABLE_LOCKED) */
	/* otherwise returns the index of the line which was prepared.
	   The prepared line comes with a free handle stored inside this line. */
	uint8_t new_table_line(bool is_task /* true for any timer type, false for any task */, bool is_interrupting /* irrelevant for any task*/){
		// check whether we can modify table:
		macro_interrupt_critical(
		if (flags.get(TABLE_LOCKED)) return TABLE_SIZE; //NO_HANDLE;
		flags.set_true(TABLE_LOCKED);
		);
		/*@when here: table was not locked before, we can access the table, we have locked the table ourself. */
		
		/* search for invalid entry to replace: */
		uint8_t index;
		// we look from max index to min when we want to add a new task, because of table layout [int timer, timer, gaps, tasks]
		// we look from min index to max when we want to add a new timer, because of table layout.
		for(index = static_cast<uint8_t>(is_task) * (TABLE_SIZE -1); (index < TABLE_SIZE); is_task ? --index : ++index){
			if (table[index].flags.get(IS_VALID) == false) goto found_invalid_entry;
		}
		/*@when here: despite we have rights to modify the table, we cannnot because it is already full */
		flags.set_false(TABLE_LOCKED);
		return TABLE_SIZE;
		
		found_invalid_entry:
		uint8_t free_index{ index };
		/*@when here: The table has a free line, we store that index as free_index */
		SchedulerHandle free_handle{ NO_HANDLE };

		// if we want to create an interrupting timer we need to swap position with first non_int timer.
		
		// search for free handle // there must be a free handle since there is at least one free line in the table
		SchedulerHandle candidates[2] = {index, table[index].handle};
		// prefer handle == index in table.... index may grow up during handler life, when task with greater index is deleted
		// second idea: handle = handle of last entry that was written here. // because this handle is possibly free
		uint8_t check_until_index[2] = {index, index};
		
		index = (index + 1) % TABLE_SIZE;
		while (true){
			if (table[index].flags.get(IS_VALID)) // if current line is valid, current line's handle must not be a candidate handle.
				for(uint8_t i = 0; i < 2; ++i)
					if (table[index].handle == candidates[i]){ // candidate is already used at currrent index
						if (i == 0){ // use different update strategies for candidates:
								// idea: go through all possible handles by moving --
							candidates[i] = minus_one_mod_table_size(candidates[i]);
						} else {
								// idea: if we wanted an handle that equals it's entry's index but we found this forbidder here, try the forbidders index
							candidates[i] = index;
						}
						check_until_index[i] = minus_one_mod_table_size(index);
					}
					for(uint8_t i = 0; i < 2; ++i){
						if (check_until_index[i] == index){
							// this candidate is a free handle
							free_handle = candidates[i];
							goto found_free_handle;
						}
					}
			index = (index + 1) % TABLE_SIZE;
		}
		
		found_free_handle:

		/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
		// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
		// <<<<<< furthermore finding free handlers will become much easier and faster
		// <<<<<< searching in method run() will last a little bit longer, I think,
		
		if ((!is_task) && (is_interrupting)){ // we need to swap a new int timer line to the section of int timers!
			if (free_index == 0){
				// we are already in the right section, do nothing
			} else {
				// we are not on top of the array. check whether predecessors are non-int timers.
				for(index = free_index; true; --index ){ // check about a swap of index and free_index
					if ((index == 0) || (table[index-1].flags.get(IS_INTTIMER))) break; // swap with current index
				}
				// we need to swap:
				// move from index to free_index
				table[free_index] = table[index];
				free_index = index;
			}
		}
		table[free_index].handle = free_handle;
		return free_index;	
	}
	
	SchedulerHandle new_timer(const time::ExtendedMetricTime& time, concepts::Callable* callable = nullptr, concepts::void_function function = nullptr, bool is_interrupting = false, bool enable = true){

		// check whether we can modify table:
		macro_interrupt_critical(
		if (flags.get(TABLE_LOCKED)) return NO_HANDLE;
		flags.set_true(TABLE_LOCKED);
		);
		// search for invalid entry to replace.
		uint8_t index;
		// we look from max index to min when we want to add a new task, because of table layout [int timer, timer, gaps, tasks]
		// we look from min index to max when we want to add a new timer, because of table layout.
		// if we want to create an interrupting timer we need to swap position with first non_int timer.
		//for(index = TABLE_SIZE -1; (index < TABLE_SIZE); --index){
		for(index = 0; (index < TABLE_SIZE); ++index){
			if (table[index].flags.get(IS_VALID) == false) goto found_invalid_entry;
		}
		flags.set_false(TABLE_LOCKED);
		return NO_HANDLE;
		
		found_invalid_entry:
		
		uint8_t free_index{ index };
		SchedulerHandle free_handle{ NO_HANDLE };
		
		// search for free handle
		SchedulerHandle candidates[2] = {index, table[index].handle};
		// prefer handle == index in table.... index may grow up during handler life, when task with greater index is deleted
		// second idea: handle = handle of last entry that was written here.
		uint8_t check_until_index[2] = {index, index};
		
		index = (index + 1) % TABLE_SIZE;
		while (true){
			if (table[index].flags.get(IS_VALID)) // check whether a handle candidate is forbidden by this line
			for(uint8_t i = 0; i < 2; ++i)
			if (table[index].handle == candidates[i]){ // candidate is not free
				if (i == 0){ // use different update strategies for candidates:
					candidates[i] = minus_one_mod_table_size(candidates[i]);
					} else {
					candidates[i] = index;
				}
				check_until_index[i] = minus_one_mod_table_size(index);
			}
			for(uint8_t i = 0; i < 2; ++i){
				if (check_until_index[i] == index){
					// this candidate is a free handle
					free_handle = candidates[i];
					goto found_free_handle;
				}
			}
			index = (index + 1) % TABLE_SIZE;
		}
		/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
		// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
		// <<<<<< furthermore finding free handlers will become much easier and faster
		// <<<<<< searching in method run() will last a little bit longer, I think,
		uint8_t free_index_2 = new_table_line(false,is_interrupting);
		
		found_free_handle:
		// some flags:
		table[free_index].flags.set_true(IS_VALID);
		table[free_index].flags.set_true(IS_TIMER);
		table[free_index].flags.set(IS_ENABLED,enable);
		table[free_index].flags.set(IS_INTTIMER,is_interrupting);
		// handle:
		table[free_index].handle = free_handle;
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
		
		flags.set_false(TABLE_LOCKED);
		return free_handle;
	}
	
	SchedulerHandle get_disabled_entries(uint8_t& count, SchedulerHandle* array = nullptr){
		macro_interrupt_critical(
		if (flags.get(TABLE_LOCKED)) {
			count = 0xFF;
			return NO_HANDLE;
		}
		flags.set_true(TABLE_LOCKED);
		);
		uint8_t counter{ 0 };
		SchedulerHandle return_value;
		for(uint8_t index = 0; index < TABLE_SIZE; ++index){
			if (table[index].flags.get(IS_VALID) && !table[index].flags.get(IS_ENABLED)){
				if (counter < count){
					array[counter] = table[index].handle;
				}
				if (counter == 0){
					return_value = table[index].handle;
				}
				++counter;
			}
		}
		count = counter;
		flags.set_false(TABLE_LOCKED);
		return return_value;
	}
	
};

/************************************************************************/
/* FUNCTION IMPLEMENTATION                                              */
/************************************************************************/

template <uint8_t TABLE_SIZE>
uint8_t scheduler2<TABLE_SIZE>::run()
{
	static_assert(TABLE_SIZE >= 1, "Table declared with size 0.");
	flags.set_false(STOP_CALLED); // delete previous stop command
	flags.set_true(IS_RUNNING); // set running flag
	flags.set_false(EMPTY_TABLE_DETECTED); // check for empty table
	wdt_enable(hardware_watchdog_timeout); // start hardware watchdog
	
	central_control_loop:
	while(! flags.get(STOP_CALLED)){ // run central control loop until someone called stop() from inside (or table became empty -> break from inside).
		
		//wdt_reset(); // reset hardware watchdog
		// reset software watchdog:
		
		uint8_t choice; // index inside
		macro_interrupt_critical(
		flags.set_true(TABLE_LOCKED); // lock table: let no one (no interrupt) modify table while we are accessing it
		);
		
		/*** look for a timer that can be executed ***/
		{
			time::ExtendedMetricTime now = scheduler::SysTime::get_instance()(); // system_time();
			for(choice = 0; choice < TABLE_SIZE; ++choice){
				if (	(table[choice].flags.get(IS_VALID) == false)	||	(table[choice].flags.get(IS_TIMER) == false)	){
					goto no_timer_expired; // we reached the end of the timer section, but no timer expired, the table is not full with timer entries
				} // <<<<< we can put this also into loop condition, because there is no different treatment for both cases.
				// at current index we see a valid timer
				if (table[choice].flags.get(IS_ENABLED)){
					// timer is enabled.
					flags.set_false(EMPTY_TABLE_DETECTED); // put this in local bool var if it is possible to delete this variable before executer is started.
					if (now > *table[choice].specifics.timer().event_time) {
						// timer has expired.
						table[choice].flags.set_false(IS_ENABLED); // disable timer to avoid execution twice
						goto execute;
					}
				}
				// side affect of timer order in table: interrupting timers are always executed first
			}
			} /*** end of looking for timer ***/
			
			// when here: table is full with timers, but no one expired, (but it is also possible that all timers)
			
			no_timer_expired:
			
			// when here: table may have timers inside, but no one expired
			
			if (flags.get(EMPTY_TABLE_DETECTED)){ // table is empty we will always have nothing to execute
				goto exit_run_because_of_empty_table;
			}
			
			/*** look for a task that can be executed ***/
			{
				uint8_t min_entry_index{ TABLE_SIZE }; // means no entry
				uint16_t min_entry_race_countdown{ 0xFFFF }; // means no entry
				
				/** look for a task with task_race_countdown == 0 **/
				for (choice = TABLE_SIZE - 1; (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false) && (choice < TABLE_SIZE); --choice){
					/*if ((table[choice].flags.get(IS_VALID) == false) || (table[choice].flags.get(IS_TIMER) == true)){
					// reached end of task section in table
					goto no_zero_countdown; //same as break;
					/// <<< check whether we can put this into loop invariant / condition
					}*/
					if (table[choice].flags.get(IS_ENABLED)){
						// current task has to be considered
						if (table[choice].specifics.task().task_race_countdown == 0){
							goto task_execute;
						}
						if (static_cast<uint16_t>(table[choice].specifics.task().task_race_countdown) < min_entry_race_countdown){
							min_entry_index = choice;
							min_entry_race_countdown = table[choice].specifics.task().task_race_countdown;
						}
					}
					} /** end of looking for task with countdown == 0 **/
					
					// <<< description no more valid: // when here: the whole table is filled with tasks, but no one's task_race_countdown was at zero
					
					// no_zero_countdown:
					
					// when here: reached end of (maybe empty, maybe max-sized) task section, but no task_race_countdown was zero
					
					if (min_entry_index == TABLE_SIZE){ // section has no enabled task <==> is empty
						flags.set_true(EMPTY_TABLE_DETECTED);
						goto central_control_loop; // the task section is empty at the moment, run scheduler from begin.
					}
					
					// reached end of non-empty task section, but no task_race_countdown was zero
					
					/** subtract minimum task_race_countdown from all count_downs in task section and let the min-countdown task be executed afterwards **/
					for (choice = TABLE_SIZE - 1; (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false) && (choice < TABLE_SIZE); --choice){
						if (table[choice].flags.get(IS_ENABLED)){
							table[choice].specifics.task().task_race_countdown -= static_cast<uint8_t>(min_entry_race_countdown);
						}
					}
					// <<< this loop can be merged with the previous loop, head of loop is the same, also first condition in body.
					choice = min_entry_index;
					
					task_execute:
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
				return exit_code;
				
				// reasons of breaking scheduling:
				/*
				0: table got empty
				1: stop was called
				
				3.???? not all requirements for starting were fulfilled
				*/
				
				
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
			}

			template <uint8_t TABLE_SIZE>
			typename scheduler2<TABLE_SIZE>::SchedulerHandle scheduler2<TABLE_SIZE>::new_task(uint8_t urgency /*= STANDARD_URGENCY*/, concepts::Callable* callable /*= nullptr*/, concepts::void_function function /*= nullptr*/, bool enable /*= true*/)
			{
				macro_interrupt_critical(
				if (flags.get(TABLE_LOCKED)) return NO_HANDLE;
				flags.set_true(TABLE_LOCKED);
				);
				// search for invalid entry to replace.
				uint8_t index;
				for(index = TABLE_SIZE -1; (index < TABLE_SIZE); --index){
					if (table[index].flags.get(IS_VALID) == false) goto found_invalid_entry;
				}
				flags.set_false(TABLE_LOCKED);
				return NO_HANDLE;
				
				found_invalid_entry:
				
				uint8_t free_index{ index };
				SchedulerHandle free_handle{ NO_HANDLE };
				
				// search for free handle
				SchedulerHandle candidates[2] = {index, table[index].handle};
				// prefer handle == index in table.... index may grow up during handler life, when task with greater index is deleted
				// second idea: handle = handle of last entry that was written here.
				uint8_t check_until_index[2] = {index, index};
				
				index = (index + 1) % TABLE_SIZE;
				while (true){
					if (table[index].flags.get(IS_VALID)) // check whether a handle candidate is forbidden by this line
					for(uint8_t i = 0; i < 2; ++i)
					if (table[index].handle == candidates[i]){ // candidate is not free
						if (i == 0){ // use different update strategies for candidates:
							candidates[i] = minus_one_mod_table_size(candidates[i]);
							} else {
							candidates[i] = index;
						}
						check_until_index[i] = minus_one_mod_table_size(index);
					}
					for(uint8_t i = 0; i < 2; ++i){
						if (check_until_index[i] == index){
							// this candidate is a free handle
							free_handle = candidates[i];
							goto found_free_handle;
						}
					}
					index = (index + 1) % TABLE_SIZE;
				}
				/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
				// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
				// <<<<<< furthermore finding free handlers will become much easier and faster
				// <<<<<< searching in method run() will last a little bit longer, I think,
				
				found_free_handle:
				// some flags:
				table[free_index].flags.set_true(IS_VALID);
				table[free_index].flags.set_false(IS_TIMER);
				table[free_index].flags.set_false(IS_ENABLED);
				if (enable) table[free_index].flags.set_true(IS_ENABLED);
				// handle:
				table[free_index].handle = free_handle;
				// callback and callback flag:
				if (callable == nullptr){
					table[free_index].callback.set_function_ptr(function);
					table[free_index].flags.set_false(IS_CALLABLE);
					} else {
					table[free_index].callback.set_callable_ptr(callable);
					table[free_index].flags.set_true(IS_CALLABLE);
				}
				// task specific information: task urgency and countdown:
				table[free_index].specifics.task().urgency = urgency;
				table[free_index].specifics.task().task_race_countdown = 0;
				
				flags.set_false(TABLE_LOCKED);
				return free_handle;

			}


			#endif //__SCH2_H__
