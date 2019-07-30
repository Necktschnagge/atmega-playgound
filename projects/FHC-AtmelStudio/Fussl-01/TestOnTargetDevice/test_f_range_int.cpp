/*
 * test_f_range_int.cpp
 *
 * Created: 28.07.2019 20:26:42
 *  Author: F-NET-ADMIN
 */ 

#include "test_f_range_int.h"
#include "f_range_int.h"
#include "f_target_device_test.h"

#include <inttypes.h>

using namespace fsl::ver_1_0::lg;

void test_base_type(f_target_device_test& test_runner){
/*	test_runner.check("The base_type is defined as _base_type, using uint8_t.",  typeid(range_int<uint8_t ,1>::base_type) == typeid(uint8_t));
	test_runner.check("The base_type is defined as _base_type, using uint16_t.", typeid(range_int<uint16_t,1>::base_type) == typeid(uint16_t));
	test_runner.check("The base_type is defined as _base_type, using uint32_t.", typeid(range_int<uint32_t,1>::base_type) == typeid(uint32_t));
	test_runner.check("The base_type is defined as _base_type, using uint64_t.", typeid(range_int<uint64_t,1>::base_type) == typeid(uint64_t));
	test_runner.check("The base_type is defined as _base_type, using int8_t.",  typeid(range_int<int8_t ,1>::base_type) == typeid(int8_t));
	test_runner.check("The base_type is defined as _base_type, using int16_t.", typeid(range_int<int16_t,1>::base_type) == typeid(int16_t));
	test_runner.check("The base_type is defined as _base_type, using int32_t.", typeid(range_int<int32_t,1>::base_type) == typeid(int32_t));
	test_runner.check("The base_type is defined as _base_type, using int64_t.", typeid(range_int<int64_t,1>::base_type) == typeid(int64_t));
	*/
}

void test_standard_constructor(f_target_device_test& test_runner){
	range_int<uint8_t ,1> a;
	range_int<uint16_t,1> b;
	range_int<uint32_t,1> c;
	range_int<uint64_t,1> d;
	range_int<int8_t ,1>  e;
	range_int<int16_t,1>  f;
	range_int<int32_t,1>  g;
	range_int<int64_t,1>  h;
	test_runner.check("The standard constructor creates a ZERO value, using uint8_t .", a == range_int<uint8_t ,1>::base_type_constants::ZERO);
	test_runner.check("The standard constructor creates a ZERO value, using uint16_t.", b == range_int<uint16_t,1>::base_type_constants::ZERO);
	test_runner.check("The standard constructor creates a ZERO value, using uint32_t.", c == range_int<uint32_t,1>::base_type_constants::ZERO);
	test_runner.check("The standard constructor creates a ZERO value, using uint64_t.", d == range_int<uint64_t,1>::base_type_constants::ZERO);
}

void run_test_f_range_int(f_target_device_test& test_runner){
	test_base_type(test_runner);
	test_standard_constructor(test_runner);
	range_int<uint8_t,1> r;
	test_runner.check("Standard constructed range int equals zero.", r == 0 );
	
	// ### check if we can oplug anything onto the simulator: iopins of the controller??
}

