/* 
* f_bytewise.h
*
* Created: 15.07.2018 20:33:41
* Author: F-NET-ADMIN
*/


#ifndef __F_BYTEWISE_H__
#define __F_BYTEWISE_H__

#include <stdint.h>

namespace fsl {
	namespace util {
		
		template <typename T>
		void byte_swap(T& a, T& b){
			uint8_t swap_byte;
			uint8_t* p_a{ reinterpret_cast<uint8_t*>(&a) };
			uint8_t* p_b{ reinterpret_cast<uint8_t*>(&b) };
			for (decltype(sizeof(T)) i = 0; i < sizeof(T); ++i){
				swap_byte = a[i];
				a[i] = b[i];
				b[i] = swap_byte;
			}
		}
	}
}
#endif //__F_BYTEWISE_H__
