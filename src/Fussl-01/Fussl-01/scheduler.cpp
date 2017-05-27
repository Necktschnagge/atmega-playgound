/*
 * scheduler.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 FPS:	see at .h file
 
 */

#include <avr/interrupt.h>

#include "scheduler.h"

using namespace time;

namespace scheduler {
	SystemTime SystemTime::instance{}; // default constructor
}

ISR (TIMER1_COMPA_vect){
	// set the compare-match value for the next interrupt
	// update the SystemTime::instance
	
	OCR1A = scheduler::SystemTime::__timer__counter__compare__value__only__called__by__interrupt__();
	// this value must not over or underflow. check this!!!
	
	// a problem would be if the calculation of the new compare match is not ready until the next osc rising edge
	// ##detect this.
	
	++scheduler::SystemTime::instance;
}

scheduler::SystemTime::SystemTime() : now(0), ref_time(0) {
	init(DEFAULT_PRECISION,DEFAULT_OSC_FREQUENCY,0.0L);
}

bool scheduler::SystemTime::is_precision_possible(uint8_t precision){
	// can be interrupted, no problem
	if (precision > 16) return false; // accuracy exceed of SystemTime itself
	constexpr uint32_t int1_32 {1u};
	if ((int1_32 << precision) > osc_frequency ) return false;
		// SystemTime cannot be more accurate than the oscillator
		// hier fehlt noch der puffer bei annähernd ==, wenn das padding anfängt negativ zu werden
		
	if ( (osc_frequency / (int1_32 << precision)) >= (int1_32 << 15) ) return false;
		// TCNT can only count up to 2^16-1 /// puffer ist hier auch eingebaut nach oben
	return true;
}

bool scheduler::SystemTime::change_precision_aborting(uint8_t precision){
	// can be interrupted, no problem
	if (is_precision_possible(precision)){
		this->precision = precision;
		return true;
	}
	return false;
}

uint8_t scheduler::SystemTime::change_precision_anyway(uint8_t precision){
	// can be interrupted, no problem
	while (precision > 0){
		if (change_precision_aborting(precision)) return precision;
		--precision;
	}
	while (precision < 64){// far too big but possible
		if (change_precision_aborting(precision)) return precision;
		++precision;
	}
	return PRECISION_ERROR;
}

const scheduler::SystemTime& scheduler::SystemTime::operator ++ (){
	uint8_t sreg = SREG;
		// do not interrupt
		// of course: if called by the ISR interrupts are already deactivated,
		// but in case someone else calls this function despite I cannot imagine any purpose to do that
	cli();
	now = now + ( 1ull << (16 - old_precision) );
	old_precision = precision;
	SREG = sreg;
	return *this;
}

bool scheduler::SystemTime::init(uint8_t precision, uint32_t osc_frequency, const long double& clock_precision){
	stop();
	// because you should not use this function to change while SystemTime is running
	// shoukld be called by the constructor ## check this
	
	instance.refinement = clock_precision;
	instance.osc_frequency = osc_frequency;
	instance.precision = 12; // for the error case // why <<<<
	uint8_t result = instance.change_precision_anyway(precision);
	instance.old_precision = instance.precision;
	
	return (result != PRECISION_ERROR);
}

bool scheduler::SystemTime::start(){
	if (instance.is_precision_possible(instance.precision) == false) return false;
	
	// <<<<< if already started ... (do not handle) ??
	
	cli(); // deactivate global interrupts
	
	TCCR1A = 0x00; // ~ NO PWM
	TCNT1 = 0; // counter starts at zero
	
	TIMSK = (TIMSK & 0b11000011) | 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
	
	OCR1A = __timer__counter__compare__value__only__called__by__interrupt__(); // register with compare value
	
	TCCR1B = 0b00001111; // CTC (Clear Timer on Compare Match) and start timer running from ext source on,
						 //triggered rising edge
						 
	sei();	// activate global interrupts
	return true;
}

void scheduler::SystemTime::stop(){
	TIMSK &= 0b11000011;
}
























 void scheduler::init(void* taskTableSpaceJunk, uint16_t taskTableSize){
	 /* using 16bit Timer1 */
	 
	 scheduler::taskTable = taskTableSpaceJunk;
	 scheduler::taskTableSize = taskTableSize;
}

/*
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
		//++now;
	}
	return partOfSecond;
}
*/

uint16_t scheduler::nextFreeHandle {1};
void* scheduler::taskTable {nullptr};
uint16_t scheduler::taskTableSize {0};


void scheduler::run(){
	//uint16_t partOfSecond = updateNowTime();
	/* 'now' is up to date and we know the current offset from the full second */
	
	// look for tasks to be executed
	
}






#if false
// using timer
// 16 bit timer/counter to count the RTC Quartz signal which is 32768 Hz
//we need to setup the 16 bit timer
TCCR1A = 0x00; // we don't use PWM (for creating a specific output signal) and we don't set any output pins
TCCR1B = 0b00001000; // timer turned off; // 1 stands for CTC (Clear Timer on Compare Match) with OCR1A
// CTC1 must be 1 to set the counter 0 when it reaches the compare value, we have no prescaler because of external source
TCCR1B = 0b00001111; // count on rising edge, timer runs.
// TCCR1C is per default 0 so we can leave this out.

// 32768 external oscillator for timing clock
// 16MHz CPU Clock

OCR1A = 1024; // 1 second divided into 32 equal parts
//OCR1AH = 0x08; // high comparison byte // only this order first high than low, please deactivate global interrupts before changing
//OCR1AL = 0x00; // low c byte			8*256 = 2048 // 8 interrupts per second
//OCR1A = 0x0FFF; // directly

// don't know why there are an a, b , c register

// please deactivate interrupts
TCNT1H =  0; // counter starts a zero
TCNT1L = 0; //
TCNT1 = 0;

TIMSK |= 0b00010000; // data sheet page 138: interrupt enable for Timer1 Compare Match A
// activate global interrupts !!!

#endif // static comment




/*
namespace month_string {
	namespace english {
		const char JAN[] = "January";
		const char FEB[] = "February";
		const char MAR[] = "March";
		const char APR[] = "April";
		const char MAY[] = "May";
		const char JUN[] = "June";
		const char JUL[] = "July";
		const char AUG[] = "August";
		const char SEP[] = "September";
		const char OCT[] = "October";
		const char NOV[] = "November";
		const char DEC[] = "December";
		
		const char * const months[] {JAN,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC};
	};
	
	namespace german {
		const char JAN[] = "Januar";
		const char FEB[] = "Februar";
		const char MAR[] = "Maerz";
		const char* const APR = english::APR;
		const char MAY[] = "Mai";
		const char JUN[] = "Juni";
		const char* const JUL = english::JUL;
		const char* const AUG = english::AUG;
		const char* const SEP = english::SEP;
		const char OCT[] = "Oktober";
		const char* const NOV = english::NOV;
		const char DEC[] = "Dezember";
		
		const char* const months[] {JAN,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC};
	};
}*/




