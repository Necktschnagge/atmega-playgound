/* 
* f_bytewise.h
*
* Created: 15.07.2018 20:33:41
* Author: Maximilian Starke
*/


#ifndef __F_BYTEWISE_H__
#define __F_BYTEWISE_H__

#include <stdint.h>

namespace fsl {
	namespace util {
		
		template <typename T>
		inline void byte_swap(T& a, T& b){
			uint8_t swap_byte;
			uint8_t* p_a{ reinterpret_cast<uint8_t*>(&a) };
			uint8_t* p_b{ reinterpret_cast<uint8_t*>(&b) };
			for (decltype(sizeof(T)) i = 0; i < sizeof(T); ++i){
				swap_byte = a[i];
				a[i] = b[i];
				b[i] = swap_byte;
			}
		}
		
		template <typename T>
		inline void normal_swap(T& a, T&b){
			T copy = a;
			a = b;
			b = copy;
		}
		
		template <typename T>
		inline void byte_copy(const volatile T& source, volatile T& destination){
			const volatile uint8_t* p_source { reinterpret_cast<const volatile uint8_t*>(&source) };
			volatile uint8_t* p_destination { reinterpret_cast<volatile uint8_t*>(&destination) };
			for (decltype(sizeof(T)) i = 0; i < sizeof(T); ++i){
				*(p_destination + i) = *(p_source + i);
			}
		}
	}
}
#endif //__F_BYTEWISE_H__
