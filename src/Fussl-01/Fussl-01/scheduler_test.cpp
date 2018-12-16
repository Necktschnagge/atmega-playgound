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
	
	//TCNT0 = 0;
	//TCCR1B &= ~0b00000111;
	//TCCR1B = 0b00000111;
	ASSR |= 0x08; // may cause corruption of other registers
	TCCR0 = 0x07;
	TCNT0 = 0;
	
	bool on{ 0 };
	while(true){
		uint8_t value = TCNT0;
		led::clear();
		led::printInt(value / 32);
		led::printSign('-');
		if (value % 32 < 10) led::printSign('0');
		led::printInt(value % 32);
		led::printDotsOnly(0xFF * on);
		on = !on;
		hardware::delay(400);
	}
	
}

