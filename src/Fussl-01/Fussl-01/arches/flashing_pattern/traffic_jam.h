/*
 * traffic_jam.h
 *
 * Created: 10.06.2019 19:08:29
 *  Author: Maximilian Starke
 */ 


#ifndef TRAFFIC_JAM_H_
#define TRAFFIC_JAM_H_

#include "flashing_pattern.h"

namespace fsl {
	namespace arc {
		
		/**
			Describes a flashing pattern where single light drops move from one side to another and fill the whole arches step by step.
			You can imagine this as a road where there was an accident at the end and one car after another comes and starts waiting in a growing queue.
		*/
		template <typename bit_sequence, uint8_t count_bulbs>
		class traffic_jam;
		
		template <uint8_t count_bulbs>
		class traffic_jam<uint16_t, count_bulbs> : public flashing_pattern<uint16_t, count_bulbs> {
			static constexpr uint16_t FINISHED_STATE{ 0xFFFF };
			static constexpr uint16_t INITIAL_STATE{ 0 };
			static_assert(traffic_jam::COUNT_BULBS <= 16, "traffic jam is not ready for more than 16 bits.");
			
			uint16_t current_state;
			
			public:
			traffic_jam() : current_state(INITIAL_STATE) {}
			
			typename traffic_jam<uint16_t, count_bulbs>::BitSequence display() const override { return current_state & /* only bits of interest */ ((1<<traffic_jam::COUNT_BULBS) - 1); }
			
			bool finished() const override { return current_state == FINISHED_STATE; }
			
			void operator++() override {
				uint8_t position = traffic_jam::COUNT_BULBS - 1; //start at highest bit.
				// search for first "0":
				for(; current_state & (1 << position); --position) /* at position there is a 1 */ {
					if (!position) { // There is no "0" at all.
						current_state = FINISHED_STATE;
						return;
					}
				}
				// position is the first zero when starting at upper bit moving to lower bit.
				// search for the first "1" to move upwards:
				for (; (~current_state) & (1<<position); --position){
					if (!position) {
						// we are at the lower limit / bit 0 and see a "0". insert a new water drop into the bottle.
						current_state = current_state | 1;
						return;
					}
				}
				// position is at the first "1" you see, if coming from upper going to lower bit, that is not part of the leading 11..1 - section.
				// move the current 1 one bit upwards:
				current_state = (current_state | (1<<(position+1))) /* add the higher bit*/ & /*remove the lower bit*/ ~(1 <<position);
			}
			
			void reset() override {current_state = INITIAL_STATE; }
		};
	
	}
}
#endif /* TRAFFIC_JAM_H_ */