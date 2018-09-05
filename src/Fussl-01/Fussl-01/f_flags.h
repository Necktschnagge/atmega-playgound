/*
* f_flags.h
*
* Created: 27.06.2018 15:10:32
* Author: F-NET-ADMIN

FPS:

should be ready:
add all doc-strings!!
write some testcode somewhen in future

look at the last lines, there are two usings,
check whether the code gets more or less depending on what implementation we choose.
!!! notice: the rangeint was not applied to the old implementation.


still: the new implementation uses range int but does never check for OUT_OF_RANGE
-> just write in explanation: behaviour undef in this case:
later check whether code gets more if we add checks in all functions

*/


#ifndef __F_FLAGS_H__
#define __F_FLAGS_H__

#include <stdint.h>

#include "f_range_int.h"

namespace fsl {
	namespace lg {
		
		#if false
		class single_flags_old_impl {
			private:
			uint8_t memory;
			public:
			using flag_id = uint8_t;
			
			/* c-tors */
			inline single_flags_old_impl(){ memory = 0; }
			inline single_flags_old_impl(uint8_t data) { memory = data; }
			
			/* accessors */
			inline bool get(flag_id bit){
				return memory & (static_cast<uint8_t>(1) << bit);
			}
			inline void set_true (flag_id bit){
				memory |= 1 << bit;
			}
			inline void set_false (flag_id bit){
				memory &= ~(1 << bit);
			}
			inline void set(flag_id bit, bool value){
				if (value){
					set_true(bit);
					} else {
					set_false(bit);
				}
			}
			inline explicit operator uint8_t(){ return memory; }
			
			inline bool get(flag_id bit) volatile {
				return memory & (static_cast<uint8_t>(1) << bit);
			}
			inline void set_true (flag_id bit) volatile {
				memory |= 1 << bit;
			}
			inline void set_false (flag_id bit) volatile {
				memory &= ~(1 << bit);
			}
			inline void set(flag_id bit, bool value) volatile {
				if (value){
					set_true(bit);
					} else {
					set_false(bit);
				}
			}
			inline explicit operator uint8_t() volatile { return memory; }
		};
		#endif
		
		
		template<uint8_t bytes>
		class flags {
			static_assert(bytes < 32, "Too many flags. bytes should no be greater than 31.");
			static_assert(bytes != 0, "Zero-size flag object type.");
			public:
			
			using memory_t = uint8_t[bytes];
			using flag_id = range_int<uint8_t,bytes * 8, true, true>;
			
			private:
			
			public:
			memory_t memory;

			/* c-tors */
			inline flags();

			inline flags(memory_t data);
			
			/* accessors */
			inline bool get(flag_id bit);

			inline void set_true(flag_id bit);
			
			inline void set_false(flag_id bit);
			
			inline void set(flag_id bit, bool value){
				if (value){
					set_true(bit);
					} else {
					set_false(bit);
				}
			}
			
			inline explicit operator const memory_t&(){ return memory; }
			
			/* volatile accessors */
			
			inline bool get(flag_id bit) volatile;

			inline void set_true(flag_id bit) volatile;
			
			inline void set_false(flag_id bit) volatile;
			
			inline void set(flag_id bit, bool value) volatile {
				if (value){
					set_true(bit);
					} else {
					set_false(bit);
				}
			}
			
			inline explicit operator const memory_t&() volatile { return memory; }
		};

		
		template<>
		inline fsl::lg::flags<1>::flags()
		{
			memory[0] = 0;
		}
		template<uint8_t bytes>
		inline fsl::lg::flags<bytes>::flags()
		{
			for(uint8_t i = 0; i < bytes; ++i) memory[i] = 0;
		}
		template<>
		inline fsl::lg::flags<1>::flags(memory_t data)
		{
			memory[0] = data[0];

		}
		template<uint8_t bytes>
		inline fsl::lg::flags<bytes>::flags(memory_t data)
		{
			for(uint8_t i = 0; i < bytes; ++i) memory[i] = data[i];
		}
		template<>
		inline bool fsl::lg::flags<1>::get(flag_id bit)
		{
			return memory[0] & (static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline bool fsl::lg::flags<bytes>::get(flag_id bit)
		{
			return memory[bit/8] & (static_cast<uint8_t>(1) << (bit % 8));
		}
		template<>
		inline void fsl::lg::flags<1>::set_true(flag_id bit)
		{
			memory[0] |= (static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline void fsl::lg::flags<bytes>::set_true(flag_id bit)
		{
			memory[bit/8] |= (static_cast<uint8_t>(1) << (bit % 8));
		}
		template<>
		inline void fsl::lg::flags<1>::set_false(flag_id bit)
		{
			memory[0] &= ~(static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline void fsl::lg::flags<bytes>::set_false(flag_id bit)
		{
			memory[bit/8] &= ~(static_cast<uint8_t>(1) << (bit % 8));
		}
		/* volatile */
		template<>
		inline bool fsl::lg::flags<1>::get(flag_id bit) volatile
		{
			return memory[0] & (static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline bool fsl::lg::flags<bytes>::get(flag_id bit) volatile
		{
			return memory[bit/8] & (static_cast<uint8_t>(1) << (bit % 8));
		}
		template<>
		inline void fsl::lg::flags<1>::set_true(flag_id bit) volatile
		{
			memory[0] |= (static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline void fsl::lg::flags<bytes>::set_true(flag_id bit) volatile
		{
			memory[bit/8] |= (static_cast<uint8_t>(1) << (bit % 8));
		}
		template<>
		inline void fsl::lg::flags<1>::set_false(flag_id bit) volatile
		{
			memory[0] &= ~(static_cast<uint8_t>(1) << bit);
		}
		template<uint8_t bytes>
		inline void fsl::lg::flags<bytes>::set_false(flag_id bit) volatile
		{
			memory[bit/8] &= ~(static_cast<uint8_t>(1) << (bit % 8));
		}


		using single_flags = flags<1>;
		// using single_flags = single_flags_old_impl;
	}
}
#endif //__F_FLAGS_H__
