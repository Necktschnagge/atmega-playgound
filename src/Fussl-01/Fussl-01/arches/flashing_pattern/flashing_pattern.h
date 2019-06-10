/*
 * flashing_pattern.h
 *
 * Created: 10.06.2019 10:31:21
 *  Author: Maximilian Starke
 */ 


#ifndef FLASHING_PATTERN_H_
#define FLASHING_PATTERN_H_

namespace fsl {
	namespace arc {
		
		/**
			Abstract class to describes a flashing pattern for Christmas arches, that have a certain number of bulbs (count_bulbs).
			For consistency, the initial state should be the state where all bulbs are off. It will be displayed first.
			A flashing pattern should end after a final amount of steps in a final state which should not be displayed by design.
			The final state is the past-the-end of all states before that should be displayed.
			
		*/
		template <typename bit_sequence, bit_sequence count_bulbs>
		class flashing_pattern {
			public:
			
			/** the number of bulbs in the arc */
			static constexpr bit_sequence COUNT_BULBS{ count_bulbs };
			
			/** the height of the middle one [or two] bulb[s], that is considered the height of the arches */
			static constexpr bit_sequence HEIGHT{ (COUNT_BULBS+1)/2 };
			
			/** Returns the bulbs' bit sequence of the current state. If in final state it should display ### */
			virtual bit_sequence display() const = 0;
			
			/** return true if and only if the final state is reached, i.e. the successor of the last state which should be displayed */
			virtual bool finished() const = 0;
			
			/** Changes the state to the successor state */
			virtual void operator++() = 0;
			
			/** reset state to the initial state */
			virtual void reset() = 0;
			
			static_assert(COUNT_BULBS > 0, "An arch must have at least one bulb.");
			
			static_assert(COUNT_BULBS + 1 > 0, "Due to arithmetic reasons (definition of HEIGHT) cound_bulbs must have an ordinary successor.");
			
		};
		
	}
}

#endif /* FLASHING_PATTERN_H_ */