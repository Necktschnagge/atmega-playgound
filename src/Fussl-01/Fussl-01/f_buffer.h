/*
 * f_buffer.h
 *
 * Created: 13.07.2018 03:56:25
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_BUFFER_H_
#define F_BUFFER_H_

#include <stdint.h>

namespace fsl {
	namespace con {
		
		template <class T, uint8_t size>
		class buffer {
			T array[size];
			
			uint8_t read_pos; // read position;
			uint8_t filled; // number of filled cells;
			
			public:
			
			buffer(): read_pos(0), filled(0) {}
				
			inline bool empty(){ return !filled; }
			inline bool full(){ return filled == size; }
				
			inline const T& read(){ if (empty()) { return array[read_pos]; } else { --filled; return array[read_pos++]; } }
			inline bool write(const T& element){ if (full()) return false; else { array[read_pos + (filled++)] = element; } }
		};
	}
}



#endif /* F_BUFFER_H_ */