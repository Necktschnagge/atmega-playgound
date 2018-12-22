/* 
* arch_mock.h
*
* Created: 30.11.2018 22:03:42
* Author: F-NET-ADMIN
*/


#ifndef __ARCH_MOCK_H__
#define __ARCH_MOCK_H__

#include <stdint.h>

#include "f_arch.h"
#include "f_ledline.h"
template<typename T>
T reverse_bits(T value, uint8_t count_bits){
	T result{ 0 };
	constexpr T ONE{ 1 };
	for (uint8_t pos = 0; pos < count_bits; ++pos){
		result |= static_cast<bool>(value & (ONE << pos)) * (ONE << (count_bits - 1 - pos));
	}
	return result;
}

class blink_steps {
	public:
	
	/** push current state again to the bulbs */
	virtual void display() const = 0;
	
	/** push the current state to arch and change state to successor state afterwars */
	virtual void operator()() = 0;
	
	/** reset state to the initial state */
	virtual void reset() = 0;
	
	/** return true if and only if the final state, i.e. the successor of the last state which is pushed to arch is reached */
	virtual bool finished() const = 0;
	
	/** perform one arch push update and immediately switch state, return true if and if final state was reached */
	inline bool next(){
		operator()();
		return finished();
		}
	};


template <uint8_t height>
class parallel_blink_up : public blink_steps {
	static constexpr uint8_t HEIGHT{ height };
	static constexpr uint8_t NUMBER_OF_BULBS{ 2*height - 1 };
	static_assert(NUMBER_OF_BULBS < 16, "driver is not ready for more than 15 bits plus one status led.");
	
	// height up to which the bulbs are on.
	uint8_t height_on;
	
	public:
	parallel_blink_up() : height_on(0) {}
	
	void display() const override {
		constexpr uint16_t ALL_ON{ (1<<(NUMBER_OF_BULBS)) - 1 };
		const uint16_t diff_off{ (1u<<(NUMBER_OF_BULBS - height_on)) - 1 };
		const uint16_t upper_side_on{ (ALL_ON & ~diff_off) };
		const uint16_t lower_side_on{ (1u<<height_on) - 1 };
		const uint16_t bulbs{ upper_side_on | lower_side_on };
		arch::pushLineVisible( bulbs );
		//led::clear();
		//led::printInt(height_on);
		//led::printSign('-');
		//led::printInt(bulbs);
		//hardware::delay(2000);
	}
	
	void reset() override { height_on = 0; }
	
	void operator()() override {
		if (finished()) return;
		display();
		++height_on;
	}
	
	inline bool finished() const override {
		return height_on > HEIGHT;
	}
	
	};

template <uint8_t height, bool reverse = false>
class fill_bottle_blink : public blink_steps {
	static constexpr uint8_t HEIGHT{ height };
	static constexpr bool REVERSE{ reverse };
	static constexpr uint8_t BULBS{ HEIGHT*2 - 1 };
	static_assert(HEIGHT <= 8, "driver is not ready for more than 15 bits plus one status led.");
	static_assert(HEIGHT > 0, "logic does not work with empty arch.");
	static constexpr uint16_t FINISHED_STATE{ 0xFFFF };
	
	uint16_t current_state;
	
	inline uint16_t inverse_state() const {
		return reverse_bits<uint16_t>(current_state,BULBS);
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
		return arch::pushLineVisible(reverse_bits<uint16_t>(config,COUNT_BULBS));
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

template<uint8_t bulbs, int16_t pause, bool reverse>
class toggle_run : public blink_steps{
	static constexpr uint8_t BULBS{ bulbs };
	static constexpr int16_t PAUSE{ pause };
	static constexpr bool REVERSE{ reverse };
	
	//## insert static_assert s
	
	int16_t state;
	
	public:
	toggle_run() : state(0){}
	
	void display() const override {
		const uint8_t on{ state > BULBS ? BULBS : state };
		
	}
	
	void reset() override {
		state = 0;
	}
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
			while(! steps.next()){
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
			while(!steps.next()){
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
	
	void display_number(uint8_t number) const {
		arch::pushLineVisible(0);
		hardware::delay(1000);
		for (uint8_t i = 0; i < number; ++i){
			hardware::delay(400);
			arch::pushLineVisible(0b10000);
			hardware::delay(400);
			arch::pushLineVisible(0);
		}
		hardware::delay(1000);
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
		
		exponential_speed_increaser slow_exp_speed{ exponential_speed_increaser(1500,200, 0x100 * 7/10) };
		exponential_speed_increaser fast_exp_speed{ exponential_speed_increaser(333,80, 0x100 *4/5) };
		while(true){
			/*begin of loop marker */
			for (uint8_t i = 0; i < 10; ++i){
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
			fast_exp_speed(positive_shot);
			display_number(7);
			slow_exp_speed(parallel);
			display_number(8);
			fast_exp_speed(negative_shot);
			display_number(9);
			
			/* end of loop marker */
			arch::pushLineVisible(0);
			hardware::delay(3000);
		}
		
	}


}; //arch_mock

#endif //__ARCH_MOCK_H__
