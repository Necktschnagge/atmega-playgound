/*
 * f_callbacks.h
 *
 * Created: 27.06.2018 16:44:54
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_CALLBACKS_H_
#define F_CALLBACKS_H_

namespace fsl {
	namespace str {
		
		using void_function = void (*)();
		
		class callable {
			public:
			virtual void operator()() = 0;
		};
		
		class const_callable {
			public:
			virtual void operator()() const = 0;
		};
		
	}
}



#endif /* F_CALLBACKS_H_ */