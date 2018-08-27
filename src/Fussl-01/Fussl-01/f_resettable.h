/*
* f_resettable.h
*
* Created: 25.08.2018 20:58:21
*  Author: F-NET-ADMIN
*/


#ifndef F_RESETTABLE_H_
#define F_RESETTABLE_H_

namespace fsl {
	
	namespace str {
		
		template<typename T, T _RESET_VALUE>
		class resettable {
			T t;
			public:
			static constexpr T RESET_VALUE{ _RESET_VALUE };
			
			inline void reset(){ t = RESET_VALUE; }
			inline operator T& () { return t; }
			inline T& operator+ () { return t; }
		};

	}
}


#endif /* F_RESETTABLE_H_ */