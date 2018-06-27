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
	
	
	template<typename return_type,typename exception_type>
	class Exceptional {
		private:
		union _union {
			return_type result;
			exception_type exception;
		};
		
		_union value;
		bool _is_exception;
		
		public:
		/* quasi c-tors */
		inline static Exceptional exception(exception_type exception_value){
			Exceptional exceptional;
			exceptional._is_exception = true;
			exceptional.value = exception_value;
			return exceptional;
		}
		inline static Exceptional result(return_type return_value){
			Exceptional exceptional;
			exceptional._is_exception = false;
			exceptional.value = return_value;
			return exceptional;
		}
		
		/* accessors */
		bool is_exception(){ return _is_exception; }
		
		return_type& get_result(){ return value.result; }
		exception_type& get_exception(){ return value.exception; }
		const return_type& get_result() const { return value.result; }
		const exception_type& get_exception() const { return value.exception; }

	};
}



#endif /* F_CONCEPTS_H_ */