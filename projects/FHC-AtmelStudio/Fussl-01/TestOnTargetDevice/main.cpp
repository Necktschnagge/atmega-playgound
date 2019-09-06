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
#include "f_test_runner.h"

int main(void)
{
	f_test_runner test_runner; //0x10ec
	DDRB = 7;
	_delay_ms(1);
	DDRB = 0;
	_delay_ms(1);
	
	run_test_f_range_int(test_runner);



	while (true){
		DDRB = 7;
		_delay_ms(1);
		DDRB = 0;
		_delay_ms(1);
		// "output test results"
		if (test_runner.any_fail()){
			for (auto it = test_runner.errors().cbegin(); it != test_runner.errors().cend(); ++it){
				const char* error_message{ it->cbegin() }; // pint to first character of single error message.
				DDRB = 0;
			}
		}
	}

	/* ready */
    while (1)
    {
    }
}

