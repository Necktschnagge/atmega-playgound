/*
 * f_stack.h
 *
 * Created: 13.07.2018 02:50:10
 *  Author: Maximilian Starke
 */ 


#ifndef F_STACK_H_
#define F_STACK_H_

namespace fsl {
	namespace con {
		
		template <class T, uint8_t size>
		class stack {
			static_assert(size > 0, "Stack has size 0.");
			
			T array[size];
			
			public:
			
			inline const T& ctop() const {	return array[0];	}
			
			inline T& top(){	return array[0];	}
			
			inline void drop(){	for(uint8_t i = 0; i < size - 1; ++i) array[i] = array[i+1];	}
			
			inline T pop (){	T element = ctop();	drop();	return element;	}
			
			inline void push(const T& element){	for (uint8_t i = size - 1; i != 0; --i) array[i] = array[i-1];	array[0] = element;	}
			
		};
	}
}



#endif /* F_STACK_H_ */