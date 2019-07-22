/*
* f_hcsr04.h
*
* Created: 16.06.2017
* Author: Maximilian Starke
*
*	FILE PROGRESS STATE:
*
*/

#ifndef F_HCSR04_H
#define F_HCSR04_H

#include <stdint.h>
#include "f_cmi.h"

namespace sensor {
#if false	
	class HCSR04 {
		public:
		static constexpr int16_t OUT_OF_RANGE {0};
		
		private:
		
		
		// states powered on, powered off,
		public:
		
		/* takes handles to pins */
		HCSR04();
		
		/* measures and returns the distance, returns OUT_OF_RANGE if bad measurement */
		int16_t get_distance();
	};
#endif
	
	// only for namespace backward compatibility:	
	template<typename Metric, uint8_t channels>
	using Kanalysator = analyzer::CMI<Metric,channels>;
	
	
}

#endif /*F_HCSR04_H*/


/*
1->3
2->6
3->1
4->7
5->8
6->2
7->4
8->5
*/