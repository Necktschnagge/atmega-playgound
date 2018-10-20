/*
 * f_callbacks.h
 *
 * Created: 27.06.2018 16:44:54
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_CALLBACKS_H_
#define F_CALLBACKS_H_

namespace fsl {
	namespace str {
		
		using void_function = void (*)();
		
		class callable {
			public:
			virtual void operator()() = 0;
		};
		
		class const_callable {
			public:
			virtual void operator()() const = 0;
		};
		
		template <typename callable_type>
		class union_callback {
			private:
			union intern_union {
				fsl::str::void_function function_ptr;
				callable_type* callable_ptr;
				
				intern_union(): function_ptr(nullptr){}
			};
			intern_union _union;
			
			public:
			
			union_callback() {}
			
			union_callback(const volatile union_callback& rhs){ _union.function_ptr = rhs._union.function_ptr; }
			
			volatile union_callback<callable_type>& operator = (const volatile union_callback<callable_type>& rhs) volatile { _union.function_ptr = rhs._union.function_ptr; return *this; }
			
			/* set a function pointer, replaces the previously stored function pointer oder callable pointer */
			inline void set_function_ptr(fsl::str::void_function ptr) volatile { _union.function_ptr = ptr; }
			/* set a callable pointer, replaces the previously stored function pointer oder callable pointer */
			inline void set_callable_ptr(callable_type* ptr) volatile { _union.callable_ptr = ptr; }
			
			/* treat intern stored callback as of type "void_function" and call if non-nullptr */
			inline void call_function() const { if (_union.function_ptr != nullptr) _union.function_ptr(); }
			/* treat intern stored callback as of type "callable" and call if non-nullptr */
			inline void call_callable() const { if (_union.callable_ptr != nullptr) (*_union.callable_ptr)(); }
				
			inline void* void_ptr(){ return _union.function_ptr; }
		};
		
		using standard_union_callback = union_callback<callable>;
	}
}



#endif /* F_CALLBACKS_H_ */