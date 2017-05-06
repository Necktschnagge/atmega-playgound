/*
 * scheduler.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 
 */

#include "scheduler.h"
#include <avr/interrupt.h>

using namespace time;

ISR (TIMER1_COMPA_vect){
	/* compare match interrupt routine */
	++scheduler::divisions_of_second;
	//### somewhere in update() we need to check  the overflow;
	if (scheduler::divisions_of_second > 0xF000){
		/* scheduler is not fast enough */
		scheduler::updateNowTime();
		//### check that we don't have missed some interrupts yet.
	}
}

uint16_t scheduler::divisions_of_second {0};
HumanTime scheduler::now;
uint16_t scheduler::nextFreeHandle {1};
void* scheduler::taskTable {nullptr};
uint16_t scheduler::taskTableSize {0};

 void scheduler::init(void* taskTableSpaceJunk, uint16_t taskTableSize){
	 /* using 16bit Timer1 */
	 
	 scheduler::taskTable = taskTableSpaceJunk;
	 scheduler::taskTableSize = taskTableSize;
	 
	 //init time
	 
	 /*now.day = 1;
	 now.year = 2017;
	 now.hour = 0;
	 now.minute = 0;
	 now.second = 0;
	 *///###must work around
	 
	 cli(); // deactivate global interrupts
	 
	 TCCR1A = 0x00; // ~ NO PWM
	 TCNT1 = 0; // counter starts at zero
	 TIMSK |= 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
	 // activate global interrupts !!!
	 OCR1A = /*1024*/ 1 << (15 - PARTS_OF_SECOND_LOG); // 1 second divided into 32 equal parts.
	 static_assert(1 << (15 - PARTS_OF_SECOND_LOG) == 1024,"bla bla");
	 // register with compare value
	 
	 TCCR1B = 0b00001111;	// CTC (Clear Timer on Compare Match) and start timer running from ext source on,
	 //triggered rising edge
	 // 32768Hz external oscillator for timing clock
	 sei();	// activate global interrupts
 }

uint16_t scheduler::updateNowTime(){
	uint16_t forwardSeconds;
	uint16_t partOfSecond;
	uint8_t sreg = SREG;
	cli();
	forwardSeconds = divisions_of_second / PARTS_OF_SECOND;
	divisions_of_second = divisions_of_second % PARTS_OF_SECOND;
	partOfSecond = divisions_of_second;
	SREG = sreg;
	while (forwardSeconds){
		--forwardSeconds;
		++now;
	}
	return partOfSecond;
}

void scheduler::run(){
	//uint16_t partOfSecond = updateNowTime();
	/* 'now' is up to date and we know the current offset from the full second */
	
	// look for tasks to be executed
	
}
