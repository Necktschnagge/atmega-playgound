/* 
* my_template_test.h
*
* Created: 05.11.2018 13:58:16
* Author: F-NET-ADMIN
*/


#ifndef __MY_TEMPLATE_TEST_H__
#define __MY_TEMPLATE_TEST_H__

#include <stdint.h>
class my_template_test
{
//variables
public:
protected:
private:


//functions
public:

						template <uint8_t x>
						static constexpr uint8_t go() { return x + 1; }

						



	my_template_test();
	~my_template_test();
protected:
private:
	my_template_test( const my_template_test &c );
	my_template_test& operator=( const my_template_test &c );

}; //my_template_test

template <>
constexpr uint8_t my_template_test::go<255>() { return 20; }

#endif //__MY_TEMPLATE_TEST_H__
