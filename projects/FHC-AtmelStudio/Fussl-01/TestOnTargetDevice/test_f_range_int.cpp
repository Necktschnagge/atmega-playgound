/*
 * test_f_range_int.cpp
 *
 * Created: 28.07.2019 20:26:42
 *  Author: F-NET-ADMIN
 */ 

#include "f_type_traits.h"

#include "test_f_range_int.h"
#include "f_test_logger.h"
#include "f_static_task_runner.h"

#include "../Fussl-01/f_range_int.h" // to be tested

#include <inttypes.h>

using namespace fsl::ver_1_0::lg;

using task_runner = callback_traits<void(*)(f_test_logger&)>;


#define WRAP_FUNCTION(SCOPE_NAME, CALLBACK) inline void __all(f_test_logger& test_logger){ return test_logger.scoped(SCOPE_NAME,[&]{ CALLBACK(test_logger); }); } using runner = task_runner::static_task_runner<__all>
#define WRAP_RUNNER(SCOPE_NAME, CALLBACK) inline void __all(f_test_logger& test_logger){ return test_logger.scoped(SCOPE_NAME,[&]{ CALLBACK::run<f_test_logger&>(test_logger); }); } using runner = task_runner::static_task_runner<__all>

namespace test {
	namespace class_range_int {
		namespace classes {
			namespace base_type_constants {
				namespace member_constants {
					namespace test_templates {
						template<class T, T RANGE>
						inline void all(f_test_logger& test_logger){
							test_logger.check("ZERO"        , static_cast<T>( range_int<T,RANGE>::base_type_constants::ZERO             ) == 0      );
							test_logger.check("ONE"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::ONE              ) == 1      );
							test_logger.check("RANGE"       , static_cast<T>( range_int<T,RANGE>::base_type_constants::RANGE            ) == RANGE  );
							test_logger.check("MIN"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::MIN              ) == 0      );
							test_logger.check("MAX"         , static_cast<T>( range_int<T,RANGE>::base_type_constants::MAX + 1          ) == RANGE  );
							test_logger.check("OUT_OF_RANGE", static_cast<T>( range_int<T,RANGE>::base_type_constants::OUT_OF_RANGE + 1 ) == 0      );
						}
					} // namespace test_template
					inline void all(f_test_logger& test_logger){
						test_logger.scoped("member_constants", [&]{
							test_logger.scoped("1",[&]{test_templates::all<uint16_t,785>(test_logger);                    });
							test_logger.scoped("2",[&]{test_templates::all<int32_t,1>(test_logger);                       });
							test_logger.scoped("3",[&]{test_templates::all<uint8_t,255>(test_logger);                     });
							test_logger.scoped("4",[&]{test_templates::all<int64_t,1'000'000'000'583LL>(test_logger);     });
							test_logger.scoped("5",[&]{test_templates::all<int16_t,23>(test_logger);                      });
						});
					}
					using runner = task_runner::static_task_runner<all>;
				} // namespace member_constants
				WRAP_RUNNER("base_type_constants", member_constants::runner);
			} // namespace base_type_constants
			WRAP_RUNNER("classes",base_type_constants::runner);
		} // namespace classes
		namespace types {
			template<class _BASE_TYPE>
			inline void test_base_type(f_test_logger& test_logger){
				test_logger.check("base_type",fsl::ver_1_0::is_same<   typename range_int<_BASE_TYPE,10,false,false>::base_type,   _BASE_TYPE   >::value);
			}
			inline void all(f_test_logger& test_logger){
				test_logger.scoped("types",[&]{
					test_base_type<uint8_t>(test_logger);
					test_base_type<int16_t>(test_logger);
					test_base_type<uint64_t>(test_logger);
					test_base_type<int32_t>(test_logger);
				});
			}
			using runner = typename task_runner::static_task_runner<all>;
		} // namespace types
		namespace public_member_functions {
			namespace constructors {
				
			}
		}
		using runner = concat<classes::runner, types::runner>;
	}
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



void run_test_f_range_int(f_test_logger& test_logger){
	return test_logger.scoped("range_int",[&]{
		test::class_range_int::runner::run<f_test_logger&>(test_logger); // review comment: check if we can add a std::ref to our standard lib and call run(fsl::ver_1_0::ref(test_logger)) instead of specifiying function template.
	#if false //debug
		test_standard_constructor(test_logger);
		range_int<uint8_t,1> r;
		test_logger.check("Standard constructed range int equals zero.", r == 0 );
	#endif
	});
}

