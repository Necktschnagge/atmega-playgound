/* 
* scheduler_test.cpp
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/

#include "scheduler_test.h"

void test_system_time(){
	
	//uint8_t log_precision{ 12 };
	
	uint8_t error_code{ 0 };
	
	//fsl::os::system_time my_system_time(1L<<15, log_precision, error_code);
	
	if (error_code){
		led::clear();
		led::printString("ER-tinit");
		hardware::delay(5000);
	}
	
	//fsl::os::system_time::link(my_system_time);
	
	//bool start_code = fsl::os::system_time::start();
	
	TCNT1 = 0;
	//TCCR1B &= ~0b00000111;
	TCCR1B = 0b00000111;
	while(true){
		//time::EMT before = my_system_time();
		//led::printInt(before.get_in_seconds());
		led::printInt(TCNT1);
		hardware::delay(500);
		led::clear();	
		led::printInt(TCNT1);
		led::printDotsOnly(0xFF);
		hardware::delay(500);
		led::clear();
		
	}
	
}

