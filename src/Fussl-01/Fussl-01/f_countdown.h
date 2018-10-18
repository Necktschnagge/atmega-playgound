/*
 * f_countdown.h
 *
 * Created: 06.09.2018 21:24:03
 * Author: Maximilian Starke
*/ 


#ifndef F_COUNTDOWN_H_
#define F_COUNTDOWN_H_

#include "f_order.h"

namespace fsl {
	
	namespace lg {
		
		template <typename integer_type>
		class countdown {
			integer_type _value;
			integer_type _reset;
			
			public:
			using base_type = integer_type;
			
			inline countdown(integer_type reset_value) : _value(reset_value), _reset(reset_value) {} 
			
			inline const integer_type& operator--() volatile { _value = _value + (!_value) - 1; return _value; } 
			inline const integer_type& operator()() volatile { return operator--(); }
			inline const integer_type& count_down() volatile { return operator--(); }
				
			inline bool zero() const volatile { return !_value; }
			inline const integer_type& value() const volatile { return _value; }
			inline const integer_type& get_reset_value() volatile { return _reset; }
			inline void reset() volatile { _value = _reset; }
				
			inline void set_value(integer_type value) volatile { _value = fsl::lg::min(value,_reset); }
			inline void set_reset_value(integer_type reset_value) volatile { _reset = reset_value; _value = fsl::lg::min(_value,_reset); }
			
			inline operator integer_type(){ return _value; }
		};
		
	}
}



#endif /* F_COUNTDOWN_H_ */