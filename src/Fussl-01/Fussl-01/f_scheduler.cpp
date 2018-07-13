/*
 * f_scheduler.cpp
 *
 * Created: 24.08.2017 18:13:52
 *  Author: Maximilian Starke
 */ 

// bedingte Kompilierung für Optimierungs Features:

#if false

#ifdef earliest_timer_deadline
	#error "config macro already defined."
#endif
#ifdef earliest_non_interrupt_timer
	#error "config macro already defined."
#endif
#ifdef keep_non_int_timers_sorted
	#error "config macro already defined."
#endif
#ifdef always_completely_go_through_unsorted_timers
	#error "config macro already defined."
#endif
#ifdef aaa
#error "config macro already defined."
#endif
#ifdef aaa
#error "config macro already defined."
#endif
#ifdef aaa
#error "config macro already defined."
#endif

/************************************************************************/
/*                  config macros                                       */
/************************************************************************/

	// let library use an additional memory place to store earliest time for an interrupting timer.
	// if undefined inteerrupting timers must be sorted in the list.
// #define earliest_interrupt_time


// #define earliest_non_interrupt_timer

// #define keep_non_int_timers_sorted

// #define always_completely_go_through_unsorted_timers

/*
#ifndef earliest_non_interrupt_timer
	#ifndef keep_non_int_timers_sorted
		#ifndef always_completely_go_through_unsorted_timers
			#error "At least one of these macros must be defined."
		#endif
	#endif
#endif
*/
#ifdef earliest_non_interrupt_timer
	#ifdef ___doubled___
		#error "internal confog macro already defined."
	#endif
	#define ___doubled___
#endif
#ifdef keep_non_int_timers_sorted
	#ifdef ___doubled___
		#error "At most one macro may be defined."
	#endif
	#define ___doubled___
#endif
#ifdef always_completely_go_through_unsorted_timers
	#ifdef ___doubled___
		#error "At most one macro may be defined."
	#endif
	// #define ___doubled___ // this line is optional
#endif

#include "f_scheduler.h"

#include "f_system_time.h"

namespace scheduler {
	
	system_time* sys_time; // ## i did not care about init yet.
	SchedulerMemoryLine* p_table {nullptr}; // ptr to the table
	uint8_t s_table {0}; // table size as the number of SchedulerMemoryLines
		
	uint32_t watchdog{NO_WATCHDOG_COUNTER_FORMAT};
		
	uint8_t is_active{0}; // read-only by ISR, every write access is critical
	uint32_t countdown; // read-only by ISR, every write access is critical
	
	// non-extern // shoulbe namespace {}
	// da könnte man überall indexer dahinter legen. dannn sparen wir die hälfte des speicherplatzes
	
	SchedulerMemoryLine* &int_timer_begin{p_table};
	SchedulerMemoryLine* non_int_timer_begin; // first element
	SchedulerMemoryLine* &int_timer_end{non_int_timer_begin}; // behind last element
	SchedulerMemoryLine* timer_end; // behind last element
	SchedulerMemoryLine* &non_int_timer_end{timer_end}; // behind last element
		
	GroupCellSize* cell_sizes_begin; // ptr to first group cell size entry
	GroupCellSize* cell_sizes_end(){return cell_sizes_begin + (/*task_end()#########*/0 /*- task_begin*/); } // ptr behind last element of groupcellsize table
	
	SchedulerMemoryLine* task_begin; // first element
	SchedulerMemoryLine* task_end(){ return p_table + s_table; } // behind last element
		// is_timer und is_valid flag werden dadurch eingespart.

	
	bool dirty_table{true}; // forgot what it is for.
		// so say now. whenever you delete a timer/task you just make it kinda invalid but don't
		// clean up the table. clean up the table somewhen, when you passed on e; group circle e.g.
		// when you disable some task this way you need to mark table dirty (set true).
		// the purpose is, if you try to add a new task you call an lib-internal allocate function.
		// if allocation fails first, then clean up the table. and try again to allocate place for the new timer entry.
		
		// further explanation: that is you do not have to go through the whole task table to look for
		// a free space chunk. instead you just do this clean up once.
		// so ein flahg kann man auch in die ertse schdeuler mem line intergrieren in die Flags die da hinten drin stehehn
	
	// more variables.
	
	time::ExtendedMetricTime earliest_timer_deadline; // earliest deadline for (any???) timer.
	// this is time optimizing. we don't need to go through every single timer.
	// we also could distinguish into int-timer and non-int-timer
	
	// for int-timer we must have such a var or we need to order the int-timers ascending by deadline time.
	// IsubsubsubSR must not need much time. one time comparison should be enough
	
	/*
		ptr auf den aktuellen task. // can ein indexer sein. dann benötigen wir weniger
		item nummer des aktuellen items vom task. // 8bit
		
	
	
	
	*/
	
	/****
	scheduler table:
	
	int timer
	int timer
	int timer
	timer
	timer
	timer
	invalid
	...
	invalid
	_
	CellSizeArray
	_
	invalid
	...
	INVALID
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

#endif