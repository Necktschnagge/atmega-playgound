/* 
* arch_mock.h
*
* Created: 30.11.2018 22:03:42
* Author: Maximilian Starke
*/


#ifndef __ARCH_MOCK_H__
#define __ARCH_MOCK_H__

#include <stdint.h>

#include "f_arch.h"
#include "f_ledline.h"


/**
 * \brief Changes the order of the first count_bit bits of given integer value. Returned value will start with the last bit of the original and end with the first bit of the original value. E.g. 001101 will be converted to 101100
 * 
 * \param value
 * \param count_bits
 * 
 * \return T
 */
template<typename T>
T mirrored_bits(T value, uint8_t count_bits){
	T result{ 0 };
	constexpr T ONE{ 1 };
	for (uint8_t pos = 0; pos < count_bits; ++pos)
		result |= static_cast<bool>(value & (ONE << pos)) * (ONE << (count_bits - 1 - pos));
	return result;
}


// the first / initial step should always be the one where all lights are off, if such state exists.
template <typename bit_sequence, bit_sequence count_bulbs>
class blink_steps {
	public:
	
	/** push current state again to the bulbs */
	virtual bit_sequence display() const = 0; //## return the bit-sequence instead of already pushing it.
	
	/** return true if and only if the final state, i.e. the successor of the last state which is pushed to arch is reached */
	virtual bool finished() const = 0;
	
	/** push the current state to arch and change state to successor state afterwards */
	// virtual void operator()() = 0; // should not be used anymore , deprecated
	
	/** Changes the state to the successor state */
	virtual void operator++() = 0;
	
	/** reset state to the initial state */
	virtual void reset() = 0;
	
	/** perform one arch push update and immediately switch state, return true if and only if final state was reached */
	inline bool next_operator_bracket(){ //## diese funktion auslagern und dann das display ergebnis an pushLineVisible weitergeben.
		if (finished()) return true;
		display(); // display current state
		operator++(); // calculate next state
		return finished();
		}
	};

template <typename bit_sequence, bit_sequence count_bulbs>
class mirrored_decorator : public blink_steps<bit_sequence> {
	
	blink_steps<bit_sequence>& component;
	
	public:
	mirrored_steps(blink_steps<bit_sequence>& component) : component(component){}
	
	bit_sequence display() const override { return mirrored_bits(component.display(), count_bulbs); }
	
	bool finished() const override { return component.finished(); }
	
	void operator++() override { return ++component; }
	
	void reset() override { return component.reset(); }
};

// mirror via inheritance: ## check if this is working
/*
template <class blink_steps_component>
class mirrored : public blink_steps_component {
	public:
	mirrored(blink_steps_component&& component) : blink_steps_component(component) {}
	
	bit_sequence display() const override { return mirrored_bits(component.display(), count_bulbs); }
};
*/

template <uint8_t count_bulbs>
class parallel_blink_up : public blink_steps<uint16_t, count_bulbs> { // is compliant with the new design
	static constexpr uint8_t COUNT_BULBS{ count_bulbs };
	static constexpr uint8_t HEIGHT{ COUNT_BULBS/2 + 1 };
	static_assert(COUNT_BULBS > 0, "parallel blink up must have at least one bulb.");
	static_assert(COUNT_BULBS <= 16, "parallel blink up is not ready for more than 16 bits.");
	
	/** height up to which the bulbs are on */
	uint8_t height_on;
	
	public:
	parallel_blink_up() : height_on(0) {}
	
	uint16_t display() const override {
		constexpr uint16_t ALL_ON{ (1<<(COUNT_BULBS)) - 1 };
		const uint16_t diff_off{ (1u<<(COUNT_BULBS - height_on)) - 1 };
		const uint16_t upper_side_on{ (ALL_ON & ~diff_off) };
		const uint16_t lower_side_on{ (1u<<height_on) - 1 };
		const uint16_t bulbs{ upper_side_on | lower_side_on };
		return bulbs;
	}
	
	inline bool finished() const override { return height_on > HEIGHT; }
	
	virtual void operator++() override{ if(!finished()) ++height_on; }
	
	void reset() override { height_on = 0; }
	
	};

template <uint8_t height, bool reverse = false>
class fill_bottle_blink : public blink_steps<uint16_t> {
	static constexpr uint8_t HEIGHT{ height };
	static constexpr bool REVERSE{ reverse }; // should not be placed here. should be implemented as decorator pattern.
	static constexpr uint8_t BULBS{ HEIGHT*2 - 1 };
	static_assert(HEIGHT <= 8, "driver is not ready for more than 15 bits plus one status led.");
	static_assert(HEIGHT > 0, "logic does not work with empty arch.");
	static constexpr uint16_t FINISHED_STATE{ 0xFFFF };
	
	uint16_t current_state;
	
	inline uint16_t inverse_state() const {
		return mirrored_bits<uint16_t>(current_state,BULBS);
	}
	
	public:
	fill_bottle_blink() : current_state(0) {}
	
	void display() const override {
		if (REVERSE) arch::pushLineVisible(inverse_state()); else arch::pushLineVisible(current_state);
	}
	
	void reset() override {current_state = 0; }
	
	bool finished() const override { return current_state & 0x8000; }
	
	void operator()() override {
		if (finished()) return;
		display();
		uint8_t position = BULBS - 1; //start at highest bit.
		// search for first "0":
		while (current_state & (1 << position)) /* at position is a 1 */ {
			if (!position) {
				current_state = FINISHED_STATE;
				return;
			}
			--position;
		}
		// position is the first zero when starting at upper bit moving to lower bit.
		// search for the first "1" to move upwards:
		while ((~current_state) & (1<<position)){
			if (!position) {
				// we are at the lower limit / bit 0 and see a "0". insert a new drop into the bottle.
				current_state = current_state | 1;
				return;
			}
			--position;
		}
		// position is at the first one coming from upper going to lower that is not part of the leading 11..1 - section.
		// move the current 1 one bit upwards:
		current_state = (current_state | (1<<(position+1))) /* add the higher bit*/ & /*remove the lower bit*/ ~(1 <<position);
	}
	
	};


template <uint8_t number_of_lights, uint8_t number_of_patches, bool reverse>
class shooting_star_blink : public blink_steps {
	static constexpr uint8_t COUNT_BULBS{ number_of_lights };
	static constexpr uint8_t COUNT_PATCHES{ number_of_patches };
	static constexpr bool REVERSE{ reverse };
	static constexpr uint8_t COUNT_USED_BITS{ COUNT_PATCHES + COUNT_BULBS };
	static constexpr uint64_t SELECT_USED_BITS{ COUNT_USED_BITS == 64 ? 0xFFFFFFFFFFFFFFFF : (1ull<<COUNT_USED_BITS) - 1 };
	static constexpr uint64_t SELECT_BULBS{ SELECT_USED_BITS - (1 << COUNT_PATCHES) + 1 };
	static constexpr uint64_t ONE{ 1 };
	
	static_assert( ((1ull<<32)<<32) - 1 == 0xFFFFFFFFFFFFFFFF, "ALL_USED_BITS_HIGH is not correct if this evaluates false.");
	static_assert(COUNT_PATCHES >= 1, "There must be at least one patch field.");
	static_assert(COUNT_USED_BITS <= 64, "Not enough RAM space in this implementation.");
	static_assert(COUNT_BULBS <= 16, "Arch lib does not support more than 16 bulbs.");
	
	/*
		BIT		0		1		2		3		4		5		6		7		8
				{patch_0, ... ,patch_n}  ...  { bulb_0, ... , bulb_m}  ...  unuseds
	*/
	uint64_t state;
	bool insert_enabled;
	
	public:
	
	shooting_star_blink(): state(1), insert_enabled(false) {}
	
	void display() const override {
		uint16_t config = static_cast<uint16_t>((SELECT_BULBS & state) >> COUNT_PATCHES);
		if (!REVERSE) return arch::pushLineVisible(config);
		return arch::pushLineVisible(mirrored_bits<uint16_t>(config,COUNT_BULBS));
	}
	
	void reset() override { state = 1; insert_enabled = false; }
	
	bool finished() const override { return (state & SELECT_USED_BITS) == SELECT_USED_BITS; }
	
	void operator()() override {
		if (finished()) return;
		display();
		const bool old_lower_bit = state & ONE;
		const bool old_upper_bit = (ONE << (COUNT_USED_BITS - 1)) & state;
		const bool& new_lower_bit_without_insertion = old_upper_bit;
		state <<= 1;
		state &= SELECT_USED_BITS;
		state |= new_lower_bit_without_insertion;
		
		if (insert_enabled){
			if (old_lower_bit && (!new_lower_bit_without_insertion)){
				state |= 1;
				insert_enabled = false;
			}
		} else {
			insert_enabled = true;
		}
	}
};

template<uint8_t bulbs, uint16_t pause, bool reverse>
class toggle_run : public blink_steps{
	static constexpr uint8_t BULBS{ bulbs };
	static constexpr uint16_t PAUSE{ pause };
	static constexpr bool REVERSE{ reverse };
	
	static_assert(BULBS <= 16, "Too many bulbs.");
	static_assert(PAUSE + BULBS < 0x8000, "Too large pause + bulbs. Use smaller pause.");
	static_assert(PAUSE < 0x8000, "Too large pause. Use smaller pause.");
	
	/*
		0: initial, 1: first bulb on, ... , 5: first 5 bulbs on, ... , (BULBS): all bulbs on, ... , (BULBS+PAUSE): all bulbs on, (BULBS+PAUSE+1): first bulb off, ... , (2*BULBS + PAUSE): all bulbs off, ... , (2*(BULBS + PAUSE)): last state,
		
		-1: first bulb off, -2: first 2 bulbs off, ...
	*/
	uint16_t state;
	uint8_t repeatings;
	uint8_t current_repeating;
	
	public:
	toggle_run(uint8_t repeatings) : state(0), repeatings(repeatings), current_repeating(1) {}
	
	void display() const override {
		const uint8_t on{ state > BULBS ? BULBS : static_cast<uint8_t>(state) };
		const uint8_t off{ state > PAUSE + 2* BULBS ? BULBS : (state > PAUSE + BULBS ? static_cast<uint8_t>(state - (PAUSE + BULBS)) : static_cast<uint8_t>(0)) };
		uint16_t bulb_config{ 0 };
		for (uint8_t i = 0; i < BULBS; ++i){
			bulb_config |= (uint16_t(1) << i) * (on >= i) * (off <= i);
		}
		if (REVERSE) bulb_config = mirrored_bits<uint16_t>(bulb_config, BULBS);
		arch::pushLineVisible(bulb_config);
	}
	
	void reset() override {   state = 0; current_repeating = 1;   }
	
	void operator()() override {
		display();
		++state;
		static_assert( 2*(BULBS + PAUSE) < 0xFFFF,"Too large pause.");
		if (state > 2*(BULBS + PAUSE)) { // behind the last state
			if (current_repeating == repeatings) current_repeating = 0; /* finished */ else ++current_repeating;
			state = 0; // sub loop reset to beginning.
		}
	}
	
	inline void set_repeatings(uint8_t repeatings){   this->repeatings = repeatings;   }
	
	bool finished() const override {   return !current_repeating;   }
	
};

//### refactor reverse should not be part of this classes, should be passed to the speed increaser.
// and the display() funciton should only return the bulb config, 
// then operator() should be final and only implemented in abstract class. calling first display() and then operator++ and then return the result of display.
// the incremnention content of operator () should be moved into operator ++.

template<uint8_t bulbs, uint16_t pause, bool reverse>
class pendle : public blink_steps {
	static constexpr uint8_t BULBS{ bulbs };
	static constexpr uint16_t PAUSE{ pause };
	static constexpr bool REVERSE{ reverse };
	static constexpr uint16_t BAP{ PAUSE + BULBS };
	
	static constexpr uint16_t LAST_STATE{ 4*BAP };
	
	static_assert(BULBS <= 16, "Too many bulbs.");
	static_assert(2*BAP < 0x8000, "Too large pause + bulbs. Use smaller pause.");
	static_assert(2*PAUSE < 0x8000, "Too large pause. Use smaller pause.");
	
	uint16_t state;
	uint8_t repeatings;
	uint8_t current_repeating;
	
	public:
	pendle(uint8_t repeatings) : state(0), repeatings(repeatings), current_repeating(1) {}
	
	void display() const override {
		const uint8_t pos_on{ state > BULBS ? BULBS : static_cast<uint8_t>(state) };
		const uint8_t pos_off{ state > BAP + BULBS ? BULBS : (state > BAP ? static_cast<uint8_t>(state - BAP) : static_cast<uint8_t>(0)) };
		const uint8_t neg_on{ state > BAP*2 + BULBS ? BULBS : (state > BAP*2 ? static_cast<uint8_t>(state - BAP*2) : static_cast<uint8_t>(0)) };
		const uint8_t neg_off{ state > BAP*3 + BULBS ? BULBS : (state > BAP*3 ? static_cast<uint8_t>(state - BAP*3) : static_cast<uint8_t>(0)) };
		uint16_t bulb_config{ 0 };
		for (uint8_t i = 0; i < BULBS; ++i){
			bulb_config |= (uint16_t(1) << i) * (((pos_on >= i) && (pos_off <= i)) || ((neg_on >= i) && (neg_off <= i)));
		}
		if (REVERSE) bulb_config = mirrored_bits<uint16_t>(bulb_config, BULBS);
		arch::pushLineVisible(bulb_config);
	}
	
	void reset() override {   state = 0; current_repeating = 1;   }
	
	void operator()() override {
		display();
		++state;
		static_assert( BAP*4 < 0xFFFF,"Too large pause.");
		if (state > LAST_STATE) {
			if (current_repeating == repeatings) current_repeating = 0; /* finished */ else ++current_repeating;
			state = 0; // sub loop reset to beginning.
		}
	}
	
	inline void set_repeatings(uint8_t repeatings){   this->repeatings = repeatings;   }
	
	bool finished() const override {   return !current_repeating;   }
	
};

class linear_speed_increaser{
	int16_t start_speed;
	int16_t speed_limit;
	int16_t delta;
	
	public:
	
	linear_speed_increaser(int16_t start_speed, int16_t speed_limit, int16_t delta) : start_speed(start_speed), speed_limit(speed_limit), delta(delta) {}
	
	inline void reset(int16_t start_speed, int16_t speed_limit, int16_t delta){
		this->start_speed = start_speed;
		this->start_speed = speed_limit;
		this->delta = delta;
	}
	
	void operator()(blink_steps& steps){
		int16_t current_speed{ start_speed };
		while(
			((current_speed < speed_limit) && (delta > 0))
			||
			((current_speed > speed_limit) && (delta < 0))
		){
			steps.reset();
			while(! steps.next_operator_bracket()){
				hardware::delay(current_speed);
			}
			current_speed = current_speed + delta;
		}
	}
};

class exponential_speed_increaser{
	int16_t start_speed;
	int16_t speed_limit;
	int16_t factor_fixed_comma_at_8;
	
	public:
	
	exponential_speed_increaser(int16_t start_speed, int16_t speed_limit, int16_t factor_times_2_up_8): start_speed(start_speed), speed_limit(speed_limit), factor_fixed_comma_at_8(factor_times_2_up_8) {}
	
	inline void reset(int16_t start_speed, int16_t speed_limit, int16_t factor_times_2_up_8){
		this->start_speed = start_speed;
		this->start_speed = speed_limit;
		this->factor_fixed_comma_at_8 = factor_times_2_up_8;
	}
	
	void operator()(blink_steps& steps){
		int16_t current_speed{ start_speed };
		while (
			((current_speed < speed_limit) && (factor_fixed_comma_at_8 > 0x100))
			||
			((current_speed > speed_limit) && (factor_fixed_comma_at_8 < 0x100))
		){
			steps.reset();
			while(!steps.next_operator_bracket()){
				hardware::delay(current_speed);
			}
			hardware::delay(current_speed);
			current_speed = (static_cast<int32_t>(current_speed)*factor_fixed_comma_at_8) >> 8;
		}
	}
	
};

class arch_mock
{
//functions
public:
	arch_mock(){ arch::init(true); }
	
	static constexpr uint8_t HEIGHT{ 5 };
	
	void display_number(uint32_t number) const {
		led::clear();
		led::printInt(number);
		#if false
		arch::pushLineVisible(0);
		hardware::delay(1000);
		for (uint8_t i = 0; i < number; ++i){
			hardware::delay(400);
			arch::pushLineVisible(0b10000);
			hardware::delay(400);
			arch::pushLineVisible(0);
		}
		hardware::delay(1000);
		#endif
	}
	
	inline void hold(const blink_steps& steps){
		steps.display();
		hardware::delay(1000);
	}
	
	inline void operator()() const /* no-return */ {
		//linear_speed_increaser speed{ linear_speed_increaser(2000,400,-200) };
		parallel_blink_up<HEIGHT> parallel{};
		fill_bottle_blink<HEIGHT, false> positive_bottle{};
		fill_bottle_blink<HEIGHT, true> negative_bottle{};
		shooting_star_blink<9,9,false> positive_shot{};
		shooting_star_blink<9,9,true> negative_shot{};
		toggle_run<9,50,false> positive_toggler{ toggle_run<9,50,false>(3) };
		toggle_run<9,50,true> negative_toggler{ toggle_run<9,50,true>(3) };
		pendle<9,30,false> pendler(3);
		
		exponential_speed_increaser slow_exp_speed{ exponential_speed_increaser(1500,200, 0x100 * 7/10) };
		exponential_speed_increaser fast_exp_speed{ exponential_speed_increaser(333,80, 0x100 *4/5) };
		while(true){
			/*begin of loop marker */
			for (uint8_t i = 0; i < 5; ++i){
			arch::pushLineVisible(0b101010101);
			hardware::delay(500);
			arch::pushLineVisible(0b010101010);
			hardware::delay(500);
			}
			
			display_number(1);
			slow_exp_speed(parallel);
			display_number(2);
			fast_exp_speed(positive_bottle);
			display_number(3);
			slow_exp_speed(parallel);
			display_number(4);
			fast_exp_speed(negative_bottle);
			display_number(5);
			slow_exp_speed(parallel);
			display_number(6);
			fast_exp_speed(pendler);
			display_number(6001);
			fast_exp_speed(positive_shot);
			display_number(7);
			slow_exp_speed(parallel);
			display_number(8);
			fast_exp_speed(negative_shot);
			display_number(9);
			fast_exp_speed(positive_toggler);
			display_number(10);
			fast_exp_speed(negative_toggler);
			display_number(11);
			
			/* end of loop marker */
			arch::pushLineVisible(0);
			hardware::delay(3000);
		}
		
	}


}; //arch_mock

#endif //__ARCH_MOCK_H__
