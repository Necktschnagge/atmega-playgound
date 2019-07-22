/*
* f_exceptional.h
*
* Created: 27.06.2018 16:29:56
* Author: F-NET-ADMIN
*/


#ifndef __F_EXCEPTIONAL_H__
#define __F_EXCEPTIONAL_H__

namespace fsl {
	namespace str {
		
		template<typename return_type, typename exception_type>
		class exceptional {
			private:
			union _union {
				return_type result;
				exception_type exception;
			};
			
			_union value;
			bool _is_exception;
			
			exceptional(){}
			
			public:
			/* quasi c-tors */
			inline static exceptional create_exception(exception_type exception_value){
				exceptional obj;
				obj._is_exception = true;
				obj.value.exception = exception_value;
				return obj;
			}
			inline static exceptional create_result(return_type return_value){
				exceptional obj;
				obj._is_exception = false;
				obj.value.result = return_value;
				return obj;
			}
			
			/* accessors */
			inline bool is_exception(){ return _is_exception; }
			inline bool is_result(){ return !_is_exception; }
			
			inline return_type& result(){ return value.result; }
			inline exception_type& exception(){ return value.exception; }
			inline const return_type& result() const { return value.result; }
			inline const exception_type& exception() const { return value.exception; }
				
			inline operator bool(){ return !_is_exception; }
			inline return_type& operator()() { return result(); }
			inline const return_type& operator()() const { return result(); }
			
		};

	}
}

#endif //__F_EXCEPTIONAL_H__
