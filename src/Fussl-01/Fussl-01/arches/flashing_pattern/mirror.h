/*
 * mirror.h
 *
 * Created: 10.06.2019 11:54:39
 *  Author: Maximilian Starke
 */ 

#ifndef MIRROR_H_
#define MIRROR_H_

#include "flashing_pattern.h"

namespace fsl {
	namespace arc {
		
		/**
			Changes the order of the first count_bit bits of given integer value.
			Returned value will start with the last bit of and end with the first bit of the original value.
			E.g. 001101 will be converted to 101100.
		*/
		template<typename T>
		T mirrored_bits(T value, uint8_t count_bits){
			T result{ 0 };
			constexpr T ONE{ static_cast<T>(1) };
			for (uint8_t pos = 0; pos < count_bits; ++pos)
				result |= static_cast<bool>(value & (ONE << pos)) * (ONE << (count_bits - 1 - pos));
			return result;
			}
		
		/**
			Extends a flashing_pattern class by mirroring the display output.
		*/
		template <class flashing_pattern_component>
		class mirrored : public flashing_pattern_component {
			public:
			using flashing_pattern_component::blink_steps_component;
			
			inline typename flashing_pattern_component::BitSequence display() const override { return mirrored_bits(flashing_pattern_component::display(), flashing_pattern_component::COUNT_BULBS); }
		};
		
		/**
			Behaves in the mirrored way compared to the flashing_pattern which reference it gets.
		*/
		template <typename bit_sequence, bit_sequence count_bulbs>
		class mirror_decorator : public flashing_pattern<bit_sequence, count_bulbs> {
			
			flashing_pattern<bit_sequence, count_bulbs>* component;
			
			public:
			
			inline mirror_decorator(flashing_pattern<bit_sequence, count_bulbs>& component) : component(&component) {}
			
			inline void set_component(flashing_pattern<bit_sequence, count_bulbs>& component){ this->component = &component; }
			
			inline bit_sequence display() const override { return mirrored_bits(component->display(), count_bulbs); }
			
			inline bool finished() const override { return component->finished(); }
			
			inline void operator++() override { return component->operator ++(); }
			
			inline void reset() override { return component->reset(); }
		};
		
	}
}

#endif /* MIRROR_H_ */