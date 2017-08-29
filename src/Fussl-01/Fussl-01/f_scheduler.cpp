/*
 * f_scheduler.cpp
 *
 * Created: 24.08.2017 18:13:52
 *  Author: Maximilian Starke
 */ 


#include "f_scheduler.h"

#include "f_systime.h"

namespace scheduler {
	
	SysTime* sys_time; // ## i did not care about init yet.
	SchedulerMemoryLine* p_table {nullptr};
	uint8_t s_table {0};
	uint32_t watchdog{NO_WATCHDOG_COUNTER_FORMAT};
	uint8_t is_active{0}; // read-only by ISR, every write access is critical
	uint32_t countdown; // read-only by ISR, every write access is critical
	
	// non-extern
	SchedulerMemoryLine* &int_timer_begin{p_table};
	SchedulerMemoryLine* non_int_timer_begin; // first element
	SchedulerMemoryLine* &int_timer_end{non_int_timer_begin}; // behind last element
	SchedulerMemoryLine* timer_end; // behind last element
	SchedulerMemoryLine* &non_int_timer_end{timer_end}; // behind last element
	SchedulerMemoryLine* task_begin; // first element
	SchedulerMemoryLine* task_end(){ return p_table + s_table; } // behind last element
	
	bool dirty_table{true};
		
		// is_timer und is_valid flag werden dadurch eingespart.
			
	/****
	scheduler table:
	
	int timer
	int timer
	int timer
	timer
	timer
	timer
	invalid
	invalid
	...
	invalid
	task
	task
	task
	
	*****/
	
	
	// central control loop. called after init and provides the tasks
	bool run(){
		if (is_active) return false; // already running. run must not be an indirect subroutine of run.
		
		macro_interrupt_critical( is_active = 1; );
		
		// start hardware watchdog here
		
		while (is_active != 2) // break if a stop() was called.
		{
			// reset software watchdog
			 macro_interrupt_critical( countdown = watchdog; dirty_table = false; );
			
			again_select_timer:
				SchedulerHandle handle{NO_HANDLE};
			//macro_interrupt_critical( // iterators are invalidated if anything changes schedule table.
										// tasks /noninttimers cannot at the moment, but int.timers who might call such methods
										// ### it is important to check whether this loop has impact on time measurement
								// to fix this it would be better to set a dirty flag when changing the ptrs / table content
				for(SchedulerMemoryLine* ptr = non_int_timer_begin; (!dirty_table) && (ptr < non_int_timer_end); ++ptr){
					if (  (ptr->specifics.timer().get_execution_time() < sys_time->get_now_time())  &&  (ptr->flags.is_enabled())){
						handle = ptr->handle;
						break;
					}
				}
				if (dirty_table) goto again_select_timer;
				
				// tasks ausführen.oi
				if (handle == NO_HANDLE){ // select a task
					
					for (SchedulerMemoryLine* ptr = task_begin; (!dirty_table) && (ptr < task_end()); ++ptr){
					}
				}
		//	);
			// look for an handle which has to be executed:
				// first look, if any non-interrupting timer is on schedule
				// than otherwise choose a task
			//### here i think that the begin and end pointer are all correct
			
			
			// run this handle-method
			
		}
		
		macro_interrupt_critical( is_active = 0;);
		
		// stop hardware watchdog
		
		return true;
	}

	
}