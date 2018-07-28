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
		
		template <class T, uint8_t size>
		class buffer {
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
			
			inline buffer(): read_pos(0), filled(0) {}
				
			inline bool empty(){ return !filled; }
			inline bool full(){ return filled == size; }
			
			inline virtual T read_copy(){ if (empty()){ return array[read_pos]; } else { --filled; return array[read_pos_pp()]; } }
			inline virtual const T& read_reference(){ if (empty()) { return array[read_pos]; } else { --filled; return array[read_pos_pp()]; } }
			inline virtual fsl::str::maybe<T> read_maybe(){ if (empty()) return fsl::str::maybe<T>(); else return fsl::str::maybe<T>(array[read_pos_pp()]); }
			inline virtual bool write(const T& element){ if (full()) return false; else { array[(static_cast<uint16_t>(read_pos) + (filled++)) % size] = element; return true; } }
			inline virtual bool write(T&& element){ if (full()) return false; else { array[(static_cast<uint16_t>(read_pos) + (filled++)) % size] = element; return true; } }
		};
		
		/* when you try to read and th buffer is empty a callback is called to fill the buffer */
		template <class T, uint8_t size>
		class read_through_buffer : public virtual buffer<T,size> { // rtbuffer
			fsl::str::callable* fill_buffer;
			using parent = buffer<T,size>;
			public:
			inline read_through_buffer(fsl::str::callable* fill_buffer) : parent(), fill_buffer(fill_buffer) {}
			
			inline virtual T read_copy() override { while(parent::empty()) (*fill_buffer)(); --parent::filled; return parent::array[parent::read_pos_pp()]; }// <<< make this functions virtual??? -> but than we have ugly vTables on our microcontroller.
			inline virtual const T& read_reference() override { while(parent::empty()) (*fill_buffer)(); --parent::filled; return parent::array[parent::read_pos_pp()]; }
			inline virtual fsl::str::maybe<T> read_maybe() override { if (parent::empty()) (*fill_buffer)(); if (parent::empty()) return fsl::str::maybe<T>(); else return fsl::str::maybe<T>(parent::array[parent::read_pos_pp()]); }
			
		};
		
		template <class T, uint8_t size>
		class write_through_buffer : public virtual buffer<T,size> {//<< wtbuffer
			fsl::str::callable* drain_buffer;
			using parent = buffer<T,size>;
			
			public:
			inline write_through_buffer(fsl::str::callable* drain_buffer) : parent(), drain_buffer(drain_buffer) {}
			
			inline virtual bool write(const T& element) override { while(parent::full()) (*drain_buffer)(); parent::array[(static_cast<uint16_t>(parent::read_pos) + (parent::filled++)) % size] = element; return true; }
			inline virtual bool write(T&& element) override { while(parent::full()) (*drain_buffer)(); parent::array[(static_cast<uint16_t>(parent::read_pos) + (parent::filled++)) % size] = element; return true; }
			
		};
		
		//<<<< make diamond complete.!!!#####
		template <class T, uint8_t size>
		class wrtbufferr : public write_through_buffer<T,size>, public read_through_buffer<T,size> {
			using parent = buffer<T,size>;
			using rparent = read_through_buffer<T,size>;
			using wparent = write_through_buffer<T,size>;
			
			public:
			inline wrtbufferr(fsl::str::callable* drain_buffer, fsl::str::callable* fill_buffer) : parent(), wparent(drain_buffer), rparent(fill_buffer) {}
				
			//check ambiguity of calls to read and write!!!!#####
		};
	}
}



#endif /* F_BUFFER_H_ */