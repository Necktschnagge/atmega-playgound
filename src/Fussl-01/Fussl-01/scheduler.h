/*
 * scheduler.h
 *
 * Created: 07.03.2017 18:16:16
 *  Author: Maximilian Starke
 */ 


#ifndef SCHEDULER_H_
#define SCHEDULER_H_
//#include <time.h>
#include <stdint.h>


#ifdef NNNNNNNNN
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

#endif



namespace scheduler {
	
	extern int8_t divisions_of_second; // ... 0 ... 31 ???
	
	class Time {
		public:
		
		enum class Month {
			JAN = 0,
			FEB = 1,
			MAR = 2,
			APR = 3,
			MAY = 4,
			JUN = 5,
			JUL = 6,
			AUG = 7,
			SEP = 8,
			OCT = 9,
			NOV = 10,
			DEC = 11
		};
		
		enum class Day {
			SUN = 0,
			MON = 1,
			TUE = 2,
			WED = 3,
			THU = 4,
			FRI = 5,
			SAT = 6,
		};
		
		namespace english {
			static const char* const JAN = "January";
			static const char* const FEB = "February";
			static const char* const MAR = "March";
			static const char* const APR = "April";
			static const char* const MAY = "May";
			static const char* const JUN = "June";
			static const char* const JUL = "July";
			static const char* const AUG = "August";
			static const char* const SEP = "September";
			static const char* const OCT = "October";
			static const char* const NOV = "November";
			static const char* const DEC = "December";
			
			static const char* const months[] {JAN,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC};
		}
		namespace german {
			static const char* const JAN = "Januar";
			static const char* const FEB = "Februar";
			static const char* const MAR = "Maerz";
			static const char* const APR = english::APR;
			static const char* const MAY = "Mai";
			static const char* const JUN = "Juni";
			static const char* const JUL = english::JUL;
			static const char* const AUG = english::AUG;
			static const char* const SEP = english::SEP;
			static const char* const OCT = "Oktober";
			static const char* const NOV = english::NOV
			static const char* const DEC = "Dezember";
			
			static const char* const months[] {JAN,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC};
		}
		
		static const char * const * months = german::months;
		
		int8_t second; // 0 ... 59
		int8_t minute; // 0 ... 59
		int8_t hour; // 0 ... 23
		int16_t day; // 0 ... 365 / 366
		int16_t year; 

			/* returns true if this year has 366 days, otherwise false */
		inline bool isLeapYear() const;

		
		inline Time& operator++(){
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
			if (day == daysOfThisYear()){
				day = 0;
				++year;
			}
		}
		
		
		inline uint16_t daysOfThisYear(){
			return 365 + isLeapYear();
		}
		
		struct {Month month; uint8_t day;} getDate() const {
			uint16_t cday = day;
			if (cday < 31)					return {Month::JAN, ++cday};
			cday -= 31;
			if (cday < 28 + isLeapYear())	return {Month::FEB, ++cday};
			cday -= 28 + isLeapYear()
			if (cday < 31)					return {Month::MAR, ++cday};
			cday -= 31;
			if (cday < 30)					return {Month::APR, ++cday};
			cday -= 30;
			if (cday < 31)					return {Month::MAY, ++cday}
			cday -= 31;
			if (cday < 30)					
			// better is to iterate through an array to calculate this.
		}
		
		Month getMonth(){
			return getDate().month;
		}
		
		uint8_t getDayOfMonth(){
			return getDate().day;
		}
		
		Day getDayOfWeek(){
			
		}
	};
	typedef Time* PTime;
	
	extern Time now;
	
	void init();
	
	typedef uint16_t SCHEDULE_HANDLE;
	
	constexpr uint16_t NO_HANDLER {0};
	
	void getDayOfWeek(){};
	
	SCHEDULE_HANDLE addTimer(void (*function)(), const Time& interval, uint16_t repeat, Priority priority){return 0;};
	
	bool cancelTimer(SCHEDULE_HANDLE handler){return false;};
	
}

#endif /* SCHEDULER_H_ */