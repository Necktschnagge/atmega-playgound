/*
 * f_maybe.h
 *
 * Created: 28.07.2018 14:28:43
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_MAYBE_H_
#define F_MAYBE_H_

namespace fsl {
	
	namespace str {
		
		template <typename T>
		class maybe {
			bool _valid;
			uint8_t object_space[sizeof(T)];
			
			public:
			inline maybe() : _valid(false) {}
			inline maybe(T&& object) : _valid(true) {
				T& t { reinterpret_cast<T&>(object_space[0]) };
				t = object;
			}
			
			inline bool valid() const { return _valid; }
			inline operator bool () const { return _valid; }
			inline T& operator*() { return reinterpret_cast<T&>(object_space[0]); }
			inline const T& operator*() const { return reinterpret_cast<const T&> (object_space[0]); }
		};
	}
}



#endif /* F_MAYBE_H_ */