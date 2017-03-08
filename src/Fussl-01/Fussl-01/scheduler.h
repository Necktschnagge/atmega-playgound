/*
 * scheduler.h
 *
 * Created: 07.03.2017 18:16:16
 *  Author: Maximilian Starke
 */ 


#ifndef SCHEDULER_H_
#define SCHEDULER_H_
//#include <time.h>



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


ISR (TIMER1_COMPA_vect){
	/* compare match interrupt routine */
	
}

namespace scheduler {
	
	int8_t divisions_of_second; // ... 0 ... 31 ???
	
	class Time {
		public:
		
		int8_t second; // 0 ... 59
		int8_t minute; // 0 ... 59
		int8_t hour; // 0 ... 23
		int16_t day; // 0 ... 365 / 366
		int16_t year; 
		
		Time& operator++(){
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
			if (day == ())
		}
		
		enum class Month {
			JANUARY,
			FEBRUARY,
			MARCH,
			APRIL,
			MAY,
			JUNE,
			JULY,
			AUGUST,
			SEPTEMBER,
			OCTOBER,
			NOVEMBER,
			DECEMBER
		};
		
		
		
		struct {Month month, uint8_t day} getDate(){
			//if 
		}
		
		Month getMonth(){
			
		}
	};
	
	MAY
	
	typedef Time* PTime;
	
	extern Time now;
	
	void init(){
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
	
	typedef uint16_t SCHEDULE_HANDLE;
	
	constexpr uint16_t NO_HANDLER {0};
	
	void getDayOfWeek(){};
	
	SCHEDULE_HANDLE addTimer(void (*function)(), const Time& interval, uint16_t repeat, Priority priority){return 0;};
	
	bool cancelTimer(SCHEDULE_HANDLE handler){return false;};
	
}

#endif /* SCHEDULER_H_ */