/*
 * f_buffer.h
 *
 * Created: 13.07.2018 03:56:25
 * Author: Maximilian Starke
 */ 


#ifndef F_BUFFER_H_
#define F_BUFFER_H_

#include <stdint.h>

#include "f_maybe.h"
#include "f_callbacks.h"

namespace fsl {
	namespace con {
		
		template <typename T>
		class buffer {
		public:
			virtual bool empty() = 0;
			virtual bool full() = 0;
			
			virtual T read() = 0;
			virtual void write(const T& element) = 0;
			
		};
		
		template <typename T>
		class rt_buffer : public virtual buffer<T> {
			fsl::str::callable* fill_buffer;
			
			public:
			inline rt_buffer(fsl::str::callable* fill_buffer) : buffer<T>(), fill_buffer(fill_buffer) {}
			inline void prepare_read(){ while(buffer<T>::empty()) (*fill_buffer)(); }
			
		};
		
		template <typename T>
		class wt_buffer : public virtual buffer<T> {
			fsl::str::callable* drain_buffer;
			
			public:
			inline wt_buffer(fsl::str::callable* drain_buffer) : buffer<T>(), drain_buffer(drain_buffer) {}
			inline void prepare_write(){ while(buffer<T>::empty()) (*drain_buffer)(); }
			
		};
		
		template <class T, uint8_t size>
		class array_buffer : public virtual buffer<T> {
			protected:
			T array[size];
			
			uint8_t read_pos; // read position;
			uint8_t filled; // number of filled cells;
			// << with this encoding we never need an empty cell for distinguishing read and write position and whether the buffer is full or empty
			
			// read_pos++ with respect to mod size
			inline uint8_t read_pos_pp(){
				uint8_t copy{ read_pos };
				read_pos = (read_pos + 1) % size;
				return copy;
			}
			
			public:
			
			inline array_buffer(): read_pos(0), filled(0) {}
				
			inline virtual bool empty() override { return !filled; }
			inline virtual bool full() override { return filled == size; }
			
			// unsave read
			inline virtual T read() override { --filled; return array[read_pos_pp()]; }
			// unsave write
			inline virtual void write(const T& element) override { array[(static_cast<uint16_t>(read_pos) + (filled++)) % size] = element; }
		};
		
		/* when you try to read and the buffer is empty a callback is called to fill the buffer */
		template <class T, uint8_t size>
		class rt_array_buffer : public virtual rt_buffer<T>, public virtual array_buffer<T,size> {
			public:
			inline rt_array_buffer(fsl::str::callable* fill_buffer) : array_buffer<T,size>(), rt_buffer<T>(fill_buffer) {}
			inline virtual T read() override { rt_buffer<T>::prepare_read(); return array_buffer<T,size>::read(); }
		};
		
		template <class T, uint8_t size>
		class wt_array_buffer : public virtual wt_buffer<T>, public virtual array_buffer<T,size> {
			public:
			inline wt_array_buffer(fsl::str::callable* drain_buffer) : wt_buffer<T>(drain_buffer), array_buffer<T,size>() {}
			inline virtual void write(const T& element) override { wt_buffer<T>::prepare_write(); return array_buffer<T,size>::write(element); }
		};
		
		template <class T, uint8_t size>
		class wrt_array_bufferr : public wt_array_buffer<T,size>, public rt_array_buffer<T,size> {
			using parent = array_buffer<T,size>;
			using rparent = rt_array_buffer<T,size>;
			using wparent = wt_array_buffer<T,size>;
			
			public:
			inline wrt_array_bufferr(fsl::str::callable* drain_buffer, fsl::str::callable* fill_buffer) : parent(), wparent(drain_buffer), rparent(fill_buffer) {}
				
			//check ambiguity of calls to read and write!!!!#####
		};
		
		template <typename T>
		inline wt_buffer<T>& operator <<(wt_buffer<T>& buffer, const char* string){
			for (; *string != '\0'; ++string) buffer.write(*string);
			return buffer;
		}
	}
}



#endif /* F_BUFFER_H_ */