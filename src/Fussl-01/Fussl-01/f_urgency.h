/*
* f_urgency.h
*
* Created: 05.09.2018 22:53:00
* Author: Maximilian Starke



provide some build artifacts for inline static constexpr funnstions.: Function table for such mappings.
use a separat task and task runner for this?
make tests for this pupose too, note: static asserts to const functions seem to not work with avr-gcc.
However trying the same code with MSVC++ erverything's fine.
so just build tests and visualization tables with another compiler.
*/


#ifndef F_URGENCY_H_
#define F_URGENCY_H_

#include <stdint.h>

namespace fsl {
	namespace os {
		
		class urgency;
		
		class urgency_helper {
			
			public:
			urgency_helper() = delete;
			
			
			private: /*** private static constexpr functions ***/
			
//			urgency_helper(){}
			
			/*
			 calculates (unsafe) the next lower inverse urgency to given inverse urgency, i.e. a higher urgency, by using a step factor (*4/5) in an unsave way:
			 Attention: unsafe_up maps 1 to 0. 0 is no valid inverse urgency. Use up(..) for safeness.
			 Note: unsave_up() is supposed to be less resource consumptive than unsave_down(). If you can choose, prefer unsave_up().
			*/
			inline static constexpr uint8_t unsafe_up(uint8_t urgency_inverse){ return static_cast<uint8_t>(static_cast<uint16_t>(urgency_inverse) * 4 / 5); }
			
			/*
			 calculates (safe) the next lower inverse urgency to given inverse urgency, i.e. a higher urgency.
			 The lowest inverse urgency (= highest urgency = 1) is fix point. 0 is mapped to 1.
			 Note: up() is supposed to be less resource consumptive than down(). If you can choose, prefer up().
			*/
			inline static constexpr uint8_t up(uint8_t urgency_inverse){ return unsafe_up(urgency_inverse) + !(unsafe_up(urgency_inverse)); }
			
			/*
			 calculates (unsafe) the next higher inverse urgency to given inverse urgency, i.e. a lower urgency, by multiplying (5/4) and rounding up.
			 Attention: unsafe_down maps 204 to 255 and x >= 255 to y > 255, which is no valid urgency. Use down(..) for safeness.
			 Note: unsafe_down is supposed to be more resource consumptive than unsave_up due to heavier round upwards procedures.
			*/
			inline static constexpr uint16_t unsafe_down(uint8_t urgency_inverse){ return (static_cast<uint32_t>(urgency_inverse) * (256 + 64) + 255) >> 8; }
			
			/*
			 calculates (safe) the next higher inverse urgency to given inverse urgency, i.e. a lower urgency.
			 Urgencies >203 will be mapped to 255. For instance 255 is a fix point.
			 Attention: 0 is mapped to 0. Note that 0 is no valid inverse urgency.
			 Note: down() is supposed to be more resource consumptive than up(). If you can choose, prefer up().
			*/
			inline static constexpr uint8_t down(uint8_t urgency_inverse){ return unsafe_down(urgency_inverse) > 255 ? 255 : static_cast<uint8_t>(unsafe_down(urgency_inverse)); }
			public:
			
			template <uint8_t linear_urgency>
			static constexpr uint8_t linear_to_inverse(){
				static_assert(linear_urgency < 20, "Linear urgency must be a value 0..20 (least important .. most important)");
				return down(linear_to_inverse<linear_urgency + 1>());
				///### better use up?; here it is compile time, so not important#####
			}

			template <uint8_t linear_urgency>
			static constexpr uint8_t linear_to_inverse_via_up(){
				static_assert(linear_urgency < 20, "Linear urgency must be a value 0..20 (least important .. most important)");
				return up(linear_to_inverse_via_up<linear_urgency - 1>());
				///### better use up?; here it is compile time, so not important#####
			}
			
			inline static constexpr uint8_t pull_0_to_1(uint8_t x){ return x + !x; }
			
			friend class urgency;
		};
		
		template <>
		constexpr uint8_t urgency_helper::linear_to_inverse<20>(){ return 1; }
		template <>
		constexpr uint8_t urgency_helper::linear_to_inverse_via_up<0>(){ return 232; }
		
		static_assert(urgency_helper::linear_to_inverse<0>() == 232, "Value not appropiate.");
		///### check that all up and all down created values are the same!!!!s
		
		constexpr uint8_t x0 = urgency_helper::linear_to_inverse<0>();
		
		class urgency {
			
			private: /*** private static constexpr functions ***/
			
			private: /*** private data ***/
			
			/* must not be 0 anytime,
			1 means most important,
			255 means least important,
			inverse urgency is the amount of scheduling steps a job needs to wait until he is scheduled since last time.
			
			urg_inv		STEP		0	1	2	3	4	5	6	7	8	9	10	11	12
				2		TASK A		x		x		x		x		x		x		x
				3		TASK B		x			x			x			x			x
				4		TASK C		x				x				x				x
			one possible run order: A, B, C; A; B; A, C; A, B; A, C; B; A; A, B, C;
			Note that you have another possible run order if it differs from the one above only in swapping As,Bs and Cs with no ; between.
			*/
			uint8_t urg_inv;
			
			private: /*** private constexpr ctors ***/
			
			/* constructs an urgency containing given urgency_inverse, but 1 if given 0. */
			inline constexpr urgency(uint8_t urgency_inverse) : urg_inv(urgency_inverse + !urgency_inverse) {}
			
			public: /*** public static constexpr "values" in function style ***/
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency lowest_urgency(){ return urgency(0xFF); } /// <<<< better to use linear_to_inv of 0? what is it?
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency very_low_urgency(){ return urgency_helper::linear_to_inverse<2>(); }
//			static constexpr urgency very_low_urgency{_urgency_helper::linear_to_inverse<2>(); }; // why is this not working?
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency low_urgency(){ return urgency_helper::linear_to_inverse<4>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency lower_urgency(){ return urgency_helper::linear_to_inverse<6>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency normal_urgency(){ return urgency_helper::linear_to_inverse<8>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency higher_urgency(){ return urgency_helper::linear_to_inverse<10>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency high_urgency(){ return urgency_helper::linear_to_inverse<12>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency very_high_urgency(){ return urgency_helper::linear_to_inverse<14>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency ultra_high_urgency(){ return urgency_helper::linear_to_inverse<16>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency ultra_plus_high_urgency(){ return urgency_helper::linear_to_inverse<18>(); }
			
			/* lowest_urgency() < very_low_urgency() < low_urgency() < lower_urgency() < normal_urgency() < higher_urgency() < high_urgency() < very_high_urgency() < ultra_high_urgency() < ultra_plus_high_urgency() < highest_urgency() */
			inline static constexpr urgency highest_urgency(){ return urgency_helper::linear_to_inverse<20>(); }
			
			
			public: /*** public static constexpr functions ***/
			
			inline static constexpr urgency up(urgency urg){ return urgency(up(urg.urg_inv)); }
			
			inline static constexpr urgency down(urgency urg){ return urgency(down(urg.urg_inv)); }
			
			public: /*** public static constexpr factory functions ***/
			
			inline static constexpr urgency create_from_urgency_inverse(uint8_t urgency_inverse){ return urgency(urgency_inverse); }
			
			template <uint8_t linear_value>
			inline static constexpr urgency create_from_linear_at_compiletime(){
				static_assert(linear_value <= 20, "Linear urgency must be a value 0..20 (least important .. most important)");
				return urgency(urgency_helper::linear_to_inverse<linear_value>());
			}
			
			static urgency create_from_linear_at_runtime(uint8_t linear_value){ uint8_t urg_inv{ urgency_helper::linear_to_inverse<0>() }; for (; linear_value != 0; --linear_value) urg_inv = urgency_helper::up(urg_inv); return urgency(urg_inv); }
			
			/* */
			template<uint8_t upper_bound, uint8_t linear_value>
			inline static constexpr uint8_t create_from_linear_range_at_compiletime(){ return 0; }
			#warning here
			
			public: /*** public constexpr c-tors ***/
			
				inline constexpr urgency() : urg_inv(normal_urgency().urg_inv) {}// replace with standard urgency
			
				public: /*** public methods ***/
			
				inline urgency& up(){ urg_inv = urgency_helper::up(urg_inv); return *this; }
				
				inline void up() volatile { urg_inv = urgency_helper::up(urg_inv); }
				
				inline urgency& down(){ urg_inv = urgency_helper::down(urg_inv); return *this; }
				
				inline void down() volatile { /*urg_inv = down(urg_inv);*/ }//####
				
#if 0
				/* deprecated. use up and down */
				inline urgency& operator++(){ urg_inv = urg_inv + !urg_inv - 1; return *this; }
				
				inline void operator++() volatile { urg_inv = urg_inv + !urg_inv - 1; }
				
				inline urgency& operator--(){ urg_inv = urg_inv - (urg_inv == 0xFF) + 1; return *this; }
				
				inline void operator--() volatile { urg_inv = urg_inv - (urg_inv == 0xFF) + 1; }
#endif
				
				inline uint8_t inverse_value() const volatile { return urg_inv; }	
				
				inline urgency& operator=(urgency rhs) { this->urg_inv = rhs.urg_inv; return *this;}
				
				inline void operator=(urgency rhs) volatile { this->urg_inv = rhs.urg_inv; }
				
				inline bool operator<(urgency rhs) const volatile { return this->urg_inv > rhs.urg_inv; }
			
				inline bool operator>(urgency rhs) const volatile { return this->urg_inv < rhs.urg_inv; }
			
				inline bool operator<=(urgency rhs) const volatile { return this->urg_inv >= rhs.urg_inv; }
			
				inline bool operator>=(urgency rhs) const volatile { return this->urg_inv <= rhs.urg_inv; }
			
				inline bool operator==(urgency rhs) const volatile { return this->urg_inv == rhs.urg_inv; }
			
				inline bool operator!=(urgency rhs) const volatile { return this->urg_inv != rhs.urg_inv; }
		};
		
	}
}


#endif /* F_URGENCY_H_ */