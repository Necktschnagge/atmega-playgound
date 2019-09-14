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

#include "f_static_task_runner.h"

void f1(){
	bool a;
}
void f2(){
	bool a;
}
void g1(){
	bool a;
}
void g2(){
	bool a;
}

void h1(){
	bool a;
}

void h2(){
	bool a;
}

using c1 = static_task_runner<void(*)(),f1,f2>;
using c2 = static_task_runner<void(*)(),g1,g2>;
using c3 = callback_traits<void(*)()>::static_task_runner<h1,h2>;

using t = concat<c1,c3,c2>;


int main(void)
{
	
	t::run();
	
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
				const char* error_message{ it->cbegin() }; // pint to first character of single error message.
				error_message++;
				DDRB = 0;
			}
		}
	}

	/* ready */
    while (1)
    {
    }
}

