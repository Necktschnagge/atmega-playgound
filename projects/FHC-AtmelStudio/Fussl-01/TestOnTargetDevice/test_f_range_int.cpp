/*
 * test_f_range_int.cpp
 *
 * Created: 28.07.2019 20:26:42
 *  Author: F-NET-ADMIN
 */ 

#include "test_f_range_int.h"
#include "f_test_logger.h"
#include "f_static_task_runner.h"

#include "../Fussl-01/f_range_int.h" // to be tested

#include <inttypes.h>

using namespace fsl::ver_1_0::lg;

using task_runner = callback_traits<void(*)(f_test_logger&)>;

namespace test {
	namespace classes {
		namespace base_type_constants {
			namespace objects {
				namespace TEMPLATE {
					template<class T, T RANGE>
					inline void all(f_test_logger& test_logger){
						test_logger.check("ZERO"        , static_cast<T>( range_int<T,RANGE>::base_type_constants::ZERO				) == 0      ); // replace tabs with spaces except begin of line!!!
						test_logger.check("ONE"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::ONE				) == 1      );
						test_logger.check("RANGE"       , static_cast<T>( range_int<T,RANGE>::base_type_constants::RANGE			) == RANGE	);
						test_logger.check("MIN"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::MIN				) == 0		);
						test_logger.check("MAX"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::MAX + 1			) == RANGE	);
						test_logger.check("OUT_OF_RANGE", static_cast<T>( range_int<T,RANGE>::base_type_constants::OUT_OF_RANGE + 1	) == 0		);
					}
				}
				inline void all(f_test_logger& test_logger){
					test_logger.scoped("base_type_constants", [&]{
						test_logger.scoped("1",[&]{TEMPLATE::all<uint16_t,785>(test_logger);					});
						test_logger.scoped("2",[&]{TEMPLATE::all<int32_t,1>(test_logger);						});
						test_logger.scoped("3",[&]{TEMPLATE::all<uint8_t,255>(test_logger);						});
						test_logger.scoped("4",[&]{TEMPLATE::all<int64_t,1'000'000'000'583LL>(test_logger);		});
						test_logger.scoped("5",[&]{TEMPLATE::all<int16_t,23>(test_logger);						});
					});
				}
				using runner = task_runner::static_task_runner<all>;
			}
			using runner = objects::runner;
		}
		using runner = base_type_constants::runner;
	}
	//ready until here...
	
	
	namespace types {
		namespace base_type {
			//template<class T, T RANGE>
			inline void all(f_test_logger& test_logger){
				//bool x; //###
			}
			using runner = typename task_runner::static_task_runner<all>;
		}
		using runner = base_type::runner;
	}
	using runner = concat<classes::runner, types::runner>;
	
}

template<class T>
inline void test_standard_constructor_with(f_test_logger& test_runner){
	range_int<T, 16> ri;
	test_runner.check("The standard constructor creates a ZERO value.", range_int<T ,16>::base_type_constants::ZERO == ri.to_base_type());
}

inline void test_standard_constructor(f_test_logger& test_runner){
	test_standard_constructor_with< int8_t>(test_runner);
	test_standard_constructor_with< int16_t>(test_runner);
	test_standard_constructor_with< int32_t>(test_runner);
	test_standard_constructor_with<uint8_t>(test_runner);
	test_standard_constructor_with<uint16_t>(test_runner);
	test_standard_constructor_with<uint32_t>(test_runner);
}



void run_test_f_range_int(f_test_logger& test_runner){
	test::classes::base_type_constants::objects::all(test_runner);
#if false //debug
	test_standard_constructor(test_runner);
	range_int<uint8_t,1> r;
	test_runner.check("Standard constructed range int equals zero.", r == 0 );
#endif
	
	// ### check if we can oplug anything onto the simulator: iopins of the controller??
}

