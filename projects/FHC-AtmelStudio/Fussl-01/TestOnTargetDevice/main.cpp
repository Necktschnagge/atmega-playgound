/*
 * TestOnTargetDevice.cpp
 *
 * Created: 28.07.2019 19:44:04
 * Author : F-NET-ADMIN
 */ 

#include <avr/io.h>
#define F_CPU 100'000
#include <util/delay.h>
#include "test_f_range_int.h"
#include "f_test_logger.h"


int main(void)
{
	f_test_logger test_logger;
	
	run_test_f_range_int(test_logger);

	while (true){
		DDRB = 7;
		_delay_ms(1);
		DDRB = 0;
		_delay_ms(1);
		// "output test results"
		if (test_logger.any_fail()){
			for (auto it = test_logger.errors().cbegin(); it != test_logger.errors().cend(); ++it){
				const char* const error_message{ it->cbegin() }; // point to first character of single error message.
				const char* cc{ error_message + 1 };
			}
		}
	}
	
	/* ready */
    while (1)
    {
    }
}
