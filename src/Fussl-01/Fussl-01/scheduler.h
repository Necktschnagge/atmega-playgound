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



namespace month_sring {
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
}

class Time;
class ExactTime;

class ExtendedMetricTime {
	int64_t value; // seconds := value / 2^16	0x:   XX XX  XX XX   XX XX . XX XX // fixed dot integer
	
	static int64_t Time_to_ExtendedMetricTime();//####
	
	inline ExtendedMetricTime& operator= (const int64_t& rop) { value = rop; return *this; }
	ExtendedMetricTime& operator = (Time& time);
	ExtendedMetricTime& operator = (ExactTime& time);
		
	inline ExtendedMetricTime(int64_t value) : value(value) {}
	inline ExtendedMetricTime(Time& time) { *this = time; }
	inline ExtendedMetricTime(ExactTime& time);
	
	
	inline ExtendedMetricTime operator+ (const ExtendedMetricTime& rop) { return value + rop.value; }
	inline ExtendedMetricTime operator- (const ExtendedMetricTime& rop) { return value - rop.value; }
	
	inline bool operator == (const ExtendedMetricTime& rop) { return value == rop.value; }
	inline bool operator <  (const ExtendedMetricTime& rop) { return value <  rop.value; }
	inline bool operator >  (const ExtendedMetricTime& rop) { return value >  rop.value; }
	inline bool operator <= (const ExtendedMetricTime& rop) { return value <= rop.value; }
	inline bool operator >= (const ExtendedMetricTime& rop) { return value >= rop.value; }
		
	};

class Time {
	
	private:
	
	static constexpr uint8_t __monthLength__[12] {31,0,31,30,31,30,31,31,30,31,30,31};
	
	public:
	
	enum class Month : uint8_t {
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
	
	enum class Day : uint8_t {
		SUN = 0,
		MON = 1,
		TUE = 2,
		WED = 3,
		THU = 4,
		FRI = 5,
		SAT = 6,
	};
	
	typedef struct sdate {
		Month month;
		uint8_t day;
	} date_t;
	
	int8_t second {0}; // 0 ... 59
	int8_t minute {0}; // 0 ... 59
	int8_t hour {0}; // 0 ... 23
	int16_t day {0}; // 0 ... 365 / 366
	int16_t year {0};
		
		/* constructor */
	Time(){}
	
	void normalize();
	
	static uint8_t month_to_int(Month month); // convertion operator ???
	static Month int_to_month(uint8_t uint8);
	
		/* returns true if this year has 366 days, otherwise false */
	inline static constexpr bool isLeapYear(int16_t year);
	inline bool isLeapYear() const		{	return isLeapYear(this->year);	}
	
		/* return the length in days of given month */
	static uint8_t getMonthLength(Month month, bool isLeapYear);	
	inline static uint8_t getMonthLength(Month month,int16_t year)	{	return getMonthLength(month,isLeapYear(year));	}
	inline uint8_t getMonthLength(Month month) const				{	return getMonthLength(month,isLeapYear());		}
	
		/* tick forward one second */
	inline Time& operator++();
	
	inline static int16_t daysOfYear(int16_t year)	{	return 365 + isLeapYear(year);	}
	inline int16_t daysOfYear()	const				{	return 365 + isLeapYear();		}
	
		/* returns current date (enum Month and day {1..31}) */
	date_t getDate() const;
	
		/* return current month */
	inline Month getMonth() const					{	return getDate().month;	}
	
		/* return current day of month */
	inline uint8_t getDayOfMonth() const			{	return getDate().day;	}
		
		/* return current day of week */
	Day getDayOfWeek() const;
	
		/* attention: operator normalizes the operands*/
	inline bool operator==(Time& rop);
		/* attention: operator normalizes the operands*/	
	inline bool operator != (Time& rop) { return ! operator == (rop); }
		/* attention: operator normalizes the operands*/
	bool operator<(Time& rop);
		/* attention: operator normalizes the operands*/
	inline bool operator <=(Time& rop) { return operator<(rop) || operator==(rop); }
		/* attention: operator normalizes the operands*/
	inline bool operator >(Time& rop) { return !operator<=(rop); }
		/* attention: operator normalizes the operands*/
	inline bool operator >= (Time& rop) { return !operator<(rop); }
		
	Time operator + (Time& rop);
	Time operator - (Time& rop);
	
};

class ExtendedTime {
public:
	Time time;	
	uint16_t divisions_of_second;
	
	inline void normalize(){ time.normalize(); }
	
	//operator == != < > <= >= + - 
	
};



/*operators for enum class must be declared here*/
Time::Month& operator++(Time::Month& op);
inline Time::Month& operator--(Time::Month& op);

namespace scheduler {
	
	typedef uint16_t SCHEDULE_HANDLE;
	constexpr uint16_t NO_HANDLE {0};
	
	constexpr uint16_t PARTS_OF_SECOND_LOG {5}; // 32 parts of second
	constexpr uint16_t PARTS_OF_SECOND {1 << PARTS_OF_SECOND_LOG};
		
	class Priority{
		uint8_t state;
		
		void setState(uint8_t priority, uint8_t percentage){
			
		}
		
	};
	
	extern uint16_t divisions_of_second; // 0 ... PARTS_OF_SECOND - 1
	extern Time now;
	extern uint16_t nexHandle;
	extern void* taskTable;
	extern uint16_t taskTableSize;
	
	void init(void* taskTableSpaceJunk, uint16_t taskTableSize);
	
	//SCHEDULE_HANDLE addTimer(void (*function)(), const Time& interval, uint16_t repeat /*, Priority priority*/){return 0;};
	
	//bool cancelTimer(SCHEDULE_HANDLE handler){return false;};
	
		// should return: whether time was increased, exact time after last full second
	uint16_t updateNowTime();
	
		/* central control loop */
	void run();
	
	class SchedulingRecord{
		SCHEDULE_HANDLE handle;
		// put the callback union from gui to another header
		uint8_t flags; //{ valid, Timer/Task, Callable/procedure, }
		
	};
	
	class ExactTime {
		Time time;
		uint8_t divisions_of_second;
		};
	
	class UCallback {
		uint16_t dummy;
		};
	
	class ExecutionTime {
		float dummy;
		};
	
	class BackgroundTaskMemLine {
		SCHEDULE_HANDLE handle;
		UCallback callback;
		uint8_t percentage;
		uint8_t priority;
		//ExecutionTime exeTime;
		uint8_t flags;
		};
		
	class TimerMemLine {
		SCHEDULE_HANDLE handle;
		UCallback callback;
		
		uint8_t priority;
		//ExecutionTime exeTime;
		uint8_t flags;
		};
	class TimerMemLineTime {
		ExactTime time;
		};
	class TimerMemLineTime2 {
		ExactTime time; // but we can leave out the year.
		uint16_t repeatings;
	};
	
	
	
	/************************************************************************/
	/* 
	ideas:
	
	list of timers:
		2				2								2											2					2				1								1
		handle,		task procedure or callable,		time when to execute*, +part of second ,		intervall*,		repeatings,		priority,		exe time,		some flags (valid etc, end of list-flag, is timer/task falg, isBackground- flag, callable/ptr flag)
													// put this directly after the first "line" in memory. then we do not need pointer.
	list of background tasks:
		2					2																		1				1								1
		handle,		callback,																	percentage,		priority,		exe time,		some flags
	
	                                                                     */
	/************************************************************************/
	
}

#ifdef hack
uint16_t test(){
	uint16_t x[2];
	return x[sizeof(Time)];
}
#endif

#endif /* SCHEDULER_H_ */