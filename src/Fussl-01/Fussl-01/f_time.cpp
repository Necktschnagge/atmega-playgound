/*
 * f_time.cpp
 *
 * Created: 04.05.2017 14:28:49
 *  Author: Maximilian Starke
 
 * FPS: see at header file.
 */ 

#include "f_time.h"

namespace { // compilation unit local constants
	
		// 400 years in days:
	constexpr int64_t c_400_years_in_days {400ll * 365 /*normal year*/ + 100 * 1 /*leap years*/ - 3 /*no leap years: 100, 200, 300*/ };
		// [++value]s of an ExtendedMetricTime for one day:
	constexpr int64_t emt_step_per_day { 24LL * 60 * 60 * (1LL << 16) };
}

bool time::HumanTime::Date::is_valid(bool isLeapYear) const {
	if ((0 <= static_cast<uint8_t>(month)) && (static_cast<uint8_t>(month) <12)){
		return day < getMonthLength(month,isLeapYear);
	} else {
		return false;
	}
}

bool time::HumanTime::HDate::is_valid(bool isLeapYear) const {
	if ((month==0) || (month > 12)) return false;
	return ((day != 0) || (day <= getMonthLength(int_to_month(month),isLeapYear)));
}


time::HumanTime::HumanTime(const ExtendedMetricTime& emt){
	int64_t value = emt.inner_value();
				//divisions_of_second = value % 0x10000;
				//value -= divisions_of_second;
	value = value >> 16;
	second = value % 60;
	value -= second; // this is necessary because of negative numbers
	value /= 60;	// int div would otherwise do -7 : 2 = -3 instead of -8 :2 = - 4;
	minute = value % 60;
	value -= minute;
	value /= 60;
	hour = value % 24;
	value -= hour;
	value /= 24;
	// now we have time remaining in days...
	// first check for applicable 400 years...
	year = 400 * (  ( value - (value % c_400_years_in_days) ) / c_400_years_in_days  );
	value -= year * c_400_years_in_days;
	// 0 <= value < factor400 assert <<<<<
	while (value >= daysOfYear()){
		value -= daysOfYear();
		++year;
	}
	day = value;
}

void time::HumanTime::normalize(){
	// clear seconds:
	minute += (second - (second % 60)) / 60;
	second = second % 60;
	// clear minutes:
	hour += (minute -(minute % 60)) / 60;
	minute %= 60;
	// clear hour:
	day += (hour - (hour % 24)) / 24;
	hour %= 24;
	// clear day;
again:
	if (day >= daysOfYear()) {
		day -= daysOfYear();
		++year;
		goto again;
		//return normalize(); // a goto would be better
	}
again2:
	if (day < 0){
		--year;
		day += daysOfYear();
		//return normalize();
		goto again2;
	}
}

uint8_t time::HumanTime::getMonthLength(Month month, bool isLeapYear){ // ready, checked
	if (month == Month::FEB) return 28 + isLeapYear;
	return time::HumanTime::__monthLength__[static_cast<uint8_t>(month)];
}

time::HumanTime::Date time::HumanTime::getDate() const { // ready, checked
	int16_t d {day};
	Month m {Month::JAN};
	while (d >= this->getMonthLength(m)){
		d -= this->getMonthLength(m);
		++m;
	}
	return Date(m, static_cast<uint8_t>(d), true);
}

bool time::HumanTime::setDate(const time::HumanTime::Date& date){
	if (   !date.is_valid( this->isLeapYear() )   ) return false;
	this->day = 0; // 01.01.
	for (Month m = Month::JAN; m != date.month; ++m){
		this->day += getMonthLength(m);
	}
	return true;
}

time::Day time::HumanTime::getDayOfWeek() const { // ready, not to check but to be tested
	/* via extended version of Gauss' formula (Georg Glaeser) */
	Date cdate = getDate();
	int16_t cyear = this->year - (month_to_int(cdate.month) < 3);
	uint8_t cday = cdate.day;
	--cdate.month;
	--cdate.month;
	uint16_t shiftedMonth = month_to_int(cdate.month);
	uint8_t llyear = cyear % 100;
	int16_t uuyear = cyear / 100;
	
	return static_cast<Day>((cday + ((26*shiftedMonth-2) / 10 ) + llyear + (llyear / 4) + (uuyear / 4) -2*uuyear) % 7);
	//return Day::MON;
}

bool time::HumanTime::operator == (const HumanTime& rop) const {
	return (rop.second == second) && (rop.minute == minute) && (rop.hour == hour) && (rop.day == day) && (rop.year == year);
}

bool time::HumanTime::operator < (const HumanTime& rop) const {
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

void time::ExtendedMetricTime::set_from(const time::HumanTime& time, const time::HumanTime& perspective){
	if (perspective == CHRIST_0) {
		value = (1LL<<16) * (60LL * (60LL * (24LL * (time.get_day()) + time.get_hour()) + time.get_minute()) + time.get_second());
		int16_t year = time.get_year();
		value += emt_step_per_day * c_400_years_in_days * ( (year - year % 400) / 400);
					// -600 -> -800 -> -2 instead of -600 -> -1
		year %= 400; // this remaining we must add in ticks
		while(year > 0){
			--year;
			value += emt_step_per_day * time::HumanTime::daysOfYear(year);
		}
	} else {
		*this = ExtendedMetricTime(time,CHRIST_0) - ExtendedMetricTime(perspective,CHRIST_0);
	}
}


/************************************************************************/
/* EMT class                                                            */
/************************************************************************/

/* nothing to implement */

/************************************************************************/
/* ideas, conceptual stuff
	historical code
                                                                        */
/************************************************************************/


/*

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
*/

