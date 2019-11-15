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
				test_logger.check("base_type",fsl::ver_1_0::is_same<   typename range_int<_BASE_TYPE,10>::base_type,   _BASE_TYPE   >::value);
			}
			inline void all(f_test_logger& test_logger){
				test_logger.scoped("types",[&]{
					test_base_type<uint8_t>(test_logger);
					test_base_type<int16_t>(test_logger);
					test_base_type<uint64_t>(test_logger);
					test_base_type<int32_t>(test_logger);
				});
			}
			using runner = task_runner::static_task_runner<all>;
		} // namespace types
		namespace public_member_functions {
			namespace constructors {
				namespace test_templates {
					template<typename T, T RANGE>
					inline void test_standard_constructor(f_test_logger& test_logger){
						test_logger.scoped("standard-ctor",[&]{
							range_int<T,RANGE> x;
							test_logger.check("std-ctor zero",x.to_base_type() == T(0));
						});
					}
					template<class T, T RANGE>
					inline void test_conversion_constructor(f_test_logger& test_logger, T TEST_END, T TEST_INC){
						test_logger.scoped("conv-ctor",[&]{
							using tt = range_int<T,RANGE>;
							tt x(0);
							test_logger.check("convert\"0\"",x.to_base_type()==0);
							T last{ 0 };
							for (T input = 1; input < TEST_END; input += TEST_INC){
								if (input <= last) break;
								last = input;
								tt y(input);
								if (input < RANGE){
									test_logger.check("in range", y.to_base_type() == input);
								} else {
									test_logger.check("out of range", y.to_base_type() == tt::base_type_constants::OUT_OF_RANGE);
								}
							}
						});
					}
					inline bool raw_compare(void const * l, void const* r, size_t size){
						const uint8_t* ll = static_cast<const uint8_t*>(l);
						const uint8_t* rr = static_cast<const uint8_t*>(r);
						for(size_t i = 0; i < size; ++i) if (ll[i] != rr[i]) return false;
						return true;
					}
					//##test conversion between different raneg_int's
					template<class T, T RANGE>
					inline void test_copy_constructor(f_test_logger& test_logger, T TEST_END, T TEST_INC){
						test_logger.scoped("copy-ctor",[&]{
							using tt = range_int<T,RANGE>;
							T last{ 0 };
							for (T input = 1; input < TEST_END; input += TEST_INC){
								if (input <= last) break;
								last = input;
								tt original(input);
								typename tt::base_type o_value = original;
								tt copy(original);
								typename tt::base_type c_value = copy;
								test_logger.check("int",o_value == c_value);
								test_logger.check("raw",raw_compare(&original,&copy,sizeof(tt)));
							}
						});
					}
					template<class T, T RANGE>
					inline void all(f_test_logger& test_logger, T MAX){
						auto inc = MAX / 14;
						auto end = MAX;
						test_standard_constructor<T,RANGE>(test_logger);
						test_conversion_constructor<T,RANGE>(test_logger,end,inc);
						test_copy_constructor<T,RANGE>(test_logger,end, inc);
					}
				} // namespace test_templates
				inline void all(f_test_logger& test_logger){
						test_logger.scoped("constructors",[&]{
						test_templates::all<uint8_t,46>(test_logger,100);
						test_templates::all<uint16_t,340>(test_logger,874);
						test_templates::all<uint32_t,0x7D'C5'73'11>(test_logger,0xFFFF'FFFF);
						test_templates::all<uint64_t,0x4231'DE81'0AC9>(test_logger,0xBB31'BC97'0AC3);
						test_templates::all<int8_t,127>(test_logger,127);
						test_templates::all<int32_t,30'111>(test_logger,32'431);
					});
				}
				using runner = task_runner::static_task_runner<all>;
			} // namespace constructors
			namespace non_constructors {
				/*
				template<class T>
				using conversion = T (*)();
				template<class T, T RANGE>
				inline void generic_test_to_base_type_conversion(f_test_logger& test_logger, conversion<T> T::* function, T value){
					
					range_int<T,RANGE> o(value);
					//test_logger.check("c2b", value == o.)
					o.to_base_type();
					
				}
				inline void test_operator_base_type(f_test_logger& test_logger){
					
				}*/
				inline void all(f_test_logger& test_logger){
					
				}
				using runner = task_runner::static_task_runner<all>;
			} // namespace non_constructors
			using unwrapped_runner = concat<constructors::runner,non_constructors::runner>;
			WRAP_RUNNER("public member functions", unwrapped_runner);
		} // namespace public_member_functions
		using runner = concat<classes::runner, types::runner, public_member_functions::runner>;
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

/// ### make doxygen document also private stuff for an "internal doc"

