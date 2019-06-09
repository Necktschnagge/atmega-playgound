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


//## todos here: the base class blink_steps got a changed design, all inheriting classes and using components need to be adopted.
// then, put classes into the arches directory snd split it where suitable.

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
	
	static constexpr bit_sequence COUNT_BULBS{ count_bulbs };
	
	static constexpr bit_sequence HEIGHT{ (COUNT_BULBS+1)/2 }; // actually works for even AND odd numbers of bulbs!!!
	
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
	
	static_assert(COUNT_BULBS > 0, "An arch must have at least one bulb.");
	static_assert(COUNT_BULBS + 1 > 0, "Due to arithmetic reasons (definition of HEIGHT) cound_bulbs must have an ordinary successor.");
	
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
template <class blink_steps_component>
class mirrored : public blink_steps_component {
	public:
	using blink_steps_component::blink_steps_component;
	
	bit_sequence display() const override { return mirrored_bits(component.display(), count_bulbs); }
};

template <uint8_t count_bulbs>
class parallel_blink_up : public blink_steps<uint16_t, count_bulbs> { // is compliant with the new design
	static_assert(COUNT_BULBS <= 16, "parallel blink up is not ready for more than 16 bits.");
	static constexpr uint8_t INITIAL_STATE{ 0 };
	
	/** height up to which the bulbs are on */
	uint8_t height_on;
	
	public:
	parallel_blink_up() : height_on(INITIAL_STATE) {}
	
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
	
	void reset() override { height_on = INITIAL_STATE; }
};

template <uint8_t COUNT_BULBS>
class fill_bottle_blink : public blink_steps<uint16_t, COUNT_BULBS> { // is compliant with new design
	static constexpr uint16_t FINISHED_STATE{ 0xFFFF };
	static constexpr uint16_t INITIAL_STATE{ 0 };
	static_assert(COUNT_BULBS <= 16, "fill bottle is not ready for more than 16 bits.");
	
	uint16_t current_state;
	
	public:
	fill_bottle_blink() : current_state(INITIAL_STATE) {}
	
	bit_sequence display() const override { return current_state & /* only bits of interest */ ((1<<COUNT_BULBS) - 1); }
	
	bool finished() const override { return current_state == FINISHED_STATE; }
	
	void operator++() override {
		uint8_t position = COUNT_BULBS - 1; //start at highest bit.
		// search for first "0":
		for(; current_state & (1 << position); --position;) /* at position there is a 1 */ {
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


template <uint8_t count_bulbs, uint8_t count_patches>
class shooting_star_blink : public blink_steps<uint16_t, count_bulbs> { // is compliant with new design
	static constexpr uint8_t COUNT_PATCHES{ count_patches };
	static_assert(COUNT_PATCHES >= 1, "There must be at least one patch bit.");
	static constexpr uint8_t COUNT_USED_BITS{ COUNT_PATCHES + COUNT_BULBS };
	static_assert(COUNT_USED_BITS <= 64, "Not enough RAM space allocated in this implementation.");
	static constexpr uint64_t PATTERN_SELECT_USED_BITS{ (1ull<<COUNT_USED_BITS) - 1 };
	static_assert((1ull << 64) - 1 == 0xFFFFFFFFFFFFFFFF, "Arithmetic error: PATTERN_SELECT_USED_BITS has bad behavior.");
	// oder version: why was it 32 32 and not once 64?: static_assert( ((1ull<<32)<<32) - 1 == 0xFFFFFFFFFFFFFFFF, "ALL_USED_BITS_HIGH is not correct if this evaluates false.");
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
	
	shooting_star_blink(): state(INITIAL_STATE), insert_enabled(INITIAL_INSERT_ENABLED) {}
	
	void display() const override { return static_cast<uint16_t>((PATTERN_SELECT_BULBS & state) >> COUNT_PATCHES); }
	
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

template<uint8_t count_bulbs, uint16_t pause, bool one_direction = false>
class toggle_run : public blink_steps<uint16_t,count_bulbs>{ // is compliant with new design
	static constexpr uint16_t PAUSE{ pause };
	static_assert(COUNT_BULBS <= 16, "Too many bulbs for this implementation (bit_sequence type is uint16_t).");
	static_assert(PAUSE + COUNT_BULBS< 0x8000, "Too large pause + bulbs. Use smaller pause.");
	static_assert(one_direction || (2*(PAUSE + COUNT_BULBS) < 0x8000), "For using both directions, pause must be smaller.")
	
	static constexpr uint16_t INITIAL_STATE{ 0 };
	static constexpr uint8_t INITIAL_CURRENT_REPEATING{ 1 };
	
	/*
		0: initial, 1: first bulb on, ... , 5: first 5 bulbs on, ... , (BULBS): all bulbs on, ... , (BULBS+PAUSE): all bulbs on, (BULBS+PAUSE+1): first bulb off, ... , (2*BULBS + PAUSE): all bulbs off, ... , (2*(BULBS + PAUSE)): behind the last state,
		
		-1: first bulb off, -2: first 2 bulbs off, ...
	
	
	all changes from left (LSB) to right (MSB):
	
	phase 1: bulbs switching on
	phase 2: bulbs staying turned on
	phase 3: bulbs switching off
	phase 4: bulbs staying switched off
	
	*/
	uint16_t state;
	uint8_t repeatings;
	uint8_t current_repeating;
	
	inline void check_and_correct_repeatings(){ if ((repeatings == 0) || (repeatings & 0x80)) repeatings = 1;}
	
	inline bool going_backwards(){ return !one_direction && !(repeatings % 2); }
	
	public:
	toggle_run(int8_t repeatings) : state(0), repeatings(repeatings), current_repeating(1) { check_and_correct_repeatings(); }
	
	bit_sequence display() const override {
		const bool IN_PHASE_1{ state < COUNT_BULBS };
		// const bool IN_PHASE_2{ (COUNT_BULBS <= state) && (state < COUNT_BULBS + PAUSE) }; // unused
		const bool IN_PHASE_3{ (COUNT_BULBS + PAUSE <= state) && (state < 2*COUNT_BULBS + PAUSE) };
		const bool IN_PHASE_4{ (2*COUNT_BULBS + PAUSE < state) && (state < 2*(COUNT_BULBS + PAUSE)) };
		
		// how many already got switched on (counted, even if they got switch off again afterwards):
		const uint8_t on{
			IN_PHASE_1 ? static_cast<uint8_t>(state) // in phase 1: as many as already turned on in this phase
			: COUNT_BULBS // after phase 1 all were switched on
		};
		// how many already got switched off (of course, some time after they got switched on):
		const uint8_t off{
			IN_PHASE_4 ? COUNT_BULBS // in phase 4 all were already switched off
			: IN_PHASE_3 ? static_cast<uint8_t>(state - (PAUSE + COUNT_BULBS)) // in phase 3: as many as already turned off in this phase
			: 0 // otherwise, i.e. in phase 1 or 2 no bulb has ever been switched off.
		};
		
		uint16_t bulb_config{ 0 };
		for (uint8_t i = 0; i < COUNT_BULBS; ++i){
			bulb_config |= (uint16_t(1) << i) * (i < on) * (i >= off); // set a bit "1" if enough bulbs were turned on, and not enough were turned off
		}
		return bulb_config;
	}
	
	bool finished() const override { return !current_repeating; }
	
	void operator++() override {
/*		if (one_direction){
			++state;
			if (state >= 2*(COUNT_BULBS + PAUSE)) { // behind the last state
				if (current_repeating == repeatings) current_repeating = 0; /* finished  else ++current_repeating;
				state = INITIAL_STATE; // sub loop reset to beginning.
			}
			return;
		}
		*/ //can be deleted, it is just to keep this, if the implementation is not fine, we can look at this older implementation
			
		if (going_backwards()) --state; else ++state;
		if (!(state % (2*(COUNT_BULBS + PAUSE)))){ // behind the last state or reached the first state
			if (current_repeating == repeatings * (one_direction ? 1 : 2)) current_repeating = 0; /* finished */ else ++current_repeating;
			if (!going_backwards()) state = INITIAL_STATE; // sub loop -> reset to beginning.
		}
		
	}
	
	void reset() override { state = INITIAL_STATE; current_repeating = INITIAL_CURRENT_REPEATING; }
	
	inline void set_repeatings(uint8_t repeatings){ this->repeatings = repeatings; check_and_correct_repeatings(); }
	
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
