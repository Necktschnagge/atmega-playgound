/*
* f_urgency.h
*
* Created: 05.09.2018 22:53:00
* Author: Maximilian Starke
*/


#ifndef F_URGENCY_H_
#define F_URGENCY_H_

#include <stdint.h>

namespace fsl {
	namespace os {
		
			class urgency {
			
				private: /*** private static constexpr functions ***/
			
				//<<<mention that up is less calc intensive than down, if you can choose, use up instead of down.
				inline static constexpr uint8_t unsave_up(uint8_t urgency_inverse){ return static_cast<uint8_t>(static_cast<uint16_t>(urgency_inverse) * 4 / 5); }
			
				inline static constexpr uint8_t up(uint8_t urgency_inverse){ return unsave_up(urgency_inverse) + !(unsave_up(urgency_inverse)); }
				//inline static constexpr uint8_t up(uint8_t urgency_inverse){ urgency_inverse = static_cast<uint16_t>(urgency_inverse) * 4 / 5; return urgency_inverse + !urgency_inverse; }
				//inline static constexpr uint8_t up(uint8_t urgency_inverse){ return static_cast<uint8_t>(static_cast<uint16_t>(urgency_inverse) * 4 / 5 + (urgency_inverse == 1)); }
			
				inline static constexpr uint16_t unsave_down(uint8_t urgency_inverse){ return (static_cast<uint32_t>(urgency_inverse) * (256 + 64) + 255) >> 8; }
			
				inline static constexpr uint8_t down(uint8_t urgency_inverse){ return unsave_down(urgency_inverse) > 255 ? 255 : static_cast<uint8_t>(unsave_down(urgency_inverse)); }
				//inline static constexpr uint8_t down(uint8_t urgency_inverse){ uint32_t urgency_inverse_2 = (static_cast<uint32_t>(urgency_inverse) * (256 + 64) + 255) >> 8; return urgency_inverse = urgency_inverse_2 > 255 ? 255 : static_cast<uint8_t>(urgency_inverse_2); }
			
				template <uint8_t linear_urgency>
				inline static constexpr uint8_t linear_to_inverse(){
					//static_assert(linear_urgency <= 20, "Linear urgency must be a value 0..20 (least important .. most important)");
					return linear_urgency==20 ? 1 : down(linear_to_inverse<linear_urgency + 1>());
				}
			
				/// create named urgencies like standard, lower, higher, low, high, very high, ultra high <<<
				private: /*** private data ***/
			
				/* must not be 0 anytime */
				uint8_t urg_inv;
			
				private: /*** private constexpr ctors ***/
			
				inline constexpr urgency(uint8_t urgency_inverse) : urg_inv(urgency_inverse + !urgency_inverse) {}
			
				public: /*** public static constexpr "values" in function style ***/
			
				inline static constexpr urgency lowest_urgency(){ return urgency(0xFF); }
			
				inline static constexpr urgency very_low_urgency(){ return linear_to_inverse<2>(); }
			
				inline static constexpr urgency low_urgency(){ return linear_to_inverse<4>(); }
			
				inline static constexpr urgency lower_urgency(){ return linear_to_inverse<6>(); }
			
				inline static constexpr urgency normal_urgency(){ return linear_to_inverse<8>(); }
			
				inline static constexpr urgency higher_urgency(){ return linear_to_inverse<10>(); }
			
				inline static constexpr urgency high_urgency(){ return linear_to_inverse<12>(); }
			
				inline static constexpr urgency very_high_urgency(){ return linear_to_inverse<14>(); }
			
				inline static constexpr urgency ultra_high_urgency(){ return linear_to_inverse<16>(); }
			
				inline static constexpr urgency ultra_plus_high_urgency(){ return linear_to_inverse<18>(); }
			
				inline static constexpr urgency highest_urgency(){ return linear_to_inverse<20>(); }
			
				public: /*** public static constexpr functions ***/
			
				inline static constexpr urgency up(urgency urg){ return urgency(up(urg.urg_inv)); }
			
				inline static constexpr urgency down(urgency urg){ return urgency(down(urg.urg_inv)); }
			
				public: /*** public static constexpr factory functions ***/
			
				inline static constexpr urgency create_from_urgency_inverse(uint8_t urgency_inverse){ return urgency(urgency_inverse); }
			
				template <uint8_t linear_value>
				inline static constexpr urgency create_from_linear_at_compiletime(){
					static_assert(linear_value <= 20, "Linear urgency must be a value 0..20 (least important .. most important)");
					return urgency(linear_to_inverse<linear_value>());
				}
			
				static urgency create_from_linear_at_runtime(uint8_t linear_value){ uint8_t urg_inv{ linear_to_inverse<0>() }; for (; linear_value != 0; --linear_value) urg_inv = up(urg_inv); return urgency(urg_inv); }
			
				public: /*** public constexpr c-tors ***/
			
				inline constexpr urgency() : urg_inv(normal_urgency().urg_inv) {}// replace with standard urgency
			
				public: /*** public methods ***/
			
				inline urgency& up(){ urg_inv = up(urg_inv); return *this; }
			
				inline urgency& down(){ urg_inv = down(urg_inv); return *this; }
			
				//<<< warning: do not use.
				inline urgency& operator++(){ urg_inv = urg_inv + !urg_inv - 1; return *this; }
			
				inline urgency& operator--(){ urg_inv = urg_inv - (urg_inv == 0xFF) + 1; return *this;}
			
				inline uint8_t get_inverse_value() const { return urg_inv; }
			
				inline bool operator<(urgency rhs) const { return this->urg_inv > rhs.urg_inv; }
			
				inline bool operator>(urgency rhs) const { return this->urg_inv < rhs.urg_inv; }
			
				inline bool operator<=(urgency rhs) const { return this->urg_inv >= rhs.urg_inv; }
			
				inline bool operator>=(urgency rhs) const { return this->urg_inv <= rhs.urg_inv; }
			
				inline bool operator==(urgency rhs) const { return this->urg_inv == rhs.urg_inv; }
			
				inline bool operator!=(urgency rhs) const { return this->urg_inv != rhs.urg_inv; }
			};

		}
}


#endif /* F_URGENCY_H_ */