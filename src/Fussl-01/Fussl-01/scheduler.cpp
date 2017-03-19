/*
 * scheduler.cpp
 *
 * Created: 07.03.2017 18:15:40
 *  Author: Maximilian Starke
 
 
 */

#include "scheduler.h"
#include <avr/interrupt.h>

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

void Time::normalize(){
	// clear seconds:
	minute += second / 60;
	second = second % 60;
	// clear minutes:
	hour += minute / 60;
	minute %= 60;
	// clear hour:
	day += hour / 24;
	hour %= 24;
	// clear day;
	if (day >= daysOfYear()) {
		day -= daysOfYear();
		++year;
		normalize();
	}
	if (day < 0){
		--year;
		day += daysOfYear();
		normalize();
	}
}


bool Time::isLeapYear(int16_t year){
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
	++second; // tick forward
	
	// check overflows:
	
	//old implementation // faster but needs more program memory
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
	
	// new implementation // needs more time but less flash memory
	//normalize();
	
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

bool Time::setDate(uint8_t month, uint8_t day_of_month){
	//### do this
	return false;
}

Time::Day Time::getDayOfWeek() const {
	/* via extended version of Gauss' formula (Georg Glaeser) */
	date_t cdate = getDate();
	int16_t cyear = this->year - (month_to_int(cdate.month) < 3);
	uint8_t cday = cdate.day;
	--cdate.month;
	--cdate.month;
	uint16_t shiftedMonth = month_to_int(cdate.month);
	uint8_t llyear = cyear % 100;
	int16_t uuyear = cyear / 100;
	
	return static_cast<Time::Day>((cday + ((26*shiftedMonth-2) / 10 ) + llyear + (llyear / 4) + (uuyear / 4) -2*uuyear) % 7);
	//return Day::MON;
}

bool Time::operator == (Time& rop){
	/*do we have to normalize the time ??#####*/
	rop.normalize();
	/*Time self {*this};
	self.normalize();*/
	normalize();
	return (rop.second == second) && (rop.minute == minute) && (rop.hour == hour) && (rop.day == day) && (rop.year == year);
}

bool Time::operator <(Time& rop){
	rop.normalize();
	normalize();
	return (year < rop.year) || ((year == rop.year) && (
		(day < rop.day) || ((day == rop.day) && (
			(hour < rop.hour) || ((hour == rop.hour) && (
				(minute < rop.minute) || ((minute == rop.minute) && (
					second < rop.second
				))
			))
		))
	));
}

Time Time::operator + (Time& rop){
	rop.normalize();
	normalize();
	Time result;
	
	result.second = second + rop.second;
	result.minute = minute + rop.minute;
	result.hour = hour + rop.hour;
	result.day = day + rop.day;
	result.year = year + rop.year;// <<< overflow ignored.
	result.normalize();
	
	return result;
}

Time Time::operator - (Time& rop){
	rop.normalize();
	normalize();
	Time result;
	
	result.second = second - rop.second;
	result.minute = minute - rop.minute;
	result.hour = hour - rop.hour;
	result.day = day - rop.day; // this is not working this way. if we want a difference between two dates. (????)
	result.year = year;
	result.normalize();
	//### idea. we define a new "independent time": second since time zero and make convertion constructors to normal time.
	
	result.year -= rop.year; // overflow ignored;
	result.normalize();
	
	return result;
}

namespace {
	// 400 years in days:
	constexpr int64_t factor400 {400ll * 365 /*normal year*/ + 100 * 1 /*leap years*/ - 3 /*no leap years: 100, 200, 300*/ };
	// [++value]s for one day:
	constexpr int64_t ticksPerDay { 24LL * 60 * 60 * (1LL << 16) };
}

/************************************************************************/
/* ET class                                                             */
/************************************************************************/

ExtendedTime::ExtendedTime(const EMT& emt) /*: divisions_of_second( (time.operator int64_t()) & 0xFFFFLL)*/ {
	int64_t value = emt;
	divisions_of_second = value % 0x10000;
	value -= divisions_of_second;
	value = value >> 16;
	time.second = value % 60;
	value -= time.second;
	value /= 60;
	time.minute = value % 60;
	value -= time.minute;
	value /= 60;
	time.hour = value % 24;
	value -= time.hour;
	value /= 60;
	// now we have time remaining in days...
	// first check for applicable 400 years...
	time.year = 400 * (  ( value - (value % factor400) ) / factor400  );
	value -= time.year * factor400;
	// 0 <= value < factor400 assert <<<<<
	while (value >= time.daysOfYear()){
		value -= time.daysOfYear();
		++time.year;
	}
	time.day = value;
}

/************************************************************************/
/* EMT class                                                            */
/************************************************************************/

ExtendedMetricTime& ExtendedMetricTime::operator = (const ExtendedTime& time){	
	value = time.divisions_of_second;
	Time source = time.time; // cannot use element initializer {...} because of public member attributes (!)
	source.normalize();
	
	value += ticksPerDay * factor400 * ( (source.year - source.year % 400) / 400); // -600 -> -800 -> -2 instead of -600 -> -1
	source.year %= 400;
	while(source.year > 0){
		--source.year;
		value += ticksPerDay * source.daysOfYear();
	}
	value += (1LL<<16) * (60LL * (60LL * (24LL * (source.day) + source.hour) + source.minute) + source.second);
	return *this;
}

uint16_t scheduler::divisions_of_second {0};
Time scheduler::now;
uint16_t scheduler::nextFreeHandle {1};
void* scheduler::taskTable {nullptr};
uint16_t scheduler::taskTableSize {0};

 void scheduler::init(void* taskTableSpaceJunk, uint16_t taskTableSize){
	 /* using 16bit Timer1 */
	 
	 scheduler::taskTable = taskTableSpaceJunk;
	 scheduler::taskTableSize = taskTableSize;
	 
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
	uint16_t partOfSecond = updateNowTime();
	/* 'now' is up to date and we know the current offset from the full second */
	
	// look for tasks to be executed
	
}
