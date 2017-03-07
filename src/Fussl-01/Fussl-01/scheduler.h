/*
 * scheduler.h
 *
 * Created: 07.03.2017 18:16:16
 *  Author: Maximilian Starke
 */ 


#ifndef SCHEDULER_H_
#define SCHEDULER_H_
//#include <time.h>

namespace scheduler {
	
	class Time {
		
	};
	
	typedef uint16_t SCHEDULE_HANDLE;
	
	constexpr uint16_t NO_HANDLER {0};
	
	void getDayOfWeek(){};
	
	SCHEDULE_HANDLE addTimer(void (*function)(), const Time& interval, uint16_t repeat){return 0;};
		
	bool cancelTimer(SCHEDULE_HANDLE handler){return false;};
	
}

#endif /* SCHEDULER_H_ */