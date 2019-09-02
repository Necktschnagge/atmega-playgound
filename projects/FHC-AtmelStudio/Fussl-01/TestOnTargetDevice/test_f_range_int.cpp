/*
 * test_f_range_int.cpp
 *
 * Created: 28.07.2019 20:26:42
 *  Author: F-NET-ADMIN
 */ 

#include "test_f_range_int.h"
#include "f_range_int.h"
#include "f_test_runner.h"

#include <inttypes.h>

using namespace fsl::ver_1_0::lg;

template<class T, T RANGE>
inline void test_base_type_constants_with(f_test_runner& test_runner){
	test_runner.check("ZERO"        , range_int<T,RANGE>::base_type_constants::ZERO == 0            );
	test_runner.check("ONE"         , range_int<T,RANGE>::base_type_constants::ONE == 1             );
	test_runner.check("MIN"         , range_int<T,RANGE>::base_type_constants::MIN == 0             );
	test_runner.check("MAX"         , range_int<T,RANGE>::base_type_constants::MAX + 1 == RANGE     );
	test_runner.check("RANGE"       , range_int<T,RANGE>::base_type_constants::RANGE == RANGE       );
	test_runner.check("OUT_OF_RANGE", static_cast<T>(range_int<T,RANGE>::base_type_constants::OUT_OF_RANGE + 1) == 0);
}

inline void test_base_type_constants(f_test_runner& test_runner){
	test_runner.enter_scope("base_type_constants");
	
		test_runner.enter_scope("1");
			test_base_type_constants_with<uint16_t,785>(test_runner);
		test_runner.leave_scope();
		
		test_runner.enter_scope("2");
			test_base_type_constants_with<int32_t,1>(test_runner);
		test_runner.leave_scope();
		
		test_runner.enter_scope("3");// this is leading to an error, only if the base_type_constants scope is set, otherwise not.
			test_base_type_constants_with<uint8_t,255>(test_runner);
		test_runner.leave_scope();
		
		test_runner.enter_scope("4");
			test_base_type_constants_with<int64_t,1'000'000'000'583LL>(test_runner);
		test_runner.leave_scope();
		
	test_runner.leave_scope();
}


template<class T>
inline void test_standard_constructor_with(f_test_runner& test_runner){
	range_int<T, 16> ri;
	test_runner.check("The standard constructor creates a ZERO value.", range_int<T ,16>::base_type_constants::ZERO == ri.to_base_type());
}

inline void test_standard_constructor(f_test_runner& test_runner){
	test_standard_constructor_with< int8_t>(test_runner);
	test_standard_constructor_with< int16_t>(test_runner);
	test_standard_constructor_with< int32_t>(test_runner);
	test_standard_constructor_with<uint8_t>(test_runner);
	test_standard_constructor_with<uint16_t>(test_runner);
	test_standard_constructor_with<uint32_t>(test_runner);
}



void run_test_f_range_int(f_test_runner& test_runner){
	test_base_type_constants(test_runner);
#if false //debug
	test_standard_constructor(test_runner);
	range_int<uint8_t,1> r;
	test_runner.check("Standard constructed range int equals zero.", r == 0 );
#endif
	
	// ### check if we can oplug anything onto the simulator: iopins of the controller??
}

