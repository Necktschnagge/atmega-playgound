/*
* f_range_int.h
*
* Created: 13.03.2018 17:52:31
* Author: F-NET-ADMIN
*/


#ifndef __F_RANGE_INT_H__
#define __F_RANGE_INT_H__

template <typename base_type, base_type RANGE, bool OVERFLOW_RESULTS_IN_OUT_OF_RANGE = true>
class range_int {
	public:
	static constexpr base_type OUT_OF_RANGE{ static_cast<base_type>(-1) }; // better #include <limits> // but not possible for AVR
		
	private:
	base_type value;
	
	inline void check_out_of_range(){
		if (!(value < RANGE)){
			value = OUT_OF_RANGE;	
		}
	}
	
	/* private c-tor for addition and subtraction */
	inline range_int(base_type value, base_type plus, base_type minus) : value(value + plus - minus) {
		check_out_of_range();
		if (OVERFLOW_RESULTS_IN_OUT_OF_RANGE) if (value + plus < value - minus) value = OUT_OF_RANGE;
	}
	
	public:
	
	/* c-tors */
	inline range_int(): value(0) {}
	inline range_int(base_type value): value(value) { check_out_of_range(); }
	
	/* conversion operator */
	inline operator base_type(){	return value;	}
		
	/* copy = operator */
	inline range_int<base_type,RANGE>& operator = (const range_int<base_type,RANGE>& another) { value = another.value; }
		
	/* modification operators */
	inline range_int<base_type,RANGE> operator + (const base_type& rop) const { return range_int<base_type,RANGE>(value,rop,0); }
	inline range_int<base_type,RANGE> operator - (const base_type& rop) const { return range_int<base_type,RANGE>(value,0,rop); }
	
	inline range_int<base_type,RANGE>& operator += (base_type rop) { return *this = *this + rop; }
	inline range_int<base_type,RANGE>& operator -= (base_type rop) { return *this = *this - rop; }
		
	inline range_int<base_type,RANGE>& operator ++ (){ return *this = *this + 1; }
	inline range_int<base_type,RANGE>& operator ++ (int){ range_int<base_type,RANGE> copy{ *this }; *this = *this + 1; return copy; }
	inline range_int<base_type,RANGE>& operator -- (){ return *this = *this - 1; }
	inline range_int<base_type,RANGE>& operator -- (int){ range_int<base_type,RANGE> copy{ *this }; *this = *this - 1; return copy; }

};

#endif //__F_RANGE_INT_H__
