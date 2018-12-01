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



class blink_steps {
	public:
	
	virtual void operator()() = 0;
	
	virtual void reset() = 0;
	
	virtual bool finished() const = 0;
	
	inline bool next(){
		operator()();
		return finished();
		}
	};


template <uint8_t max_height>
class parallel_blink_up : public blink_steps {
	static constexpr uint8_t MAX_HEIGHT{ max_height };
	static_assert(MAX_HEIGHT < 9, "driver is not ready for more than 15 bits plus one status led.");
	
	uint8_t height_on;
	
	public:
	parallel_blink_up() : height_on(0) {}
	
	void reset() override { height_on = 0; }
	
	void operator()() override {
		if (finished()) return;
		constexpr uint16_t ALL_ON{ (1<<(MAX_HEIGHT)) - 1 };
		const uint16_t diff_off{ (1<<(MAX_HEIGHT - height_on)) - 1 };
		const uint16_t upper_side_on{ (ALL_ON & ~diff_off) };
		const uint16_t lower_side_on{ (1<<height_on) - 1 };
		arch::pushLineVisible( upper_side_on | lower_side_on );
		++height_on;
	}
	
	bool finished() const override {
		return height_on > MAX_HEIGHT;
	}
	
	};

template <uint8_t max_height, bool reverse = false>
class fill_bottle_blink : public blink_steps {
	static constexpr uint8_t MAX_HEIGHT{ max_height };
	static_assert(MAX_HEIGHT < 9, "driver is not ready for more than 15 bits plus one status led.");
	static_assert(MAX_HEIGHT > 0, "logic does not work with empty arch.");
	static constexpr uint16_t FINISHED_STATE{ 0xFFFF };
	
	uint16_t current_state;
	
	inline void update_arch(){ arch::pushLineVisible(current_state); }
	
	public:
	fill_bottle_blink() : current_state(0) {}
	
	void reset() override {current_state = 0; }
	
	bool finished() const override { return current_state & (1<<15); }
	
	void operator()() override {
		if (finished()) return;
		update_arch();
		uint8_t position = MAX_HEIGHT * 2 - 2; //start at highest bit.
		// search for first "0":
		while (current_state & (1 << position)){
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


class linear_speed_increaser{
	int16_t start_speed;
	int16_t speed_limit;
	int16_t delta;
	
	public:
	
	linear_speed_increaser(int16_t start_speed, int16_t speed_limit, int16_t delta) : start_speed(start_speed), speed_limit(speed_limit), delta(delta) {}
	
	void reset(int16_t start_speed, int16_t speed_limit, int16_t delta){
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

class arch_mock
{
//functions
public:
	arch_mock(){ arch::init(); }
	
	static constexpr uint8_t HEIGHT{ 5 };
	
	inline void operator()() const /* no-return */ {
		linear_speed_increaser speed{ linear_speed_increaser(2000,400,-200) };
		parallel_blink_up<HEIGHT> parallel{};
		fill_bottle_blink<HEIGHT, false> positive_bottle{};
		fill_bottle_blink<HEIGHT, true> negative_bottle{};
		while(true){
			speed(parallel);
			speed(positive_bottle);
			speed(parallel);
			speed(negative_bottle);
			arch::pushLineVisible(0);
			hardware::delay(10000);
		}
		
	}


}; //arch_mock

#endif //__ARCH_MOCK_H__
