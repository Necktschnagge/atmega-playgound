/*
* f_task_runner.h
*
* Created: 05.03.2018 18:14:50
*  Author: Maximilian Starke
*
*	FPS: everything alright
*
*/


#ifndef __F_TASK_RUNNER_H__
#define __F_TASK_RUNNER_H__

#include <stdint.h>

namespace kernel {
	
	/* a simple (stupid-strategy) scheduler
	it does not really care about scheduling and rather executes one procedure after another */
	
	/* SIZE is the max amount of task, that are executed in an infinite loop. */
	template <uint8_t SIZE>
	class TaskRunner {
		public:
		using task_handle = uint8_t;
		
		static constexpr task_handle NO_HANDLE{ 0xFF };
		private:
		struct task {
			void (*callback)() = nullptr; // is valid if not nullptr and if enable == true
			bool enable = false;
		};
		
		/* array of all tasks */
		task task_table[SIZE];
		
		uint8_t next_position;
		
		public:
		
		/* create TaskRunner with empty table */
		TaskRunner(): next_position(0) {
			for (uint8_t i = 0; i < SIZE; ++i) task_table[i].enable = false;
		}
		
		/* let the TaskRunner::run() return after current running task is finished. */
		/* is interrupt critical. do not call by interrupt */
		inline void exit(){	next_position = 0xFF;	}
		
		/* adds given task to table and enables it.
		returns task_handle of task iff successful
		returns NO_HANDLE iff table full */
		task_handle add_task(void (* callback)());
		
		/* removes a still enabled task from the task table
		returns true if task was removed.
		returns false if handle invalid or no such task. */
		bool remove_task(task_handle handle);
		
		/* starts the task runner.
		TaskRunner can be stopped by calling exit(),
		TaskRunner will stop immediately if table becomes empty
		TaskRunner will stop immediately if table size <2 */
		/* return value is code for the reason of aborting:
		0 ... table became empty
		1 ... exit was called
		2 ... table size to small*/
		uint8_t run();
	};

}


/**************************************************************************************
-------------------------------IMPLEMENTATION------------------------------------------
***************************************************************************************/

template <uint8_t SIZE>
auto kernel::TaskRunner<SIZE>::add_task(void (* callback)()) -> kernel::TaskRunner<SIZE>::task_handle
{
	for (uint8_t i = 0; i < SIZE; ++i){
		if (task_table[i].enable == false){
			task_table[i].callback = callback;
			task_table[i].enable = true;
			return i;
		}
	}
	return NO_HANDLE;
}

template <uint8_t SIZE>
bool kernel::TaskRunner<SIZE>::remove_task(task_handle handle)
{
	if (handle < SIZE)
	if (task_table[handle].enable){
		task_table[handle].enable = false;
		return true;
	}
	return false;
}


template <uint8_t SIZE>
uint8_t kernel::TaskRunner<SIZE>::run()
{
	if (SIZE < 2) return 2;
	uint8_t empty_check{ 0 };
	while (true){
		if (next_position == 0xFF) return 1;
		uint8_t current_position = next_position;
		next_position = (next_position+1) % SIZE;
		++empty_check;
		if ((task_table[current_position].enable) && (task_table[current_position].callback != nullptr)){
			task_table[current_position].callback();
			empty_check = 0;
		}
		if (empty_check == SIZE) return 0;
	}
}


#endif //__F_TASK_RUNNER_H__
