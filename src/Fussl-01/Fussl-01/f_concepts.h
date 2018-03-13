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
	
	class Callable {
	public:
		virtual void operator()() = 0;
	};
	
	class ConstCallable {
	public:
		virtual void operator()() const = 0;
	};
	
	class Flags { // not yet ready. do not use.
	private:
		uint8_t memory;
	public: 
		typedef uint8_t flag_id;
	
	/* c-tors */
		inline Flags(){ memory = 0; }
		inline Flags(uint8_t data) { memory = data; }
			
	/* accessors */
		inline bool get(flag_id bit){ 
			return memory & (static_cast<uint8_t>(1) << bit);
		}
		inline void set_true (flag_id bit){
			memory |= 1 << bit; 
		}
		inline void set_false (flag_id bit){
			memory &= ~(1 << bit);
		}
		
	};
	
	
}



#endif /* F_CONCEPTS_H_ */