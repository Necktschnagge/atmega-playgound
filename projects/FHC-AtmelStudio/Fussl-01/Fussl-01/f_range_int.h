/*
* f_range_int.h
* Created: 13.03.2018 17:52:31
*/

#ifndef __F_RANGE_INT_H__
#define __F_RANGE_INT_H__
namespace fsl {
	namespace ver_1_0 {
		namespace lg {
			
			/*!
			@author Maximilian Starke
			@date 2019
			@copyright All rights reserved.
			
			@brief Type for Integers in a range {\a base_type_constants::ZERO, ... , \a base_type_constants::MAX}.
			@details Stores a number in {\a base_type_constants::ZERO, ... , \a base_type_constants::MAX} or #OUT_OF_RANGE but no other value.
			@tparam _base_type The base integer type that is used internally. !add description, what this type must bring with it. Must provide a total order with comparison operator < and operator ==. ??It must provide constructors for _base_type(0), _base_type(1), _base_type(-1)
			@tparam _RANGE The maximum value that a #fsl::ver_1_0::lg::range_int can take + 1. Must be positive.
			@tparam _OVERFLOW_RESULTS_IN_OUT_OF_RANGE Defines whether some arithmetic operation (e.g. #operator+) leading to an overflow or underflow will result in #OUT_OF_RANGE or if it will behave like a ring in mathematics.
			@tparam _OUT_OF_RANGE_IS_ABSORBING If it is set to true any arithmetical operation will result in #OUT_OF_RANGE if one of the operands is #OUT_OF_RANGE. Otherwise the result of any arithmetical operation where at least one operand is #OUT_OF_RANGE will be undefined. Set this true only if this feature is really needed, since hat will have bad impact on performance.
			*/
			template <typename _base_type, _base_type _RANGE, bool _OVERFLOW_RESULTS_IN_OUT_OF_RANGE = true, bool _OUT_OF_RANGE_IS_ABSORBING = true>
			class range_int {
				public:
				
				/// @brief \range_int__base_type
				using base_type = _base_type;
				
				/// @brief Contains all constants of type #base_type.
				struct base_type_constants {
					
					/// @brief Zero in base_type.
					static constexpr base_type ZERO{ base_type(0) };
					
					/// @brief One in base_type.
					static constexpr base_type ONE{ base_type(1) };
					
					/// \range_int__RANGE
					static constexpr base_type RANGE{ _RANGE };
					static_assert(ZERO < RANGE, "Invalid template argument: Range limit must be a positive value.");
					
					/// @brief Minimum value that a #fsl::ver_1_0::lg::range_int can take.
					static constexpr base_type MIN{ ZERO };
					
					/// @brief Maximum value that a #fsl::ver_1_0::lg::range_int can take.
					static constexpr base_type MAX{ RANGE - 1 };
					
					/// @brief A dedicated value of type #base_type that is interpreted as out of range.
					static constexpr base_type OUT_OF_RANGE{ base_type(-1) };
					static_assert(!(ZERO < OUT_OF_RANGE && OUT_OF_RANGE < RANGE), "Internal error: The base_type value for OUT_OF_RANGE must not be part of the range.");
				};
				
				/// \range_int__RANGE
				static constexpr base_type RANGE{ base_type_constants::RANGE };
				
				/// @brief Defines whether some arithmetic operation (e.g. #operator+) leading to an overflow will result in #OUT_OF_RANGE or if it will behave like a ring in mathematics. avoid duplicates.
				static constexpr bool OVERFLOW_RESULTS_IN_OUT_OF_RANGE{ _OVERFLOW_RESULTS_IN_OUT_OF_RANGE };
				
				/// @brief Defines whether some operation e.g. #operator+ leading to an overflow will result in #OUT_OF_RANGE or if it will believe like a ring in mathematics. Check hoiw we can avoid duplicates!
				static constexpr bool OVERFLOW_BEHAVIOR_DEFINED_BY_RING_STRUCTURE{ !OVERFLOW_RESULTS_IN_OUT_OF_RANGE };
				static_assert( !OVERFLOW_BEHAVIOR_DEFINED_BY_RING_STRUCTURE || RANGE == base_type_constants::ONE || base_type_constants::MAX < 2 * base_type_constants::MAX, "Invalid template arguments: To operate in ring mode 2 * (RANGE - 1) must not overflow within base_type. Choose a bigger base_type!");
				
				/// @brief If it is set true any arithmetical operation will result in #OUT_OF_RANGE if one of the operands is #OUT_OF_RANGE. Avoid duplicates!!!
				static constexpr bool OUT_OF_RANGE_IS_ABSORBING{ _OUT_OF_RANGE_IS_ABSORBING };
				
				/// @brief A dedicated value of #fsl::ver_1_0::lg::range_int that represents the out of range case.
				static constexpr range_int OUT_OF_RANGE{ base_type_constants::OUT_OF_RANGE };
				
				private:
				/// @brief The internal value.
				base_type value;
				
				inline static constexpr base_type contract_out_of_range_base_type(base_type value){ return value < RANGE ? value : base_type_constants::OUT_OF_RANGE; }
				
				inline void contract_out_of_range(){ value = contract_out_of_range_base_type(value); }
				
#if false // this c-tor should be deleted, addition and subtraction should get own static member functions.
				/* private c-tor for addition and subtraction */
				// only addition and subtraction will be executed after each other max +2 -4 is out of range!
				inline range_int(base_type value, base_type summand, base_type minus) : value(value + summand - minus) { // maybe constexpr ???<<<<<
					if (OUT_OF_RANGE_IS_ABSORBING)
						if (value == base_type_constants::OUT_OF_RANGE || summand == base_type_constants::OUT_OF_RANGE || minus == base_type_constants::OUT_OF_RANGE)
							this->value = base_type_constants::OUT_OF_RANGE;
					
					contract_out_of_range();
					if (OVERFLOW_RESULTS_IN_OUT_OF_RANGE) if (value + summand < value - minus) value = base_type_constants::OUT_OF_RANGE;
					// by the way...     ring behaviour is missing here val +summand > RANGE -> need %-operator.
				}
				
#endif
				
				
				inline static base_type addition(base_type summand0, base_type summand1){
					// restructure as soon as if constexpr is supported. // do this via template specialization!!!
					if (summand0 == base_type_constants::OUT_OF_RANGE) return OUT_OF_RANGE_IS_ABSORBING ? base_type_constants::OUT_OF_RANGE : summand1;
					if (summand1 == base_type_constants::OUT_OF_RANGE) return OUT_OF_RANGE_IS_ABSORBING ? base_type_constants::OUT_OF_RANGE : summand0;
					const base_type raw_sum{ summand0 + summand1 };
					if (OVERFLOW_RESULTS_IN_OUT_OF_RANGE){
						if (summand0 > raw_sum || summand1 > raw_sum) return base_type_constants::OUT_OF_RANGE;
					} else {
						return raw_sum % RANGE;
					}
				}
				
				// define subtraction.
				
				public:
				
				/// @brief Returns true if and only if the given value is element of { \a base_type_constants::ZERO, ... , \a base_type_constants::MAX}
				inline static bool in_range(base_type value){ return base_type_constants::MIN <= value && value <=base_type_constants::MAX; }
				
				/// @brief Constructs a #fsl::ver_1_0::lg::range_int initialized with \a base_type_constants::ZERO.
				inline constexpr range_int(): value(0) {}
				/// @brief Constructs a #fsl::ver_1_0::lg::range_int initialized with given \a value.
				/// @details If not #in_range(\a value) the constructed #fsl::ver_1_0::lg::range_int is equal to #OUT_OF_RANGE.
				inline constexpr range_int(const base_type& value): value(contract_out_of_range_base_type(value)) {}
				/// Copy constructor. Constructs a #fsl::ver_1_0::lg::range_int as a copy of \a another.
				inline constexpr range_int(const range_int& another) : value(another.value) {}
				
				/// @brief Converts \a *this into #base_type
				/// \sa to_base_type().
				inline constexpr operator base_type() const {	return value;	}
				
				/// @brief Converts \a *this into #base_type
				/// \sa @ref fsl::ver_1_0::lg::range_int::operator base_type()
				inline constexpr base_type to_base_type() const {	return value;	}
				
				/// @brief Copy assignment operator.
				inline range_int& operator = (const range_int<base_type,RANGE>& another) { value = another.value; return *this; }
				
				/// Performs
				inline constexpr range_int<base_type,RANGE> operator + (const base_type& rop) const { return addition(value,rop); }
				/// Performs subtraction with a base_type integer
				inline constexpr range_int<base_type,RANGE> operator - (const base_type& rop) const { return range_int<base_type,RANGE>(value,0,rop); }
							
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
}

#endif //__F_RANGE_INT_H__
