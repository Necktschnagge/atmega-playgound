/*
 * f_systime_old_alpha.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 FPS:	see at .h file
 
 */

#if false 

#define critical_begin		uint8_t __sreg__ = SREG; cli() // rename this shit!!! to something like macro_critical_begin
#define critical_end		SREG = __sreg__
#define critical(code)		critical_begin; { code } critical_end
// it is so un-C++-like but I think here it it good because it makes code better readable and understandable
// make sure that you don't jump from inside a critical to outside without using an extra critical_end;

#include <avr/interrupt.h>

#include "scheduler.h"

using namespace time;

namespace scheduler {
	SystemTime_OLD SystemTime_OLD::instance{}; // default constructor
}
#if false // only because I write a new library defining the same ISR
ISR (TIMER1_COMPA_vect){
	// set the compare-match value for the next interrupt
	// update the SystemTime::instance
	
	OCR1A = scheduler::SystemTime_OLD::__timer__counter__compare__value__only__called__by__interrupt__();
	// this value must not over or underflow. check this!!!
	
	// a problem would be if the calculation of the new compare match is not ready until the next osc rising edge
	// ##detect this.
	
	++scheduler::SystemTime_OLD::instance;
}
#endif

scheduler::SystemTime_OLD::SystemTime_OLD() : refinement(0), now(0), ref_time(0) {
	init(DEFAULT_PRECISION,DEFAULT_OSC_FREQUENCY,0.0L);
}

int16_t scheduler::SystemTime_OLD::get_time_padding(){
	static long double padding {0};
	padding += refinement
	+ static_cast<long double>(osc_frequency % (1<<precision))
	/ static_cast<long double>(1<<precision);
	// extract the int part of padding
	long double trunced = trunc(padding);
	padding -= trunced;
	return static_cast<int16_t>(trunced); // ### think about test cases how are negative numbers trunced?
}

bool scheduler::SystemTime_OLD::is_precision_possible(uint8_t precision){
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

uint16_t scheduler::SystemTime_OLD::__timer__counter__compare__value__only__called__by__interrupt__() {
	//#### check overflows / underflows
	int32_t value;
	
	again:
	value = static_cast<int32_t>(instance.get_normal_TCNT_compare_value()) + static_cast<int32_t>(instance.get_time_padding());
	
	begain:
	if (value == 0){
		// well, I think this is okay. It happens when we use maximum precision and or osc is a slower than it should be.
		// but this case should really not appear far more than one time again during one function call.
		++instance;
		goto again;
	}
	if (value < 0){
		// ### log any kind of error. this case should never happen!!!!
		// we should stop immediately.
		value = 0;
		goto begain;
		// osc frequency is far from its value
	}
	if (value > 0xFFFE){ // FFFF must also be possible
		// osc_frequency far from its value
		// ## log error
		value = 0xFFFF;
	}
	
	return static_cast<uint16_t>(value);
}

void scheduler::SystemTime_OLD::get_now(time::ExtendedMetricTime& target) const {
	critical(
		target = now;
		);
}

bool scheduler::SystemTime_OLD::change_precision_aborting(uint8_t precision){
	// can be interrupted, no problem
	if (is_precision_possible(precision)){
		long double factor {1};
	//	for (uint8_t i = 0; i < abs(precision - this->precision); ++i){
	//		factor *= 2;
	//	} #### abs not working
		// refinement /=  1.0 (1<< (precision - this->precision) ); ###same
		this->precision = precision;
		return true;
	}
	return false;
}

uint8_t scheduler::SystemTime_OLD::change_precision_anyway(uint8_t precision){
	// can be interrupted, no problem,
	// we only change precision, which is read by ISR, but only 1 Byte size
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

void scheduler::SystemTime_OLD::update_refinement(const ExtendedMetricTime& offset){
	critical(
		// vielleicht wäre es besser wenn wir direkt die Frequenz ändern würden.
	);
}

void scheduler::SystemTime_OLD::reset_ref_time(){
	critical(
	// do not interrupt because of reading now
		ref_time = now;
	);
}

const scheduler::SystemTime_OLD& scheduler::SystemTime_OLD::operator ++ (){
	critical(
		// do not interrupt
		// of course: if called by the ISR interrupts are already deactivated,
		// but in case someone else calls this function despite I cannot imagine any purpose to do that
		now = now + ( 1ull << (16 - old_precision) );
		old_precision = precision;
	);
	return *this;
}

bool scheduler::SystemTime_OLD::init(const long double& refinement, uint32_t osc_frequency, uint8_t precision){
	stop();
	// because you should not use this function to change while SystemTime is running
	
	instance.refinement = refinement;
	instance.osc_frequency = osc_frequency;
	instance.precision = 10; // for the error case:
							// at error case precision will be unchanged,
							// so we want to have at least some not totally senseless value
							
							// by the way: if there was precision error you cannot start SystemTime with start();
	uint8_t result = instance.change_precision_anyway(precision);
	instance.old_precision = instance.precision;
	
	return (result != PRECISION_ERROR);
}

bool scheduler::SystemTime_OLD::start(){
	if (instance.is_precision_possible(instance.precision) == false) return false;
	if (TCCR1B & 0b00000111) return false; // already started
	
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

void scheduler::SystemTime_OLD::stop(){
	TCCR1B = 0b00000000;
	//	TIMSK &= 0b11000011;	// cancel the ISR reference
}
























 void scheduler::init(void* taskTableSpaceJunk, uint16_t taskTableSize){
	 /* using 16bit Timer1 */
	 
	 scheduler::taskTable = taskTableSpaceJunk;
	 scheduler::taskTableSize = taskTableSize;
}


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




#endif