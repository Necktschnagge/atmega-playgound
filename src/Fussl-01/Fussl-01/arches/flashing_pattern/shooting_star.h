/*
* shooting_star.h
*
* Created: 10.06.2019 19:52:21
*  Author: Maximilian Starke
*/


#ifndef SHOOTING_STAR_H_
#define SHOOTING_STAR_H_

namespace fsl {
	namespace arc {
		
		/**
			Describes a flashing pattern where a growing section of bulbs turned on are moving from one side of the arches to the other.
			Only works with bit_sequence uint16_t.
		*/
		template <uint8_t count_bulbs, uint8_t count_patches>
		class shooting_star : public flashing_pattern<uint16_t, count_bulbs> { // is compliant with new design
			static constexpr uint8_t COUNT_PATCHES{ count_patches };
			static_assert(COUNT_PATCHES >= 1, "There must be at least one patch bit.");
			static constexpr uint8_t COUNT_USED_BITS{ COUNT_PATCHES + shooting_star::COUNT_BULBS };
			static_assert(COUNT_USED_BITS <= 64, "Not enough RAM space allocated in this implementation.");
			static constexpr uint64_t PATTERN_SELECT_USED_BITS{ (1ull<<COUNT_USED_BITS) - 1 };
			static_assert(((1ull << 32) << 32) - 1 == 0xFFFFFFFFFFFFFFFF, "Arithmetic error: PATTERN_SELECT_USED_BITS has bad behavior."); // two times 32 because of compiler warnings.
			static constexpr uint64_t PATTERN_SELECT_PATCHES{ (1 << COUNT_PATCHES) + 1 };
			static constexpr uint64_t PATTERN_SELECT_BULBS{ PATTERN_SELECT_USED_BITS - PATTERN_SELECT_PATCHES };
			
			static constexpr uint64_t ONE{ 1ull };
			static constexpr uint64_t PATTERN_INSERT_POSITION{ ONE << (COUNT_PATCHES - 1) };
			static constexpr uint64_t INITIAL_STATE{ PATTERN_INSERT_POSITION };
			static constexpr bool INITIAL_INSERT_ENABLED{ false };
			/*
			BIT		0		1		2		3		4		5		6		7		8
			{patch_0, ... ,patch_n}  ...  { bulb_0, ... , bulb_m}  ...  unuseds
			*/
			uint64_t state;
			bool insert_enabled;
			
			public:
			
			shooting_star(): state(INITIAL_STATE), insert_enabled(INITIAL_INSERT_ENABLED) {}
			
			uint16_t display() const override { return static_cast<uint16_t>((PATTERN_SELECT_BULBS & state) >> COUNT_PATCHES); }
			
			bool finished() const override { return (state & PATTERN_SELECT_USED_BITS) == PATTERN_SELECT_USED_BITS; }
			
			void operator++() override {
				// circular shift:
				const bool OLD_UPPER_BIT = state & (ONE << (COUNT_USED_BITS - 1));
				const bool& NEW_LOWER_BIT = OLD_UPPER_BIT;
				state <<= 1;
				state &= PATTERN_SELECT_USED_BITS;
				state |= NEW_LOWER_BIT;
				
				// already done if insert is temporary disabled:
				if (!insert_enabled){
					// ignore insert for this time because there was an insert in the last step
					insert_enabled = true;
					return;
				}
				
				// do insert on condition:
				if ((!(state & PATTERN_INSERT_POSITION)) /* (insert position = last patch bit) has a "0". */
				&& (state & (PATTERN_INSERT_POSITION << 1)) /* behind insert position = first bulb bit = insert position of last step has a "1". */
				) /* if insert position has switched from "1" to "0" */ {
					state |= PATTERN_INSERT_POSITION;
					insert_enabled = false; // do not insert in the next step.
				}
			}
			
			void reset() override { state = INITIAL_STATE; insert_enabled = INITIAL_INSERT_ENABLED; }
			
		};
	}
}

#endif /* SHOOTING_STAR_H_ */