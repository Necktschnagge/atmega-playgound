/*
* f_task_runner.h
*
* Created: 05.03.2018 18:14:50
*  Author: Maximilian Starke
*/


#ifndef __F_TASK_RUNNER_H__
#define __F_TASK_RUNNER_H__

#include <stdint.h>

namespace kernel {
	
	/* a simple (stupid-strategy) scheduler
	it does not really care about scheduling and rather executes one procedure after another */
	
	template <uint8_t SIZE>
	class TaskRunner {
		public:
		typedef uint8_t task_handle;
		
		private:
		struct task {
			void (*callback)() = nullptr;
			bool enable = false;
		};
		
		task task_table[SIZE];
		
		uint8_t next_position;
		
		public:
		
		TaskRunner(): next_position(0) {
			for (uint8_t i = 0; i < SIZE; ++i) task_table[i].enable = false;
		}
		
		/* is interrupt critical. do not call by interrupt */
		inline void exit(){	next_position = 0xFF;	}
		
		task_handle add_task(void (* callback)()){
			for (uint8_t i = 0; i < SIZE; ++i){
				if (task_table[i].enable == false){
					task_table[i].callback = callback;
					task_table[i].enable = true;
					return i;
				}
			}
			return task_handle(0xFF);
		}
		
		bool remove_task(task_handle handle){
			if (handle < SIZE)
				if (task_table[handle].enable){
					task_table[handle].enable = false;
					return true;
			}
			return false;
		}
		
		void run(){
			if (SIZE == 0) return;
			while (true){
				if (next_position == 0xFF) return;
				uint8_t current_position = next_position;
				next_position = (next_position+1) % SIZE;
				if ((task_table[current_position].enable) && (task_table[current_position].callback != nullptr))
					task_table[current_position].callback();
			}
		}
	};
	
}



#endif //__F_TASK_RUNNER_H__
