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

void test_scheduler_blink_led();

#endif //__SCHEDULER_TEST_H__
