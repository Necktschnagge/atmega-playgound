/* 
* scheduler_test.cpp
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/

#include "scheduler_test.h"

void test_scheduler_blink_led(){
	
	uint8_t log_precision{ 12 };
	
	uint8_t error_code{ 0 };
	
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
	
	#warning auskommentiert
	/*
	blink_led my_blink_led(test_scheduler, fsl::hw::gpio_pin(fsl::hw::gpio_pin::Port::C, 2));
	
	volatile time::EMT my_blink_led_time = time::EMT::MIN();

	test_scheduler.new_timer(my_blink_led_time , &my_blink_led);
		
	test_scheduler.run();
	*/	
}

