/* 
* scheduler_test.cpp
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/

#include <avr/wdt.h>
#include "f_scheduler.h"
#include "scheduler_test.h"
#include "f_ledline.h"
#include "f_iopin.h"
#include "f_hardware.h"


using scheduler_type = fsl::os::scheduler<7>;

class blink_led : public fsl::str::callable {
	
	bool led_state {false};
	fsl::hw::gpio_pin led_pin = fsl::hw::gpio_pin(fsl::hw::gpio_pin::Port::C, 0);
	scheduler_type* p_test_scheduler;
	
	public:
	
	blink_led(scheduler_type& s) : p_test_scheduler(&s) {
		led_pin.set_as_output();
	}
	
	void operator()() override {
		led_state = !led_state;
		led_pin.write_output(led_state);
		scheduler_type::handle_type MY_HANDLE { p_test_scheduler->my_handle()};
		p_test_scheduler->set_event_time(MY_HANDLE, fsl::os::system_time::get_instance().get_now_time() + time::ExtendedMetricTime::seconds_to_EMT(1));
		p_test_scheduler->set_entry_enable(MY_HANDLE,true);
	}
	
};



void test(){
	uint8_t log_precision {12};
	uint8_t error_code {0};
	
	fsl::os::system_time my_system_time(1L<<15, log_precision, error_code);
	if (error_code){
		led::clear();
		led::printString("ER-tinit");
		hardware::delay(5000);
	}
	

	fsl::os::system_time::link(my_system_time);
	bool start_code = fsl::os::system_time::start();
	if (! start_code){
		led::clear();
		led::printString("ERR-time");
		hardware::delay(5000);
	}
	
	static fsl::os::scheduler<7> test_scheduler(false, WDTO_1S, 0);
	
	
	blink_led my_blink_led(test_scheduler);
	
	volatile time::EMT my_blink_led_time = time::EMT::MIN();

	test_scheduler.new_timer(my_blink_led_time , &my_blink_led);
	/*	
	
	test_scheduler.run();
	*/
}

