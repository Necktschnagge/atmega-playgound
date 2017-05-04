/*
 * f_time.h
 *
 * Created: 04.05.2017 14:28:26
 *  Author: Maximilian Starke
 */ 

#ifndef F_TIME_H_
#define F_TIME_H_

/* extern headers */
#include <stdint.h>

/* [no] own headers */


class HumanTime;
class ExtendedMetricTime;

typedef ExtendedMetricTime EMetricTime, EMTime, EMT;
typedef HumanTime HTime, HT;

// typedef HumanTime Time;


////<<< put everything in a namespace
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

inline Month& operator++(Month& op){
	return op = static_cast<Month>(static_cast<uint8_t>(op) + 1 % 12);
}

inline Month& operator--(Month& op){
	return op = static_cast<Month>(static_cast<uint8_t>(op) + 11 % 12);
}
	
	enum class Day : uint8_t {
		SUN = 0,
		MON = 1,
		TUE = 2,
		WED = 3,
		THU = 4,
		FRI = 5,
		SAT = 6,
	};


class HumanTime {
	
private:
	
	static constexpr uint8_t __monthLength__[12] {31,0,31,30,31,30,31,31,30,31,30,31};
		// why is feb 0 ??: just because it is not used since feb 28/29 must be calculated individually
		
	int8_t second {0};	// 0 ... 59
	int8_t minute {0};	// 0 ... 59
	int8_t hour {0};	// 0 ... 23
	int16_t day {0};	// 0 ... 364 / 365
	int16_t year {0};
	// private values have to be normalized after leaving any member function
	

public:
	
	struct HDate;
	
	struct Date {
		Month month; 
		uint8_t day; // 0 ... MonthLength-1 // #days over in this month
		
			/* check whether the date valid */
		bool is_valid(bool isLeapYear) const;
	
		Date(Month month, uint8_t day, bool isLeapYear = true) : month(month), day(day) {
			if (!is_valid(isLeapYear)) {
				month = Month::JAN;
				day = 0;
			}
		}
			/* create a date from an HDate
			   set to 1.1. if not valid with given isLeapYear */
		Date(const HDate& hdate, bool isLeapYear = true) : month(static_cast<Month>(hdate.month-1)), day(hdate.day-1) {
			if (!is_valid(isLeapYear)){
				month = Month::JAN;
				day = 0;
			}
		}
	};
	
	struct HDate {
		uint8_t month;	// 1..12
		uint8_t day;	// 1..31
		
		bool is_valid(bool isLeapYear) const;
	};
			
		/* constructors */
	constexpr HumanTime(){}
		
	HumanTime(const ExtendedMetricTime& emt);
	
	//HumanTime(const ExtendedTime& etime);
	
	void normalize();
	
	// ### provide some getters and setters!
	inline int8_t get_second()		const { return second;	}
	inline int8_t get_minute()		const { return minute;	}
	inline int8_t get_hour()		const { return hour;	}
	inline int16_t get_day()		const { return day;		}
	inline int16_t get_day_of_year()const { return get_day();}
	inline int16_t get_year()		const { return year;	}

		/* enum month -> human-friendly month int8 coding (JAN,1), (FEB,2), ..., (DEC,12) */
	inline static uint8_t month_to_int(Month month)		{ return static_cast<uint8_t>(month) + 1; };
		
		/* 1->JAN, 2->FEB, ..., 12->DEC */
	inline static Month int_to_month(uint8_t uint8)		{ return static_cast<Month>((uint8 - 1) % 12); }
	
		/* returns true if given year has 366 days, otherwise false */
	inline static bool constexpr isLeapYear(int16_t year); // constexpr makes it implicitly inline
	
		/* return true if  this->year has 366 days, otherwise false */
	inline bool isLeapYear() const		{	return isLeapYear(this->year);	}
	
		/* return the length in days of given month */
	static uint8_t getMonthLength(Month month, bool isLeapYear);
	
		/* return the length in days of given month */
	inline static uint8_t getMonthLength(Month month,int16_t year)	{	return getMonthLength(month,isLeapYear(year));	}
		
		/* return the length in days of given month in this->year */
	inline uint8_t getMonthLength(Month month) const				{	return getMonthLength(month,isLeapYear());		}
	
		/* tick forward one second */
	HumanTime& operator++(); //could be inline if we use the time intensive solution <<<<<
		// what about -- ??  //<<<<
			
		/* return the amount of days of given year */
	inline static int16_t daysOfYear(int16_t year)	{	return 365 + isLeapYear(year);	}
		
		/* return the amount of days of this->year */
	inline int16_t daysOfYear()	const				{	return 365 + isLeapYear();		}
	
		/* returns current date (enum Month and day {0..30}) */
	Date getDate() const;
	
		/* change the date within the current year */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
	bool setDate(const Date& date);
		
		/* change the date within the current year */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
	inline bool setDate(const HDate& hdate)			{	if (hdate.is_valid(isLeapYear())) return setDate(Date(hdate, true));	return false;	}
	
		/* set the date within current year by using human friendly coding */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
	inline bool setHDate(uint8_t month, uint8_t day_of_month)	{	return setDate({month, day_of_month});	}
		
		/* set the date within current year by using human friendly coding */	
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
	inline bool setHDate(Month month, uint8_t day_of_month)		{	return setDate({static_cast<uint8_t>(static_cast<uint8_t>(month) + 1), day_of_month});	}
	
		/* return current month */
	inline Month get_month() const					{	return getDate().month;	}
	
		/* return current day (0..30) of month */
	inline uint8_t get_day_of_month() const			{	return getDate().day;	}
		
		/* return current day (1..31) of month */
	inline uint8_t getHDayOfMonth() const			{	return getDate().day+1;	}	
		
		/* return current day of week */
	Day getDayOfWeek() const;
	
	/// <<< refactoring "normalizes should nit be necessary since we made the intern data private:
		/* attention: operator == normalizes the operands */
		/* decides whether two Times are equal down to seconds */
	bool operator==(HumanTime&);

		/* attention: operator normalizes the operands */
		/* decides where two Times are not equal in at least one second */
	inline bool operator != (HumanTime& rop) { return ! operator == (rop); }
		// do we need this inline function or does C++ provide Templates of those Operators doing that stuff for us ??? <<<<
		
		/* attention: operator normalizes the operands */
	bool operator<(HumanTime&);
	
		/* attention: operator normalizes the operands */
	inline bool operator <=(HumanTime& rop) { return operator<(rop) || operator==(rop); }
		
		/* attention: operator normalizes the operands */
	inline bool operator >(HumanTime& rop) { return !operator<=(rop); }
		
		/* attention: operator normalizes the operands */
	inline bool operator >= (HumanTime& rop) { return !operator<(rop); }
		
		/* attention: operator normalizes the operands */
		/* add two times (deprecated) */
		/* one Time is interpreted as itself and the other time as a time-dif. */
		/* for time diffs it is recommended to use ExtendedMetricTime or MetricTime */
	//Time operator + (Time& rop); // deprecated!!!!!
	
	//inline Time operator + (const EMT& rop) const; // make this the default implementation
	
	//inline Time operator - (const EMT& rop) const; // reduce against + if possible anyway!
	
	//EMT operator - (Time& rop);// Time - Time = EMT,  <<--  emt - time = emt via conversion
	
	/************************************************************************/
	/* thinking about what arithmetoic iperands should be possible:
	
	EMT for time in any context. (a time vector)
		a time gap (a time vector itself)
		a time (a time vector pointing to a time point when the perspective is known, but of course independent from the point of standing)
	
	so with EMT you can:
		+ - bin and unär anyway
		+ unär ???
		- unär
		emt + emt -> emt
		emt - emt -> emt
		
		
		EMT * int -> EMT
		int *EMT -> EMT
		EMT : EMT -> int
		EMT : int -> emt
	
	HumanTime: /// << we really should call it HumanTime and make a typedef to use just 'Time' as synonym.
		only for time in context as a time point (but of course independent from point of view)
		
		emt + HT -> ht
		ht + emt -> ht
		ht - ht -> emt
		ht - emt -> ht
		
		
		
	                                                                     */
	/************************************************************************/
	
};

inline auto operator + (const HumanTime&)								-> const HumanTime&;
		auto operator - (const HumanTime&)								-> const HumanTime& = delete;
inline auto operator + (const ExtendedMetricTime&, const HumanTime&)	-> HumanTime;
inline auto operator + (const HumanTime&, const ExtendedMetricTime&)	-> HumanTime;
inline auto operator - (const HumanTime&, const HumanTime&)				-> ExtendedMetricTime;
inline auto operator - (const HumanTime&, const ExtendedMetricTime&)	-> HumanTime;
///## those op need to be defined!!!!!


/* f-like transcription operators, = operator must be a non-static member function */
inline void operator << (Month& lop, uint8_t rop)		{ lop = HumanTime::int_to_month(rop); }
inline void operator >> (Month lop, uint8_t& rop)		{ rop = HumanTime::month_to_int(lop); }
inline void operator << (uint8_t& lop, Month rop)		{ return rop>>lop; }
inline void operator >> (uint8_t lop, Month& rop)		{ return rop<<lop; }



//<<< refactor: provide a new Metric Time which works as EMT but has no extendency means there are no divisions of seconds
/* we don't need such a time class right now. just providing EMT and HT should be enough.
class ExtendedTime { // should be exthumantime !!! 
public:
	HumanTime time;	
	uint16_t divisions_of_second;
	
	ExtendedTime(const ExtendedTime& extTime) = default; // copy constructor
	ExtendedTime(const ExtendedTime&& extTime) = delete;
	ExtendedTime(const HumanTime& time) : time(time), divisions_of_second(0) {} // conversion constructor for Time
	ExtendedTime(const ExtendedMetricTime& time); // conversion from EMT
	
	inline void normalize(){ time.normalize(); }
	
	//operator == != < > <= >= + - 
	
};
*/


class ExtendedMetricTime { // seems ready, only some <<<< test sth out.
private:
public:
	static constexpr HumanTime CHRIST_0 = HumanTime();

	int64_t value; // seconds := value / 2^16	0x:   XX XX  XX XX   XX XX . XX XX // fixed-dot-integer
	
public:
		/* assignment operators */
	inline ExtendedMetricTime& operator = (const int64_t& rop) { value = rop; return *this; }
	inline ExtendedMetricTime& operator = (const HumanTime& time);
	
		/* ctor / conversion	from/to		int64 */
	inline ExtendedMetricTime(int64_t value) : value(value) {}	// implicit conversion int64 ->  EMT
	//inline operator int64_t() const { return value; }					// implicit conversion  EMT  -> int64
		
		/* c-tors */
	inline ExtendedMetricTime(const HumanTime& time, const HumanTime& perspective = CHRIST_0);
	// { *this = time; } //## handle christ// must be implicitly possible via extendedTime c-tor <<<<?????
	
	//inline int64_t& inner_value()								{	return value;	}
	
		/* setter (supposedly for time differences) */
	inline void setFromSeconds	(int32_t seconds)		{ value = (1LL << 16) * seconds; }
	inline void setFromMinutes	(int16_t minutes)		{ value = (1LL << 16) * 60 * minutes; }
	inline void setFromHours	(int16_t hours)			{ value = (1LL << 16) * 60 * 60 *hours; }
	inline void setFromDays		(int16_t days)			{ value = (1LL << 16) * 60 * 60 * 24 * days; }
	void set_from(const HumanTime& time, const HumanTime& perspective = CHRIST_0);
// { *this = time; } //## handle christ// must be implicitly possible via extendedTime c-tor <<<<?????
	
		/* factories */
	inline static ExtendedMetricTime seconds_to_EMT		(int32_t seconds)		{ return ExtendedMetricTime((1LL << 16) * seconds); }
	inline static ExtendedMetricTime minutes_to_EMT		(int16_t minutes)		{ return ExtendedMetricTime((1LL << 16) * 60 * minutes); }
	inline static ExtendedMetricTime hours_to_EMT		(int16_t hours)			{ return ExtendedMetricTime((1LL << 16) * 60 * 60 * hours); }
	inline static ExtendedMetricTime days_to_EMT		(int16_t days)			{ return ExtendedMetricTime((1LL << 16) * 60 * 60 * 24 * days); }	
	
		/* arithmetic operators */
	inline friend ExtendedMetricTime operator + (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop);
	//inline friend HumanTime operator + (const ExtendedMetricTime&, const HumanTime&);
	inline friend ExtendedMetricTime operator - (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop);
	template <typename T>
	inline friend ExtendedMetricTime operator * (ExtendedMetricTime emt, T factor);
	template <typename T>
	inline friend ExtendedMetricTime operator * (T factor, ExtendedMetricTime emt);
	template <typename T>
	inline friend ExtendedMetricTime operator / (ExtendedMetricTime emt, T divisor);
	template <typename T>
	inline friend void div(const ExtendedMetricTime&, const ExtendedMetricTime, T&);
	
		/* comparison operators */
	inline friend bool operator == (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop); /// <<<<<<< no lop, rop
	inline friend bool operator <  (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop); 
	inline friend bool operator >  (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop); 
	inline friend bool operator <= (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop); 
	inline friend bool operator >= (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop);
		// ## is comparison possible without these functions and with implicit conversion to int instead ????
};





/* header body : */

// HumanTime Operators:

inline auto operator + (const HumanTime& op)								-> HumanTime const& {
	return op;
}

inline auto operator + (const ExtendedMetricTime& lop, const HumanTime& rop)	-> HumanTime {
	return HumanTime(lop + ExtendedMetricTime(rop, ExtendedMetricTime::CHRIST_0));
}

inline auto operator + (const HumanTime&, const ExtendedMetricTime&)	-> HumanTime;
inline auto operator - (const HumanTime&, const HumanTime&)				-> ExtendedMetricTime;
inline auto operator - (const HumanTime&, const ExtendedMetricTime&)	-> HumanTime;

// EMT constructors and operators
inline ExtendedMetricTime& ExtendedMetricTime::operator = (const HumanTime& time) {
	set_from(time,CHRIST_0);
	return *this;
}

inline ExtendedMetricTime::ExtendedMetricTime(const HumanTime& time, const HumanTime& perspective){
	set_from(time, perspective);
}


// notice: constexpr functions are implicitely inline!
bool constexpr HumanTime::isLeapYear(int16_t year){ // ready, checked
	/*
	if (year % 4)	return false;
	if (year % 100)	return true;
	if (year % 400)	return false;
	return true;
	*/
	return (year % 4) ? false : (
							(year % 100) ? true : (!(year % 400))
							);
}



inline bool operator == (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop)		{ return lop.value == rop.value; }
inline bool operator <  (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop)		{ return lop.value <  rop.value; }
inline bool operator >  (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop)		{ return lop.value >  rop.value; }
inline bool operator <= (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop)		{ return lop.value <= rop.value; }
inline bool operator >= (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop)		{ return lop.value >= rop.value; }

inline ExtendedMetricTime operator + (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop) { return lop.value + rop.value; }
inline ExtendedMetricTime operator - (const ExtendedMetricTime& lop, const ExtendedMetricTime& rop) { return lop.value - rop.value; }
inline ExtendedMetricTime operator * (const ExtendedMetricTime emt, const int64_t factor)			{ return emt * factor; }
inline ExtendedMetricTime operator * (const int64_t factor, const ExtendedMetricTime emt)			{ return factor * emt; }
inline ExtendedMetricTime operator / (const EMT emt, const int64_t divisor)							{ return emt / divisor; }

/*inline Time Time::operator + (const EMT& rop) const { return rop + EMT(*this); }
inline Time Time::operator - (const EMT& rop) const { return EMT(*this) - rop; }
*/







#endif /* F_TIME_H_ */