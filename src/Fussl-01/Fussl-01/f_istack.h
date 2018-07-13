/*
 * f_istack.h
 *
 * Created: 13.07.2018 03:01:27
 *  Author: Maximilian Starke
 */ 


#ifndef ISTACK_H_
#define ISTACK_H_

#include <stdint.h>

namespace fsl {
	namespace con {
		
		template <class T, uint8_t size>
		class istack {
			static_assert(size > 0, "Stack has size 0.");
			
			T array[size];
			
			uint8_t next_free_cell; // = number of elements on the stack
			
			public:
			
			istack() : next_free_cell(0){}
			
			inline bool empty(){	return !next_free_cell;			}
			inline bool full(){		return next_free_cell == size;	}
				
			inline T top() const {	return empty() ? T() : array[next_free_cell-1];	}			
			inline void drop(){	if (!empty()) --next_free_cell;		}
			inline T pop (){	T element = top();	drop();	return element;	}
			inline void push(const T& element){	if (!full()) array[next_free_cell++] = element;	}	
			
		};
	}
}



#endif /* ISTACK_H_ */