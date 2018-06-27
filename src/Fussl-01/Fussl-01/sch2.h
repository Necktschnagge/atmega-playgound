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

#include <math.h>

#include "f_macros.h"
#include "f_range_int.h"
#include "f_concepts.h"
#include "f_time.h"
#include "f_systime.h"

template <uint8_t TABLE_SIZE>
class scheduler2 {
	
	//// ###think about volatile stuff in this class
	
	public:
	
	/*** public types ***/
	
	using SchedulerHandle = fsl::lg::range_int<uint8_t,TABLE_SIZE,true,true>;
	
	/*** public constexpr constants ***/
	
	/* refers to none of the entries in scheduler table */
	static constexpr SchedulerHandle NO_HANDLE{ SchedulerHandle::OUT_OF_RANGE };
	
	/* the default urgency for task entries in scheduler table */
	static constexpr uint8_t DEFAULT_URGENCY{ 50 };
	
	private:
	
	/*** private types ***/

		/* instances of this type contain either a pointer to concepts::Callable or a concepts::void_function */	
	class UnionCallback {
		private:
		union InternUnion {
			concepts::void_function function_ptr;
			concepts::Callable* callable_ptr;
			
			InternUnion(): function_ptr(nullptr){}
		};
		InternUnion _union;
		
		public:			
		/* set a function pointer, replaces the previously stored function pointer oder callable pointer */
		inline void set_function_ptr(concepts::void_function ptr){ _union.function_ptr = ptr; }
		inline void set_callable_ptr(concepts::Callable* ptr) { _union.callable_ptr = ptr; }
		inline void call_function() const { if (_union.function_ptr != nullptr) _union.function_ptr(); }
		inline void call_callable() const { if (_union.callable_ptr != nullptr) (*_union.callable_ptr)(); }
	};
		
	struct TaskSpecifics {
		/* value, that is added to task_race_countdown after each callback execution
		   255 .. least important ... 0 ... most important
		*/
		uint8_t urgency_inverse;
		/* Always the task with the smallest task_race_countdown will be executed. If this is ambiguous one of them will be executed.
		   After a task is executed task_race_countdown will be increased by urgency_inverse.
		   At least when a task_race_countdown would overflow if it has to be increased, all task_race_countdowns have to be shorten together.
		   To do so, determine the min task_race_countdown and subtract this value from all task_race_countdowns.
		*/
		uint8_t task_race_countdown;		
	};

	struct TimerSpecifics {
		/* earliest time when timer can be executed */
		volatile time::ExtendedMetricTime* event_time;
	};
	
	class UnionSpecifics {
		private:
		union InternUnion {
			TaskSpecifics task;
			TimerSpecifics timer;
		};
		InternUnion _union;
		public:
		TaskSpecifics& task() { return _union.task; }
		TimerSpecifics& timer() { return _union.timer; }
		// design decision justification: using direct references makes objects become larger by 2*ptrsize, I tried it
	};
	
	struct SchedulerMemoryLine {
		SchedulerHandle handle; // 1
		UnionCallback callback; // 2
		UnionSpecifics specifics; // 2
		concepts::Flags flags; // 1
	};
	
	/*** private constexpr constants ***/
	
	/* flags for each scheduler line */
	static constexpr concepts::Flags::flag_id IS_VALID{ 0 }; // if an entry is not valid, the entry may be overwritten.
	static constexpr concepts::Flags::flag_id IS_ENABLED{ 1 }; // if an entry is valid, then the entry will be considered on scheduling decisions iff is_enabled
	// timer will automatically be disabled right before executing, if you want the timer to execute again after a certain time, you have to reenable from inside the timer procedure
	static constexpr concepts::Flags::flag_id IS_TIMER{ 2 }; // if entry is valid, then entry (containing unions) is treated as a timer iff IS_Timer and as task otherwise.
	static constexpr concepts::Flags::flag_id IS_INTTIMER{ 3 }; // if entry is_valid and is_timer, then callback may be executed as soon as event_time is reached on system time interrupt iff IS_INTTIMER. Otherwise is has to wait until the current task/timer returned.
	static constexpr concepts::Flags::flag_id IS_CALLABLE{ 4 }; // if entry is_valid, the callback union will be treated as a callable iff IS_CALLABLE, otherwise as function pointer.

	/* flags once for every scheduler object */
	static constexpr concepts::Flags::flag_id IS_RUNNING{ 0 }; // true iff run() runs // write access only by run()
	static constexpr concepts::Flags::flag_id STOP_CALLED{ 1 }; // if true, run() should return when current task returns.
	// if it is true no int timers will be executed anymore until scheduler was started again.
	static constexpr concepts::Flags::flag_id TABLE_LOCKED{ 2 }; // if scheduler is accessing scheduling table, it must not be modified by interrupt timers, modifying only allowed if you own TABLE_LOCKED.
	static constexpr concepts::Flags::flag_id EMPTY_TABLE_DETECTED{ 3 }; // for function-local internal use in run() only.
	static constexpr concepts::Flags::flag_id SOFTWARE_INTERRUPTS_ENABLE{ 4 }; // scheduler will only look for int timers at now time update if enabled
	
	/*** private data ***/
	
	/* the scheduler object's flags */
	volatile concepts::Flags flags;
	
	
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
		There is an object's flag TABLE_LOCKED.
		One might set TABLE_LOCKED true if it was false in an atomic step at some point.
		Then he is called the owner of TABLE_LOCKED. One should eventually release TABLE_LOCKED, i.e. set it false.
		You are always allowed to read from the table
		if you own TABLE_LOCKED you are allowed to read from a "non-volatile" (there wont be anyone interrupting you and changing table while you are reading) table and modify the table.
		if you do not own TABLE_LOCKED you are only allowed to read the table, but there is no guarantee that table is non-volatile and also
		no guaranty that table is in a consistent state if you are the interrupt during some table modification.
	*/
	volatile SchedulerMemoryLine table[TABLE_SIZE];

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
	
	volatile time::ExtendedMetricTime earliest_interrupting_timer_release;
	//### on construction it must be set to now!!! // set to min!!!
	//###when add a int timer it must be updated to the min of old and time fo new int_timer
	

	/*** private methods ***/
	
	inline void software_watchdog_reset(){ macro_interrupt_critical(software_watchdog_countdown_value = software_watchdog_reset_value;); }
		
	/* executes a callback. if callback pointer is nullptr, nothing will be done 
	   iff (free_and_unfree_lock) the table_locked will be turned off right before callback and turned on right after callback */
	template<bool free_and_unfree_lock = false>
	inline void execute_callback(UnionCallback callback, bool is_callable){
		if (free_and_unfree_lock) flags.set_false(TABLE_LOCKED);
		(is_callable) ? callback.call_callable() : callback.call_function();
		if (free_and_unfree_lock) flags.set_true(TABLE_LOCKED);
	}
	
	/* executes callback of given table index.
	   The callback will automatically be checked if it is nullptr, in this case nothing will be done
	   Make sure that table is non-volatile during function call - You should own table.
	   iff (free_and_unfree_lock) the table_locked will be turned off right before callback and turned on right after callback */
	template<bool free_and_unfree_lock = false>
	inline void execute_callback(uint8_t table_index){
		return execute_callback<free_and_unfree_lock>(table[table_index].callback,table[table_index].flags.get(IS_CALLABLE));
	}
	
	/* goes through all (valid) int timer entries and executes the callback if execution_time expired and entry is enabled.
	   all callback subroutines are called without table being locked.
	   returns	0:	!!! error:		table is already locked, nothing is done by this function
				1:	regular call:	algorithm went through table, probably executing at least one callback 
				2:	regular call:	earliest_time was not reached, no iteration through table
	*/   
	inline uint8_t execute_interrupting_timers(){
		time::ExtendedMetricTime new_earliest_interrupting_timer_release = time::ExtendedMetricTime::MAX();
		time::ExtendedMetricTime now = scheduler::SysTime::get_instance()();
		
		if (now < earliest_interrupting_timer_release)		return 2; 
		if (flags.get(TABLE_LOCKED))						return 0;
		// Wenn das interrupt kommt, während der scheduler scheduled, dann haben wir ein Prolem!
		//#### das darf nicht sein, es muss trotzdem ein inttimer ausgeführt werden.
		// die Modifikationen vom scheduler oder subrutinen müssen atomar gegen interrupts geschützt werden.
		// dann ist bei jedem interrupt sichergestellt, dass zumindest die inttimer abgearbeitet werden können.
		// das hebelt aber irgendwie das TABLE-locked aus!!!!
		// ein table lock muss also immer mit atomarer kapselung versehen werden???
		// das führt auch in teufels küche...
		
		//lösungsideen:
		/************************************************************************/
		/* 
		1. eine schedulerline darf nur in atomarer umgebung geändert werden
		2. in der schedulertable shiften (d.h. die relation aus Handle und Index ändern) darf nur, wer TABLE LOCK besitzt.
		
		-> inttimer executer hat immer eine konsistente table und darf darin atomar das enable von ausgeführten timern ändern.
		-> scheduler kann sich gegen umordnen der Table sichern, und für den Zeitraum des AUslesen eines eintrages in eine atomater section gehen.
		
		                                                                     */
		/************************************************************************/
		
		flags.set_true(TABLE_LOCKED);
		uint8_t index = 0;
		
		// #### check at all other positions where we have merged the loop condition, that we do not access before checking out of range like in next line: #####
		while(index < TABLE_SIZE){
			if ((table[index].flags.get(IS_VALID) == false) || (table[index].flags.get(IS_TIMER) == false) || (table[index].flags.get(IS_INTTIMER) == false)){
				break;
			}
			if (table[index].flags.get(IS_ENABLED)){
					if (now >= *table[index].specifics.timer().event_time){
						table[index].flags.set_false(IS_ENABLED);
						execute_callback<true>(index);
						index = 0;
						continue;
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
		flags.set_false(TABLE_LOCKED);
		earliest_interrupting_timer_release = new_earliest_interrupting_timer_release;
		return 1;
	}
	
	/* activate hardware watchdog and set the timeout to hardware_watchdog_timeout */
	void activate_hardware_watchdog(){
		macro_interrupt_critical(
		wdt_enable(hardware_watchdog_timeout);
		);
	}
	
	/* deactivate hw watchdog */
	void deactivate_hardware_watchdog(){
		macro_interrupt_critical(
		wdt_disable();
		);
	}
	
	public:
	
	/*** public data ***/
	
	/*
	IMPORTANT: There should not be any >>>static<<< [public] data since class is template.
	*/
	
	/*** public methods ***/
	
	/* executes all stuff that should be done on now_time update, including hardware watchdog reset, int-timer execution e.a.
	   should be called once on every update of the now time */
	inline void time_update_interrupt_handler(){
		if ((software_watchdog_reset_value == 0) /*software watchdog disabled*/ || (software_watchdog_countdown_value > 0) /*software watchdog not exceeded*/)
		{
			--software_watchdog_countdown_value;
			wdt_reset();
			// replace hd watchdog calls in run() by calls to software watchdog. ####
			//#1
		}
		if (flags.get(SOFTWARE_INTERRUPTS_ENABLE) && (flags.get(STOP_CALLED) == false)) execute_interrupting_timers();
	}
	
	/* returns size in bytes of one SchedulerMemoryLine */
	static constexpr uint8_t table_line_size_of(){ return sizeof(SchedulerMemoryLine); }
	/* returns size in bytes of the whole scheduler table */
	static constexpr uint16_t table_size_of(){ return sizeof(SchedulerMemoryLine)*TABLE_SIZE; }
	
	/* deactivates software watchdog
	   note that hardware watchdog is still in use and reboots controller if 
		* you use too long interrupt timer handlers or
		* if the Scheduler itself is corrupt and deactivates interrupts for too long
	*/
	inline void deactivate_software_watchdog(){		macro_interrupt_critical(software_watchdog_reset_value = 0;);	}
	
	/* activate the software watchdog for running scheduler.
	   software watchdog will be startet when you call run() and stopped before run() returns, but may be enabled before you call run()
	*/
	template <bool round_to_ceiling = false, bool truncate = false, bool truncate_and_add_one = true>
	inline void activate_software_watchdog(const time::ExtendedMetricTime& watchdog){
		if (watchdog <= 0) return deactivate_software_watchdog();
		macro_interrupt_critical(
			static_assert(0 + round_to_ceiling + truncate + truncate_and_add_one == 1, "There must be one template argument being true and two being false");
			if (round_to_ceiling){
				software_watchdog_reset_value =
				static_cast<decltype(software_watchdog_reset_value)>(
					ceil(
						static_cast<long double>(watchdog.value) * (scheduler::SysTime::get_instance().precision())
					)
				);
			} 
			if (truncate){
				software_watchdog_reset_value = static_cast<decltype(software_watchdog_reset_value)>
					(
						static_cast<long double>(watchdog.value) * (scheduler::SysTime::get_instance().precision())
					);			
			}
			if (truncate_and_add_one){
				software_watchdog_reset_value = static_cast<decltype(software_watchdog_reset_value)>
				(
				static_cast<long double>(watchdog.value) * (scheduler::SysTime::get_instance().precision())
				)
				+ 1;
			}
		);
	}	
	
	/* central control loop / scheduling loop to call after construction  of scheduler
	   return code:
			0	:	scheduler terminated because of empty table
			1	:	scheduler terminated because of STOP call from inside
			2	:	scheduler terminated because another one owns TABLE_LOCKED
	*/
	uint8_t run();

	/* make scheduler return after current executed task / timer */
	inline void stop(){		macro_interrupt_critical(flags.set_true(STOP_CALLED););		}

	/* returns (uint - 1) mod TABLE_SIZE */
	static constexpr uint8_t minus_one_mod_table_size(uint8_t uint){
		return static_cast<uint8_t>( (static_cast<uint16_t>(uint) + static_cast<uint16_t>(TABLE_SIZE -1)) % static_cast<uint16_t>(TABLE_SIZE) );
	}
	
	/*
	try to add new task to table.
	returns SchedulerHandle of created task
	if not successful NO_HANDLE is returned.
	*/
	SchedulerHandle new_task(concepts::Callable* callable = nullptr, concepts::void_function function = nullptr, uint8_t urgency = DEFAULT_URGENCY, bool enable = true);
	
	
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
							goto found_free_handle1;
						}
					}
			index = (index + 1) % TABLE_SIZE;
		}
		
		found_free_handle1:

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
		//#### go one here and with function new table line
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
					goto found_free_handle2;
				}
			}
			index = (index + 1) % TABLE_SIZE;
		}
		/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
		// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
		// <<<<<< furthermore finding free handlers will become much easier and faster
		// <<<<<< searching in method run() will last a little bit longer, I think,
		found_free_handle2:

		uint8_t free_index_2 = new_table_line(false,is_interrupting);
		// this line was before found_free_handle2, compiler came up with error. check behaviour!!!!#######
		
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
	
	

	/*** static assertions ***/
		
	static_assert(sizeof(SchedulerHandle) == 1, "SchedulerHandle has not the appropriate size.");
	static_assert(sizeof(UnionCallback) == 2, "UnionCallback has not the appropriate size.");
	static_assert(sizeof(UnionSpecifics) == 2, "UnionSpecifics has not the appropriate size.");
	static_assert(sizeof(concepts::Flags) == 1, "concepts::Flags has not the appropriate size.");
	static_assert(sizeof(SchedulerMemoryLine) == 6, "SchedulerMemoryLine has not the appropriate size.");

};

/************************************************************************/
/* FUNCTION IMPLEMENTATION                                              */
/************************************************************************/

template <uint8_t TABLE_SIZE>
uint8_t scheduler2<TABLE_SIZE>::run() // method ready, checked twice!!!
{
	static_assert(TABLE_SIZE >= 1, "Table declared with size 0.");
	flags.set_false(STOP_CALLED); // delete previous stop command
	flags.set_true(IS_RUNNING); // set running flag
	flags.set_false(EMPTY_TABLE_DETECTED); // check for empty table
	wdt_enable(hardware_watchdog_timeout); // start hardware watchdog
	wdt_reset();
	software_watchdog_reset();
		
	central_control_loop:
	while(! flags.get(STOP_CALLED)){ // run central control loop until someone called stop() from inside (or table became empty -> break from inside).
		
		software_watchdog_reset();		
		
		uint8_t choice; // index inside
		
		macro_interrupt_critical(
			choice = flags.get(TABLE_LOCKED); // union-like use of choice for table lock check.
			flags.set_true(TABLE_LOCKED); // lock table: let no one (no interrupt) modify table while we are accessing it
		);
		if (choice) goto exit_run_because_table_is_locked;
		
		// Now we own the TABLE_LOCKED lock.
		
		/*** look for a timer that can be executed ***/
		{
			time::ExtendedMetricTime now = scheduler::SysTime::get_instance()(); // system_time();
			for(choice = 0;
				/* in range */ (choice < TABLE_SIZE) && /* and */ (! /* not out of section */ ((table[choice].flags.get(IS_VALID) == false) || (table[choice].flags.get(IS_TIMER) == false)) );
				++choice){
				// at current index we see a valid timer
				if (table[choice].flags.get(IS_ENABLED)){
					// timer is enabled.
					macro_interrupt_critical(
						flags.set_false(EMPTY_TABLE_DETECTED); // put this in local bool var if it is possible to delete this variable before executer is started.
					);
					if (now > *table[choice].specifics.timer().event_time) {
						// timer has expired.
						macro_interrupt_critical(
							table[choice].flags.set_false(IS_ENABLED); // disable timer to avoid execution twice
						);
						goto execute;
					}
				}
				// side affect of timer order in table: interrupting timers are always executed first
			}
		} /*** end of looking for timer ***/
			
		// when here: table may have or may not have or may consist only of timers inside, but no one expired
		// EMPTY_TABLE_DETECTED was set to false when looking through timers if there was at least one enabled timer
			
		if (flags.get(EMPTY_TABLE_DETECTED)){ // table empty, nothing to schedule anymore
			goto exit_run_because_of_empty_table;
		}
			
		/*** look for a task that can be executed ***/
		{
			uint8_t min_entry_index{ TABLE_SIZE }; // means no entry
			uint16_t min_entry_race_countdown{ 0xFFFF }; // means no entry
				
			/** look for a task with task_race_countdown == 0 **/
			for (choice = TABLE_SIZE - 1;
				(choice < TABLE_SIZE) && (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false);
				--choice){
				// choice is an index where we see a valid task entry
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
								
			// when here: reached end of (maybe empty, maybe max-sized, maybe between) task section, but no task_race_countdown was zero
					
			if (min_entry_index == TABLE_SIZE){ // <==> section has no enabled task <==> task section is called empty (but not 0-sized i.g.)
				flags.set_true(EMPTY_TABLE_DETECTED);
				goto central_control_loop; // the task section is empty at the moment, run scheduler from begin.
			}
					
			// reached end of non-empty task section, but no task_race_countdown was zero
					
			/** subtract minimum task_race_countdown from all count_downs in task section and let the min-countdown task be executed afterwards **/
			for (choice = TABLE_SIZE - 1; (choice < TABLE_SIZE) && (table[choice].flags.get(IS_VALID) == true) && (table[choice].flags.get(IS_TIMER) == false); --choice){
				if (table[choice].flags.get(IS_ENABLED)){
					table[choice].specifics.task().task_race_countdown -= static_cast<uint8_t>(min_entry_race_countdown);
				}
			}
			choice = min_entry_index;
			
			task_execute:
			table[choice].specifics.task().task_race_countdown = table[choice].specifics.task().urgency;
			// goto execute; // implicitly done
		} /*** end of looking for task to be executed ***/
			
		execute:
		{
			const bool is_callable = table[choice].flags.get(IS_CALLABLE);
			UnionCallback callback = table[choice].callback;
			flags.set_false(TABLE_LOCKED);
			
			execute_callback(callback,is_callable);
		} /*** end of execute ***/
	}
	/* reasons for exit run() */
	
	uint8_t exit_code;
	exit_run_because_of_stop_call: /* by leaving central control loop by violating loop invariant */
	{
		// when here: central control loop ended because stop was called
		exit_code = 1;
		goto exit_run;
	}
	exit_run_because_table_is_locked:
	{
		exit_code = 2;
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
		///##### run starts and stops hardware watchdog. change this for better control about when we reset watchdog by now time interrupt.
	}
	return exit_code;				
	
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
	nstead of "re baseing" we could also use an extra variable as last minimum, new minimum will be lowest number >= last min.
	
	// just implement one strategy and later implement the others and make it possible to choose by conditioned compiling.
	}
	*/
}



template <uint8_t TABLE_SIZE>
typename scheduler2<TABLE_SIZE>::SchedulerHandle scheduler2<TABLE_SIZE>::new_task(concepts::Callable* callable /*= nullptr*/, concepts::void_function function /*= nullptr*/, uint8_t urgency /*= DEFAULT_URGENCY*/, bool enable /*= true*/)
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
				goto found_free_handle3;
			}
		}
		index = (index + 1) % TABLE_SIZE;
	}
	/// <<<< this is much work to find free handlers, think about allowing to put all entries unordered into the table.
	// <<<<<< then we can always use the indices as handlers, -> we need less memory for the table (1 byte per line)
	// <<<<<< furthermore finding free handlers will become much easier and faster
	// <<<<<< searching in method run() will last a little bit longer, I think,
	
	found_free_handle3:
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


////////////////////////////////////////////////////////////////////////// conceptional:

	/*
	//static scheduler2<TABLE_SIZE>* instance; /// here is a big problem: our class is template. // someone (interrupt by now time update) cannot know which function to call
	// should be no problem if behaviour is like: each template instanciation has its own static instance pointer.
	// this is the case that every tempolate instanciation has its own static member, you have to extern define these members, otherwise linkage errors will come up.
	// user has to write an own function that is called by systime now update and only propergates i.e. calls MY_SCHEDULER::interrupt handler(); where 
	// MY_SCHEDULER has to be defined with using ~ = scheduler2<20>; or similar
	
	// i think it is better to do not have this static member. user must provide one self-written function as callback handler which calls instance->time_update_interrupt_handler()
	*/


////
/*
 ideas for further improvements
 
	always save a copy of earliest time a int timer has to be executed: This way it is no longer necessary to go through whole table on each timer interrupt
 
 
 
 
 */