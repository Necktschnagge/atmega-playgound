/* 
* scheduler_test.cpp
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/

#include "scheduler_test.h"

void test_scheduler_blink_led(){
	led::LFPrintString("m-1");
	hardware::delay(4000);
	uint8_t log_precision{ 12 };
	
	uint8_t error_code{ 0 };
	
	fsl::os::system_time my_system_time(1L<<15, log_precision, error_code);
	led::LFPrintString("m-2");
	hardware::delay(4000);	
	if (error_code){
		led::clear();
		led::printString("ER-tinit");
		hardware::delay(5000);
		while (true){}
	}
	led::LFPrintString("m-3");
	hardware::delay(4000);
	fsl::os::system_time::link(my_system_time);
	
	bool start_code = fsl::os::system_time::start();
	led::LFPrintString("m-4");
	hardware::delay(4000);
	if (! start_code){
		led::clear();
		led::printString("ERR-time");
		hardware::delay(5000);
		while (true){}
	}
	led::LFPrintString("m-5");
	hardware::delay(4000);
	fsl::os::scheduler<7> test_scheduler(false, WDTO_1S, 0);
	led::LFPrintString("m-6");
	hardware::delay(4000);
	blink_led my_blink_led(test_scheduler, fsl::hw::gpio_pin(fsl::hw::gpio_pin::Port::C, 2));
	
	volatile time::EMT my_blink_led_time = time::EMT::MIN();

	test_scheduler.new_timer(my_blink_led_time , &my_blink_led);
	led::LFPrintString("m-7");
	hardware::delay(4000);	
	test_scheduler.run();
	led::LFPrintString("m-8");
	hardware::delay(4000);	
}

