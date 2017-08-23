/*
 * f_time.h
 *
 * Created: 04.05.2017 14:28:26
 *  Author: Maximilian Starke
 
 FPS:
	^header and source file seem to consistent and ~complete subsystem
	something due to prettiness could be done here.
	test should be written, f-checks should be done before.
	
	just because of time problem we leave this lib at its current state
	
	all time stuff has also been put in an extra namespace (after everything was written). 
	(This destroyed (like ever) a lot. I hope I made no mistakes when hot fixing this namespace conflicts)
	
 */ 

#ifndef F_TIME_H_
#define F_TIME_H_


/* extern headers */
#include <stdint.h>

/* [no] own headers */


namespace time {

	/************************************************************************/
	/* TYPE DECLARATION
																			*/
	/************************************************************************/

	class HumanTime;
	class ExtendedMetricTime;

	typedef ExtendedMetricTime EMetricTime, EMTime, EMT;
	typedef HumanTime HTime, HT;

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


	class HumanTime {

	private:

		static constexpr uint8_t __monthLength__[12]{ 31,0,31,30,31,30,31,31,30,31,30,31 };
		// why is feb 0 ??: just because it is not used since feb 28/29 must be calculated individually

		int8_t second{ 0 };	// 0 ... 59
		int8_t minute{ 0 };	// 0 ... 59
		int8_t hour{ 0 };	// 0 ... 23
		int16_t day{ 0 };	// 0 ... 364 / 365
		int16_t year{ 0 };
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
			Date(const HDate& hdate, bool isLeapYear = true) : month(static_cast<Month>(hdate.month - 1)), day(hdate.day - 1) {
				if (!is_valid(isLeapYear)) {
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
		constexpr HumanTime() {}

		HumanTime(const ExtendedMetricTime& emt);

		/* create a HumanTime by year, day, hour, minute, second */
		/* attention: if any illegal argument, the behavior is undefined */
		/* normalize() will be called to avoid illegal states */
		HumanTime(int16_t year, int16_t day_of_year, int8_t hour, int8_t minute, int8_t second) : second(second), minute(minute), hour(hour), day(day_of_year), year(year) { normalize(); }

		HumanTime(int16_t year, const Date& date, int8_t hour, int8_t minute, int8_t second) : second(second), minute(minute), hour(hour), day(0), year(year) { setDate(date); normalize(); }

		inline int8_t get_second()		const { return second; }
		inline int8_t get_minute()		const { return minute; }
		inline int8_t get_hour()		const { return hour; }
		inline int16_t get_day()		const { return day; }
		inline int16_t get_day_of_year()const { return get_day(); }
		inline int16_t get_year()		const { return year; }

		inline int8_t& unsave_second() { return second; }
		inline int8_t& unsave_minute() { return minute; }
		inline int8_t& unsave_hour() { return hour; }
		inline int16_t& unsave_day() { return day; }
		inline int16_t& unsave_year() { return year; }
		void normalize();

		inline HumanTime& set_second(int8_t s) { second = s; normalize(); return *this; }
		inline HumanTime& set_minute(int8_t m) { minute = m; normalize(); return *this; }
		inline HumanTime& set_hour(int8_t h) { hour = h; normalize(); return *this; }
		inline HumanTime& set_day(int16_t d) { day = d; normalize(); return *this; }
		inline HumanTime& set_year(int16_t y) { year = y; normalize(); return *this; }

		/* enum month -> human-friendly month int8 coding (JAN,1), (FEB,2), ..., (DEC,12) */
		inline static uint8_t month_to_int(Month month) { return static_cast<uint8_t>(month) + 1; };

		/* 1->JAN, 2->FEB, ..., 12->DEC */
		inline static Month int_to_month(uint8_t uint8) { return static_cast<Month>((uint8 - 1) % 12); }

		/* returns true if given year has 366 days, otherwise false */
		inline static bool constexpr isLeapYear(int16_t year); // constexpr makes it implicitly inline

			/* return true if  this->year has 366 days, otherwise false */
		inline bool isLeapYear() const { return isLeapYear(this->year); }

		/* return the length in days of given month */
		static uint8_t getMonthLength(Month month, bool isLeapYear);

		/* return the length in days of given month */
		inline static uint8_t getMonthLength(Month month, int16_t year) { return getMonthLength(month, isLeapYear(year)); }

		/* return the length in days of given month in this->year */
		inline uint8_t getMonthLength(Month month) const { return getMonthLength(month, isLeapYear()); }

		/* tick forward one second */
		inline HumanTime& operator++() { ++second; normalize(); return *this; }

		inline HumanTime& operator--() { --second; normalize(); return *this; }

		/* return the amount of days of given year */
		inline static int16_t daysOfYear(int16_t year) { return 365 + isLeapYear(year); }

		/* return the amount of days of this->year */
		inline int16_t daysOfYear()	const { return 365 + isLeapYear(); }

		/* returns current date (enum Month and day {0..30}) */
		Date getDate() const;

		/* change the date within the current year */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
		bool setDate(const Date& date);

		/* change the date within the current year */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
		inline bool setDate(const HDate& hdate) { if (hdate.is_valid(isLeapYear())) return setDate(Date(hdate, true));	return false; }

		/* set the date within current year by using human friendly coding */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
		inline bool setHDate(uint8_t month, uint8_t day_of_month) { return setDate({ month, day_of_month }); }

		/* set the date within current year by using human friendly coding */
		/* returns true if date was valid, date changed */
		/* returns false if date was invalid, date unchanged */
		inline bool setHDate(Month month, uint8_t day_of_month) { return setDate({ static_cast<uint8_t>(static_cast<uint8_t>(month) + 1), day_of_month }); }

		/* return current month */
		inline Month get_month() const { return getDate().month; }

		/* return current day (0..30) of month */
		inline uint8_t get_day_of_month() const { return getDate().day; }

		/* return current day (1..31) of month */
		inline uint8_t getHDayOfMonth() const { return getDate().day + 1; }

		/* return current day of week */
		Day getDayOfWeek() const;

		/* decides whether two Times are equal down to seconds */
		bool operator==(const HumanTime&) const;

		/* decides where two Times are not equal in at least one second */
		inline bool operator != (const HumanTime& rop) const { return !operator == (rop); }

		/* decides if a human time is strong less another */
		bool operator<(const HumanTime&) const;

		inline bool operator <=(const HumanTime& rop) const { return operator<(rop) || operator==(rop); }
		inline bool operator >(const HumanTime& rop) const { return !operator<=(rop); }
		inline bool operator >= (const HumanTime& rop) const { return !operator<(rop); }


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

	class ExtendedMetricTime { // seems ready, only some <<<< test sth out.
	private:
	public:
		static constexpr HumanTime CHRIST_0 = HumanTime();

		int64_t value; // seconds := value / 2^16	0x:   XX XX  XX XX   XX XX . XX XX // fixed-dot-integer

	public:
		/* assignment operators */
		inline ExtendedMetricTime& operator = (const int64_t& rop) { value = rop; return *this; }
		inline ExtendedMetricTime& operator = (const HumanTime& time);

		/* c-tors */
		inline constexpr ExtendedMetricTime() : value(0) {}
		inline constexpr ExtendedMetricTime(int64_t value) : value(value) {}	// implicit conversion int64 ->  EMT
		inline ExtendedMetricTime(const HumanTime& time, const HumanTime& perspective = CHRIST_0);

		//inline int64_t& inner_value()								{	return value;	}

			/* setter (supposedly for time differences) */
		inline void setFromSeconds(int32_t seconds) { value = (1LL << 16) * seconds; }
		inline void setFromMinutes(int16_t minutes) { value = (1LL << 16) * 60 * minutes; }
		inline void setFromHours(int16_t hours) { value = (1LL << 16) * 60 * 60 * hours; }
		inline void setFromDays(int16_t days) { value = (1LL << 16) * 60 * 60 * 24 * days; }

		void set_from(const HumanTime& time, const HumanTime& perspective = CHRIST_0);

		/* factories */
		inline static ExtendedMetricTime seconds_to_EMT(int32_t seconds) { return ExtendedMetricTime((1LL << 16) * seconds); }
		inline static ExtendedMetricTime minutes_to_EMT(int16_t minutes) { return ExtendedMetricTime((1LL << 16) * 60 * minutes); }
		inline static ExtendedMetricTime hours_to_EMT(int16_t hours) { return ExtendedMetricTime((1LL << 16) * 60 * 60 * hours); }
		inline static ExtendedMetricTime days_to_EMT(int16_t days) { return ExtendedMetricTime((1LL << 16) * 60 * 60 * 24 * days); }

		/* arithmetic operators */
/*		inline friend ExtendedMetricTime operator + (const ExtendedMetricTime&, const ExtendedMetricTime&);
		inline friend ExtendedMetricTime operator - (const ExtendedMetricTime&, const ExtendedMetricTime&);
		template <typename T>
		inline friend ExtendedMetricTime operator * (ExtendedMetricTime, T);
		template <typename T>
		inline friend ExtendedMetricTime operator * (T, ExtendedMetricTime);
		template <typename T>
		inline friend ExtendedMetricTime operator / (ExtendedMetricTime, T);
		template <typename T>
		inline friend void div(const ExtendedMetricTime&, const ExtendedMetricTime, T&);
*/
		/* comparison operators */
		inline friend bool operator == (const ExtendedMetricTime&, const ExtendedMetricTime&);
		inline friend bool operator <  (const ExtendedMetricTime&, const ExtendedMetricTime&);
		inline friend bool operator >  (const ExtendedMetricTime&, const ExtendedMetricTime&);
		inline friend bool operator <= (const ExtendedMetricTime&, const ExtendedMetricTime&);
		inline friend bool operator >= (const ExtendedMetricTime&, const ExtendedMetricTime&);

		/// <<< add some trunc or round function
	};


}



/************************************************************************/
/* FUNCTION / OPERATOR DECLARATION
                                                                        */
/************************************************************************/

/* Human Time arithmetic operators */
inline auto operator + (const time::HumanTime&)								-> const time::HumanTime&;
auto operator - (const time::HumanTime&)								-> const time::HumanTime& = delete; // its clear why it is what it is - but why is it written down here?
inline auto operator + (const time::ExtendedMetricTime&, const time::HumanTime&)	-> time::HumanTime;
inline auto operator + (const time::HumanTime&, const time::ExtendedMetricTime&)	-> time::HumanTime;
inline auto operator - (const time::HumanTime&, const time::HumanTime&)				-> time::ExtendedMetricTime;
inline auto operator - (const time::HumanTime&, const time::ExtendedMetricTime&)	-> time::HumanTime;


/* f-like transcription operators, = operator must be a non-static member function, therefore it cannot be used */
inline void operator << (time::Month& lop, uint8_t rop)		{ lop = time::HumanTime::int_to_month(rop); }
inline void operator >> (time::Month lop, uint8_t& rop)		{ rop = time::HumanTime::month_to_int(lop); }
inline void operator << (uint8_t& lop, time::Month rop)		{ return rop>>lop; }
inline void operator >> (uint8_t lop, time::Month& rop)		{ return rop<<lop; }
// <<< could be splitted but not now <<<<<<

/* Extended Metric Time comparison operators */
inline bool operator == (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
inline bool operator <  (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
inline bool operator >  (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
inline bool operator <= (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
inline bool operator >= (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);

/* Extended Metric Time arithmetic operators */
inline time::ExtendedMetricTime operator + (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
//inline time::HumanTime operator + (const time::ExtendedMetricTime&, const time::HumanTime&);
inline time::ExtendedMetricTime operator - (const time::ExtendedMetricTime&, const time::ExtendedMetricTime&);
template <typename T>
inline time::ExtendedMetricTime operator * (time::ExtendedMetricTime, T);
template <typename T>
inline time::ExtendedMetricTime operator * (T, time::ExtendedMetricTime);
template <typename T>
inline time::ExtendedMetricTime operator / (time::ExtendedMetricTime, T);
template <typename T>
inline void div(const time::ExtendedMetricTime&, const time::ExtendedMetricTime, T&);
	


/************************************************************************/
/* INLINE FUNCTION IMPLEMENTATION
                                                                        */
/************************************************************************/

inline time::Month& operator++(time::Month& op){
	return op = static_cast<time::Month>(static_cast<uint8_t>(op) + 1 % 12);
}

inline time::Month& operator--(time::Month& op){
	return op = static_cast<time::Month>(static_cast<uint8_t>(op) + 11 % 12);
}

// time::HumanTime Operators:
inline auto operator + (const time::HumanTime& op)								-> time::HumanTime const& {
	return op;
}

inline auto operator + (const time::ExtendedMetricTime& lop, const time::HumanTime& rop)	-> time::HumanTime {
	return time::HumanTime(lop + time::ExtendedMetricTime(rop, time::ExtendedMetricTime::CHRIST_0));
}

inline auto operator + (const time::HumanTime& lop, const time::ExtendedMetricTime& rop)	-> time::HumanTime {
	return rop + lop;
}

inline auto operator - (const time::HumanTime& lop, const time::HumanTime& rop)				-> time::ExtendedMetricTime {
	return time::ExtendedMetricTime(lop) - time::ExtendedMetricTime(rop);
}

inline auto operator - (const time::HumanTime& lop, const time::ExtendedMetricTime& rop)	-> time::HumanTime {
	return time::HumanTime(time::ExtendedMetricTime(lop,time::ExtendedMetricTime::CHRIST_0) - rop);
}

// EMT constructors and operators
inline time::ExtendedMetricTime& time::ExtendedMetricTime::operator = (const time::HumanTime& time) {
	set_from(time,CHRIST_0);
	return *this;
}

inline time::ExtendedMetricTime::ExtendedMetricTime(const time::HumanTime& time, const time::HumanTime& perspective){
	set_from(time, perspective);
}


// notice: constexpr functions are implicitly inline!
bool constexpr time::HumanTime::isLeapYear(int16_t year){ // ready, checked
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

inline bool operator == (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop)		{ return lop.value == rop.value; }
inline bool operator <  (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop)		{ return lop.value <  rop.value; }
inline bool operator >  (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop)		{ return lop.value >  rop.value; }
inline bool operator <= (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop)		{ return lop.value <= rop.value; }
inline bool operator >= (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop)		{ return lop.value >= rop.value; }

inline time::ExtendedMetricTime operator + (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop) { return lop.value + rop.value; }
inline time::ExtendedMetricTime operator - (const time::ExtendedMetricTime& lop, const time::ExtendedMetricTime& rop) { return lop.value - rop.value; }
template <typename T>
inline time::ExtendedMetricTime operator * (time::ExtendedMetricTime emt, T factor) { return emt.value * static_cast<int64_t>(factor); }
template <typename T>
inline time::ExtendedMetricTime operator * (T factor, time::ExtendedMetricTime emt) { return emt.value * static_cast<int64_t>(factor); }
template <typename T>
inline time::ExtendedMetricTime operator / (time::ExtendedMetricTime emt, T divisor) { return emt.value / static_cast<int64_t>(divisor); }
template <typename T>
inline void div(const time::ExtendedMetricTime& dividend, const time::ExtendedMetricTime divisor, T& quotient) { quotient = static_cast<T>(dividend.value / divisor.value); }

#endif /* F_TIME_H_ */





/* ideas or conceptional stuff stored here: */

//<<< refactor: provide a new Metric Time which works as EMT but has no extendency means there are no divisions of seconds
/* we don't need such a time class right now. just providing EMT and HT should be enough.
class ExtendedTime { // should be exttime::HumanTime !!! 
public:
	time::HumanTime time;	
	uint16_t divisions_of_second;
	
	ExtendedTime(const ExtendedTime& extTime) = default; // copy constructor
	ExtendedTime(const ExtendedTime&& extTime) = delete;
	ExtendedTime(const HumanTime& time) : time(time), divisions_of_second(0) {} // conversion constructor for Time
	ExtendedTime(const ExtendedMetricTime& time); // conversion from EMT
	
	inline void normalize(){ time.normalize(); }
	
	//operator == != < > <= >= + - 
	
};
*/