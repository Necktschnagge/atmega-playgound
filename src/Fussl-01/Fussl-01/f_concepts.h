/*
 * f_concepts.h
 *
 * Created: 24.08.2017 20:16:04
 *  Author: Maximilian Starke
 */ 


#ifndef F_CONCEPTS_H_
#define F_CONCEPTS_H_

namespace concepts {
	
	class Callable {
	public:
		virtual void operator()() = 0; 
	};
	
}



#endif /* F_CONCEPTS_H_ */