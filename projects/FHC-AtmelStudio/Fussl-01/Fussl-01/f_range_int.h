/**
@file  f_range_int.h
@author Maximilian Starke
@date 2019
@copyright All rights reserved. For any license please contact the author.
*/

#ifndef __F_RANGE_INT_H__
#define __F_RANGE_INT_H__

#include <stdint.h>

namespace fsl {
	namespace ver_1_0 {
		namespace lg {
			
			/*!
			@author Maximilian Starke
			
			@brief Type for Integers in a range {\a base_type_constants::ZERO, ... , \a base_type_constants::MAX}.
			@details Stores a number in {\a base_type_constants::ZERO, ... , \a base_type_constants::MAX} or #OUT_OF_RANGE but no other value. Note that #OUT_OF_RANGE is absorbing: Any operation will result in #OUT_OF_RANGE if one of the operands is #OUT_OF_RANGE. Any overflowing or underflowing operation will also result in #OUT_OF_RANGE.
			@tparam _base_type The base integer type that is used internally. It must provide a total order over operator< and operator == and must be constructible from int8_t. It must provide binary operator + and binary operator - for addition and subtraction as well as unary operator - for additive inversion.
			@tparam _RANGE The maximum value that a #fsl::ver_1_0::lg::range_int can take + 1. Must be positive.
			*/
			template <typename _base_type, _base_type _RANGE>
			class range_int {
				public:
				
				/// @brief The base integer type that is used internally.
				using base_type = _base_type;
				
				/// @brief Contains all constants of type #base_type.
				struct base_type_constants {
					
					/// @brief Zero in base_type.
					static constexpr base_type ZERO{ base_type(static_cast<int8_t>(0)) };
					
					/// @brief One in base_type.
					static constexpr base_type ONE{ base_type(static_cast<int8_t>(1)) };
					
					/// @brief The maximum value that a #fsl::ver_1_0::lg::range_int can take + 1.
					/// Number of pairwise distinct values that the #fsl::ver_1_0::lg::range_int can take, not counting #OUT_OF_RANGE.
					static constexpr base_type RANGE{ _RANGE };
					static_assert(ZERO < RANGE, "Invalid template argument: Range limit must be a positive value.");
					
					/// @brief Minimum value that a #fsl::ver_1_0::lg::range_int can take.
					static constexpr base_type MIN{ ZERO };
					
					/// @brief Maximum value that a #fsl::ver_1_0::lg::range_int can take.
					static constexpr base_type MAX{ RANGE - ONE };
					
					/// @brief A dedicated value of type #base_type that is interpreted as out of range.
					static constexpr base_type OUT_OF_RANGE{ base_type(static_cast<int8_t>(-1)) };
				};
				
				/// @brief The maximum value that a #fsl::ver_1_0::lg::range_int can take + 1.
				/// Number of pairwise distinct values that the #fsl::ver_1_0::lg::range_int can take, not counting #OUT_OF_RANGE.
				static constexpr base_type RANGE{ base_type_constants::RANGE };
				
				/// @brief A dedicated value of #fsl::ver_1_0::lg::range_int that represents the out of range case.
				static constexpr range_int OUT_OF_RANGE{ base_type_constants::OUT_OF_RANGE };
				
				/// @brief Returns true if and only if the given value is element of { \a base_type_constants::ZERO, ... , \a base_type_constants::MAX}
				inline static constexpr bool in_range(const base_type& value){ return base_type_constants::MIN == value || (base_type_constants::MIN < value && value < base_type_constants::RANGE); }
				static_assert(!in_range(base_type_constants::OUT_OF_RANGE), "Internal error: The base_type value for OUT_OF_RANGE must not be part of the range.");
				static_assert(!(base_type_constants::RANGE < base_type_constants::MIN), "Invalid template argument: Range must not be negative.");
				
				private:
				/// @brief The internal value. Must be #in_range or #OUT_OF_RANGE.
				base_type value;
				
				/// @brief The identity on all #base_type values that are #in_range. Maps all other values to #OUT_OF_RANGE.
				inline static constexpr base_type contract_out_of_range_base_type(const base_type& value){ return in_range(value) ? value : base_type_constants::OUT_OF_RANGE; }
				
				/// @brief Adds two #base_type values, as defined by supposed behavior for type #fsl::ver_1_0::lg::range_int.
				/// @details Any overflow will result in #OUT_OF_RANGE. If any operand is #OUT_OF_RANGE the result will be #OUT_OF_RANGE.
				/// @param summand0 must be #in_range or #OUT_OF_RANGE. Otherwise the result is undefined.
				/// @param summand1 must be #in_range or #OUT_OF_RANGE. Otherwise the result is undefined.
				inline static base_type addition(const base_type& summand0, const base_type& summand1){
					if (summand0 == base_type_constants::OUT_OF_RANGE) return base_type_constants::OUT_OF_RANGE;
					if (summand1 == base_type_constants::OUT_OF_RANGE) return base_type_constants::OUT_OF_RANGE;
					if (base_type_constants::MAX - summand0 < summand1) return base_type_constants::OUT_OF_RANGE; //check overflow
					return summand0 + summand1;
				}
				
				/// @brief Subtracts subtrahend from minuend, as defined by supposed behavior for type #fsl::ver_1_0::lg::range_int.
				/// @details Any underflow will result in #OUT_OF_RANGE. If any operand is #OUT_OF_RANGE the result will be #OUT_OF_RANGE.
				/// @param minuend must be #in_range or #OUT_OF_RANGE. Otherwise the result is undefined.
				/// @param subtrahend must be #in_range or #OUT_OF_RANGE. Otherwise the result is undefined.
				inline static base_type subtraction(const base_type& minuend, const base_type& subtrahend){
					if (minuend == base_type_constants::OUT_OF_RANGE) return base_type_constants::OUT_OF_RANGE;
					if (subtrahend == base_type_constants::OUT_OF_RANGE) return base_type_constants::OUT_OF_RANGE;
					if (minuend < subtrahend) return base_type_constants::OUT_OF_RANGE; //check underflow
					return minuend - subtrahend;
				}
				
				/// @brief Constructs a #fsl::ver_1_0::lg::range_int initialized with given \a value without checking for #OUT_OF_RANGE.
				/// @details The \a value must be #in_range or #base_type_constants::OUT_OF_RANGE. Otherwise the result will be undefined.
				inline constexpr range_int(const base_type& value, decltype(nullptr) _ignore_) : value(value){}
				
				public:
				
				/// @brief Constructs a #fsl::ver_1_0::lg::range_int initialized with \a base_type_constants::ZERO.
				inline constexpr range_int(): value(0) {}
				
				/// @brief Constructs a #fsl::ver_1_0::lg::range_int initialized with given \a value.
				/// @details If not #in_range(\a value) the constructed #fsl::ver_1_0::lg::range_int is equal to #OUT_OF_RANGE.
				inline constexpr range_int(const base_type& value): value(contract_out_of_range_base_type(value)) {}
				
				/// Copy constructor. Constructs a #fsl::ver_1_0::lg::range_int as a copy of \a another.
				inline constexpr range_int(const range_int& another) : value(another.value) {}
				
				/// @brief Converts \a *this into #base_type
				/// \sa to_base_type().
				inline constexpr operator base_type() const { return value; }
				
				/// @brief Converts \a *this into #base_type
				/// \sa @ref fsl::ver_1_0::lg::range_int::operator base_type()
				inline constexpr base_type to_base_type() const { return value; }
				
				/// @brief Conversion assignment operator.
				inline range_int& operator = (const base_type& value){ this->value = contract_out_of_range_base_type(value); }
				
				/// @brief Copy assignment operator.
				inline range_int& operator = (const range_int& another) { value = another.value; return *this; }
				
				/// @brief Performs addition with a #base_type integer.
				/// @details In case of an overflow or underflow the result will be #OUT_OF_RANGE.
				inline constexpr range_int operator + (const base_type& rop) const {
					if (rop < 0) return range_int(subtraction(value,contract_out_of_range_base_type(-rop)), nullptr); // use unchecked constructor
					return range_int(addition(value,contract_out_of_range_base_type(rop)), nullptr); // use unchecked constructor
				}
				
				/// @brief Performs subtraction with a #base_type integer.
				/// @details In case of an overflow or underflow the result will be #OUT_OF_RANGE.
				inline constexpr range_int operator - (const base_type& rop) const {
					if (rop < 0) return range_int(addition(value, contract_out_of_range_base_type(-rop)), nullptr); // use unchecked constructor
					return range_int(subtraction(value, contract_out_of_range_base_type(rop)), nullptr); // use unchecked constructor
				}
				
				/// @brief Performs addition with another #fsl::ver_1_0::lg::range_int.
				/// @details In case of an overflow or underflow the result will be #OUT_OF_RANGE.
				inline constexpr range_int operator + (const range_int& rop) const {
					return range_int(addition(value, rop.to_base_type()), nullptr); // use unchecked constructor
				}
				
				/// @brief Performs subtraction with another #fsl::ver_1_0::lg::range_int.
				/// @details In case of an overflow or underflow the result will be #OUT_OF_RANGE.
				inline constexpr range_int operator - (const range_int& rop) const {
					return range_int(subtraction(value, rop.to_base_type()), nullptr); // use unchecked constructor
				}
				
				/// @brief Performs addition of a #base_type integer.
				inline constexpr range_int& operator += (const base_type& rop) { return *this = *this + rop; }
				
				/// @brief Performs subtraction of a #base_type integer.
				inline constexpr range_int& operator -= (const base_type& rop) { return *this = *this - rop; }
				
				/// @brief Performs a pre-increment.
				inline range_int& operator ++ (){ return *this = this->to_base_type() + 1; }
				
				/// @brief Performs a post-increment.
				inline range_int operator ++ (int){ range_int copy{ *this }; this->operator ++(); return copy; }
				
				/// @brief Performs a pre-decrement.
				inline range_int& operator -- (){
					return *this = this->to_base_type() - 1;
					/*
						well defined since
							- for signed types the first value out of range in negative direction, that is -1, indeed exists.
							- for unsigned types the first value out of range is per standard guaranteed to be 2^N - 1:
								- since unsigned int types support correct modulo behavior and
								- 2^N - 1 must therefore equal the maximum representable value,
								- and therefore is greater than or equal to RANGE, which is OUT_OF_RANGE
					*/
					}
				
				/// @brief Performs a post-decrement.
				inline range_int operator -- (int){ range_int copy{ *this }; this->operator --(); return copy; }
			};
		}
	}
}

#endif //__F_RANGE_INT_H__
