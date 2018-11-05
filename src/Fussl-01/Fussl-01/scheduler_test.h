/* 
* scheduler_test.h
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/


#ifndef __SCHEDULER_TEST_H__
#define __SCHEDULER_TEST_H__

#include "f_scheduler.h"
#include "f_ledline.h"
#include "f_iopin.h"
#include "f_hardware.h"
#include "f_time.h"
#include "f_system_time.h"

using scheduler_type = fsl::os::scheduler<7>;

class blink_led : public fsl::str::callable {
	
	scheduler_type* p_test_scheduler;
	
	fsl::hw::gpio_pin led_pin;

	public:
	
	inline blink_led(scheduler_type& s, fsl::hw::gpio_pin pin) : p_test_scheduler(&s), led_pin(pin) {
		led_pin.set_as_output();
	}
	
	inline bool state(){ return led_pin; };
	
	virtual void operator()() override {
		led_pin = !led_pin;
		const scheduler_type::handle_type MY_HANDLE{ p_test_scheduler->my_handle() };
		const volatile time::EMT* expired{ p_test_scheduler->get_event_time(MY_HANDLE) };
		if (expired == nullptr) {
			//#report error;
			return;
		}
		p_test_scheduler->set_event_time(MY_HANDLE, *expired + time::ExtendedMetricTime::seconds_to_EMT(1));
		p_test_scheduler->set_entry_enable(MY_HANDLE,true);
	}
	
};

class screen_watch : public fsl::str::callable {
	time::ExtendedMetricTime last_screen_update;
	public:
	inline screen_watch() : last_screen_update(time::EMetricTime::MIN()) {}
	
	virtual void operator()() override {
		time::ExtendedMetricTime now = fsl::os::system_time::get_instance().get_now_time();
		if (last_screen_update + time::ExtendedMetricTime::seconds_to_EMT(1) < now){
			last_screen_update = last_screen_update + time::ExtendedMetricTime::seconds_to_EMT(1);
			long double all_seconds = now.get_in_seconds();
			uint32_t all_seconds_int = ceil(all_seconds);
			led::clear();
			uint8_t length = led::printInt(all_seconds_int / 60);
			if (length <= 5){
				led::printSign('.');
				led::printInt(all_seconds_int % 60);
			} else if (length <= 8 ){
				// just display the minutes
			} else {
				led::LFPrintString("OVERFLOW");
			}
			// display now
		}
	}
	
	};

void test_scheduler_blink_led();

#endif //__SCHEDULER_TEST_H__
