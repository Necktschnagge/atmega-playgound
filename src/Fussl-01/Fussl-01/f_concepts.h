/*
* f_concepts.h
*
* Created: 24.08.2017 20:16:04
*  Author: Maximilian Starke
*/


#ifndef F_CONCEPTS_H_
#define F_CONCEPTS_H_

#include <stdint.h>

namespace concepts {
	
	using void_function = void (*)();
	
	class Callable {
		public:
		virtual void operator()() = 0;
	};
	
	class ConstCallable {
		public:
		virtual void operator()() const = 0;
	};
	
	
}



#endif /* F_CONCEPTS_H_ */