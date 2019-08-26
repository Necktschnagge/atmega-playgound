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
	f_test_runner test_runner;
	for (int i = 0; i < 2; ++ i){
		DDRB = 7;
		_delay_ms(1);
		DDRB = 0;
		_delay_ms(1);
	}
	
	run_test_f_range_int(test_runner);
	for (int i = 0; i < 2; ++ i){
		DDRB = 7;
		_delay_ms(1);
		DDRB = 0;
		_delay_ms(1);
	}

	/* ready */
    while (1)
    {
    }
}

