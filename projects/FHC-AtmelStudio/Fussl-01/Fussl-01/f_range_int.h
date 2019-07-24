/*
* f_range_int.h
*
* Created: 13.03.2018 17:52:31
* @author Maximilian Starke

FPS:

Things should be ready but make comments!!!! and go through everything once

*/


#ifndef __F_RANGE_INT_H__
#define __F_RANGE_INT_H__
namespace fsl {
	namespace lg {
		
		/// @brief Type for Integers in a range {0, ... , RANGE}
		/// @details Stores a number x in {0, ... , RANGE} or #OUT_OF_RANGE.
		/// @tparam _base_type The integer type that is used internally to store the number.
		/// @tparam OVERFLOW_RESULTS_IN_OUT_OF_RANGE Defines whether some operation like #operator+ leading to an overflow will result in #OUT_OF_RANGE or if it will believe like a ring in mathematics.
		template <typename _base_type, _base_type RANGE, bool OVERFLOW_RESULTS_IN_OUT_OF_RANGE = true, bool OUT_OF_RANGE_IS_ABSORBING = true>
		class range_int {
			public:
			
			/// The base integer type that is used internally.
			using base_type = _base_type;
			
			/// A dedicated value of type #base_type that is interpreted as out of range.
			static constexpr base_type OUT_OF_RANGE{ static_cast<_base_type>(-1) }; // better #include <limits> // but not possible for AVR
			
			//static constexpr range_int OUT_OF_RANGE()??
			
			private:
			_base_type value;
			
			inline static constexpr _base_type in_range(_base_type value){ return value < RANGE ? value : OUT_OF_RANGE; }
			
			inline void check_out_of_range(){
				if (!(value < RANGE)){
					value = OUT_OF_RANGE;
				}
			}
			
			/* private c-tor for addition and subtraction */
			inline range_int(_base_type value, _base_type plus, _base_type minus) : value(value + plus - minus) { // maybe constexpr ???<<<<<
				if (OUT_OF_RANGE_IS_ABSORBING) if (value == OUT_OF_RANGE) this->value = OUT_OF_RANGE;
				check_out_of_range();
				if (OVERFLOW_RESULTS_IN_OUT_OF_RANGE) if (value + plus < value - minus) value = OUT_OF_RANGE;
			}
			
			public:
			
			/// Constructs a new #fsl::lg::range_int initialized with 0.
			inline constexpr range_int(): value(0) {}
			/// Constructs a new #fsl::lg::range_int initialized with given value.
			/// @details If the given value is out of range, the constructed #fsl::lg::range_int will be ... .
			inline constexpr range_int(const _base_type& value): value(in_range(value)) {}
			/// Copy constructor. Constructs a new #fsl::lg::range_int as a copy of \a rhs.
			inline constexpr range_int(const range_int& rhs) : value(rhs.value) {}
			/// Copy constructor. Constructs a new #fsl::lg::range_int as a copy of \a rhs.
			inline range_int(const volatile range_int& rhs) : value(rhs.value) {}
			
			/// Converts *this into #base_type
			/// \sa to_base_type().
			inline constexpr operator _base_type() const {	return value;	}
			/// Converts *this into #base_type
			/// \sa to_base_type().
			inline operator _base_type() const volatile {	return value;	}
			
			/// Converts *this into #base_type
			/// \sa @ref fsl::lg::range_int::operator _base_type()
			inline constexpr base_type to_base_type() const {	return value;	}
			/// Converts *this into #base_type
			/// \sa @ref fsl::lg::range_int::operator _base_type()
			inline _base_type to_base_type() const volatile {	return value;	}
			
			inline range_int<_base_type,RANGE>& operator = (const range_int<_base_type,RANGE>& another) { value = another.value; return *this; }
			inline void operator = (const range_int<_base_type,RANGE>& another) volatile { value = another.value; }
			inline range_int<_base_type,RANGE>& operator = (const volatile range_int<_base_type,RANGE>& another) { value = another.value; return *this; }
			inline void operator = (const volatile range_int<_base_type,RANGE>& another) volatile { value = another.value; }
			
			/// Performs
			inline constexpr range_int<_base_type,RANGE> operator + (const _base_type& rop) const { return range_int<_base_type,RANGE>(value,rop,0); }
			/// Performs subtraction with a base_type integer
			inline constexpr range_int<_base_type,RANGE> operator - (const _base_type& rop) const { return range_int<_base_type,RANGE>(value,0,rop); }
						
			/// Performs addition of a #base_type integer.
			inline constexpr range_int<base_type,RANGE>& operator += (base_type rop) { return *this = *this + rop; }
			/// Performs subtraction of a #base_type integer.
			inline constexpr range_int<base_type,RANGE>& operator -= (base_type rop) { return *this = *this - rop; }
			
			/// Performs a pre-increment on the value.
			inline range_int<base_type,RANGE>& operator ++ (){ return *this = static_cast<base_type>(*this) + 1; }
			/// Performs a post-increment on the value.
			inline range_int<base_type,RANGE>& operator ++ (int){ range_int<base_type,RANGE> copy{ *this }; *this = static_cast<base_type>(*this) + 1; return copy; }
			/// Performs a pre-decrement on the value.
			inline range_int<base_type,RANGE>& operator -- (){ return *this = static_cast<base_type>(*this) - 1; }
			/// Performs a post-decrement on the value.
			inline range_int<base_type,RANGE>& operator -- (int){ range_int<base_type,RANGE> copy{ *this }; *this = static_cast<base_type>(*this) - 1; return copy; }

		};
	}
}

#endif //__F_RANGE_INT_H__
