/*
* f_range_int.h
*
* Created: 13.03.2018 17:52:31
* Author: F-NET-ADMIN
*/


#ifndef __F_RANGE_INT_H__
#define __F_RANGE_INT_H__

template <typename _base_type, _base_type RANGE, bool OVERFLOW_RESULTS_IN_OUT_OF_RANGE = true>
class range_int {
	public:

	using base_type = _base_type;

	static constexpr _base_type OUT_OF_RANGE{ static_cast<_base_type>(-1) }; // better #include <limits> // but not possible for AVR
	
	private:
	_base_type value;
	
	inline void constexpr check_out_of_range(){
		if (!(value < RANGE)){
			value = OUT_OF_RANGE;
		}
	}
	
	/* private c-tor for addition and subtraction */
	inline range_int(_base_type value, _base_type plus, _base_type minus) : value(value + plus - minus) { // maybe consgtexpr ???
		check_out_of_range();
		if (OVERFLOW_RESULTS_IN_OUT_OF_RANGE) if (value + plus < value - minus) value = OUT_OF_RANGE;
	}
	
	public:
	
	/* c-tors */
	inline constexpr range_int(): value(0) {}
	inline constexpr range_int(_base_type value): value(value) { check_out_of_range(); }
	
	/* conversion operator */
	inline constexpr operator _base_type() const {	return value;	}
	inline constexpr _base_type to_base_type() const {	return value;	}
	
	/* copy = operator */
	inline constexpr range_int<_base_type,RANGE>& operator = (const range_int<_base_type,RANGE>& another) { value = another.value; return *this; }
	
	/* modification operators */
	inline range_int<_base_type,RANGE> operator + (const _base_type& rop) const { return range_int<_base_type,RANGE>(value,rop,0); }
	inline range_int<_base_type,RANGE> operator - (const _base_type& rop) const { return range_int<_base_type,RANGE>(value,0,rop); }
	
	inline range_int<_base_type,RANGE>& operator += (_base_type rop) { return *this = *this + rop; }
	inline range_int<_base_type,RANGE>& operator -= (_base_type rop) { return *this = *this - rop; }
	
	inline range_int<_base_type,RANGE>& operator ++ (){ return *this = static_cast<_base_type>(*this) + 1; }
	inline range_int<_base_type,RANGE>& operator ++ (int){ range_int<_base_type,RANGE> copy{ *this }; *this = static_cast<_base_type>(*this) + 1; return copy; }
	inline range_int<_base_type,RANGE>& operator -- (){ return *this = static_cast<_base_type>(*this) - 1; }
	inline range_int<_base_type,RANGE>& operator -- (int){ range_int<_base_type,RANGE> copy{ *this }; *this = static_cast<_base_type>(*this) - 1; return copy; }

};

#endif //__F_RANGE_INT_H__
