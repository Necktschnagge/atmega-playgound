/*
 * parallel_climber.h
 *
 * Created: 10.06.2019 10:57:52
 *  Author: Maximilian Starke
 */ 

#include "flashing_pattern.h"

#ifndef PARALLEL_CLIMBER_H_
#define PARALLEL_CLIMBER_H_

namespace fsl {
	namespace arc {
		
		/** Describes a flashing pattern where at time t all bulbs up to height t are one. */
		template <uint8_t count_bulbs>
		class parallel_climber : public flashing_pattern<uint16_t, count_bulbs> { // is compliant with the new design
			static_assert(parallel_climber::COUNT_BULBS <= 16, "parallel blink up is not ready for more than 16 bits.");
			static constexpr uint8_t INITIAL_STATE{ 0 };
			
			/** height up to which the bulbs are on */
			uint8_t height_on;
			
			public:
			parallel_climber() : height_on(INITIAL_STATE) {}
			
			uint16_t display() const override {
				constexpr uint16_t ALL_ON{ (1<<(parallel_climber::COUNT_BULBS)) - 1 };
				const uint16_t diff_off{ (1u<<(parallel_climber::COUNT_BULBS - height_on)) - 1 };
				const uint16_t upper_side_on{ (ALL_ON & ~diff_off) };
				const uint16_t lower_side_on{ (1u<<height_on) - 1 };
				const uint16_t bulbs{ upper_side_on | lower_side_on };
				return bulbs;
			}
			
			inline bool finished() const override { return height_on > parallel_climber::HEIGHT; }
			
			virtual void operator++() override{ if(!finished()) ++height_on; }
			
			void reset() override { height_on = INITIAL_STATE; }
		};

	}
}

#endif /* PARALLEL_CLIMBER_H_ */