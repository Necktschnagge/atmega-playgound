/*
 * scheduler.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 
 */

#include "scheduler.h"

int8_t scheduler::divisions_of_second;

ISR (TIMER1_COMPA_vect){
	/* compare match interrupt routine */
	
}

bool scheduler::Time::isLeapYear() const {
	if (year % 4)	return false;
	if (year % 100)	return true;
	if (year % 400)	return false;
	return true;
}

scheduler::Time scheduler::now();

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
 
 
