/*
 * scheduler.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 
 */

#include "scheduler.h"
#include <avr/interrupt.h>

uint16_t scheduler::divisions_of_second;

ISR (TIMER1_COMPA_vect){
	/* compare match interrupt routine */
	++scheduler::divisions_of_second;
	//### somewhere in update() we need to check  the overflow;
}

uint8_t Time::month_to_int(Time::Month month){
	return static_cast<uint8_t>(month) + 1;
	static_assert(static_cast<uint8_t>(Month::APR) == 3, "Error in month_to_int");
	static_assert(static_cast<uint8_t>(Month::DEC) == 11,"Error");
}

Time::Month Time::int_to_month(uint8_t uint8){
	return static_cast<Time::Month>((uint8 - 1) % 12);
}

Time::Month& operator++(Time::Month& op){
	op = static_cast<Time::Month>(static_cast<uint8_t>(op) + 1 % 12);	
	return op;
}

constexpr bool Time::isLeapYear(int16_t year){
	/*
	if (year % 4)	return false;
	if (year % 100)	return true;
	if (year % 400)	return false;
	return true;
	*/
	return (year % 4) ? false : ((year % 100) ? true : ((year % 400) ? false : true ));
}

uint8_t Time::getMonthLength(Month month, bool isLeapYear){
	if (month == Month::FEB) return 28 + isLeapYear;
	return Time::__monthLength__[static_cast<uint8_t>(month)];
}

Time& Time::operator++(){
	++second;
	if (second == 60){
		second = 0;
		++minute;
	}
	if (minute == 60){
		minute = 0;
		++hour;
	}
	if (hour == 24){
		hour = 0;
		++day;
	}
	if (day == daysOfYear()){
		day = 0;
		++year;
	}
	return *this;
}

Time::date_t Time::getDate() const {
	int16_t rday {day};
	Month rmonth {Month::JAN};
	while (rday >= getMonthLength(rmonth)){
		rday -= getMonthLength(rmonth);
		++rmonth;
	}
	return {rmonth, static_cast<uint8_t>(rday)};
}

Time scheduler::now;

 void scheduler::init(){
	 /* using 16bit Timer1 */
	 
	 //init time
	 
	 now.day = 1;
	 now.year = 2017;
	 now.hour = 0;
	 now.minute = 0;
	 now.second = 0;
	 
	 cli(); // deactivate global interrupts
	 
	 TCCR1A = 0x00; // ~ NO PWM
	 TCNT1 = 0; // counter starts at zero
	 TIMSK |= 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
	 // activate global interrupts !!!
	 OCR1A = 1024; // 1 second divided into 32 equal parts.
	 // register with compare value
	 
	 TCCR1B = 0b00001111;	// CTC (Clear Timer on Compare Match) and start timer running from ext source on,
	 //triggered rising edge
	 // 32768Hz external oscillator for timing clock
	 sei();	// activate global interrupts
 }


