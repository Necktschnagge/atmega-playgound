/*
* f_system_time.h
*
* Created: 22.08.2017 18:41:29
*  Author: Maximilian Starke, Jens Kästner, Johannes König
*/


#ifndef F_SYSTIME_H_
#define F_SYSTIME_H_

//#include "f_callbacks.h"
//#include "f_macros.h"
#include "f_time.h"


namespace fsl {
	namespace os {
		class clock_watch_quartz {
		public:
			inline static uint8_t start();
			inline static uint8_t stop();
			static system_time* instance;
		};
		class system_time {
		private:
			time::ExtendedMetricTime delta_t;
			time::ExtendedMetricTime now;
		public:
			system_time(const time::ExtendedMetricTime& delta_t);
			inline time::ExtendedMetricTime get_now();
			inline void set_now(const time::ExtendedMetricTime& time);
		};
		class ExtendedMetricTime;


	}
} // namespace
#endif /* F_SYSTIME_H_ */