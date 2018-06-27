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
		
		template<typename return_type,typename exception_type>
		class exceptional {
			private:
			union _union {
				return_type result;
				exception_type exception;
			};
			
			_union value;
			bool _is_exception;
			
			public:
			/* quasi c-tors */
			inline static exceptional exception(exception_type exception_value){
				exceptional exceptional;
				exceptional._is_exception = true;
				exceptional.value = exception_value;
				return exceptional;
			}
			inline static exceptional result(return_type return_value){
				exceptional exceptional;
				exceptional._is_exception = false;
				exceptional.value = return_value;
				return exceptional;
			}
			
			/* accessors */
			inline bool is_exception(){ return _is_exception; }
			
			inline return_type& get_result(){ return value.result; }
			inline exception_type& get_exception(){ return value.exception; }
			inline const return_type& get_result() const { return value.result; }
			inline const exception_type& get_exception() const { return value.exception; }
		};

	}
}

#endif //__F_EXCEPTIONAL_H__
