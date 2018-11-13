/* 
* scheduler_test.cpp
*
* Created: 18.10.2018 19:40:30
* Author: F-NET-ADMIN
*/

#include "scheduler_test.h"

void test_system_time(){
	
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
	while(true){
		time::EMT before = my_system_time();
		led::printInt(before.get_in_seconds());
		hardware::delay(500);
		led::clear();	
	}
	
}

