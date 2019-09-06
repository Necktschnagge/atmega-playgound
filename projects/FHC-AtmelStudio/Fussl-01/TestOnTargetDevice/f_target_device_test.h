/* 
* f_target_device_test.h
*
* Created: 28.07.2019 20:39:38
* Author: F-NET-ADMIN
*/


#ifndef __F_TARGET_DEVICE_TEST_H__
#define __F_TARGET_DEVICE_TEST_H__

#include <avr/io.h>

class f_target_device_test
{
//variables
public:
protected:
private:

//functions
public:
	f_target_device_test();
	~f_target_device_test();
	f_target_device_test( const f_target_device_test &c ) = delete;
	f_target_device_test& operator=( const f_target_device_test &c ) = delete;

	inline void check(const char* description, bool claim){
		if (! claim){
			DDRA |= 1;
		}
	}

}; //f_target_device_test

#endif //__F_TARGET_DEVICE_TEST_H__
