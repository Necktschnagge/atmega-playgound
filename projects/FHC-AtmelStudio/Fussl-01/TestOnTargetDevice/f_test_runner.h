/* 
* f_target_device_test.h
*
* Created: 28.07.2019 20:39:38
* Author: F-NET-ADMIN
*/


#ifndef __F_TARGET_DEVICE_TEST_H__
#define __F_TARGET_DEVICE_TEST_H__

#include <avr/io.h>
#include <stddef.h>
#include <stdlib.h>


inline void* operator new (size_t n, void* ptr) {return ptr;}

template<typename T>
class vector {
	T* array; // pointer to array
	size_t underlying_array_size; // size of array, in count(T), not in Bytes
	size_t vector_size; // number of elements stored in 
	
	static constexpr size_t next_size(const size_t& s){ return 2* (s+ 2); }
	
	bool resize(size_t nsize){
		if (vector_size > nsize) return false;
		T* narray = static_cast<T*>(malloc(nsize*sizeof(T)));
		if (narray == nullptr) return false;
		
		auto raw_bytes_begin = reinterpret_cast<uint8_t*>(this->array);
		auto raw_bytes_end = reinterpret_cast<uint8_t*>(this->array + underlying_array_size);
		if (raw_bytes_end < raw_bytes_begin) { free(narray); return false; }
		
		auto niter = reinterpret_cast<uint8_t*>(narray);
		for(uint8_t* it = raw_bytes_begin; it < raw_bytes_end; ++it) *(niter++) = *it;
		
		free(this->array); // can it be used with arrays?
		this->array  = narray;
		underlying_array_size = nsize;
		return true;
	}
	
	bool extend_size(){ return resize(next_size(underlying_array_size)); }
	bool shrink_to_fit(){ return resize(vector_size); }
	
	public:
	vector() : array(nullptr), underlying_array_size(0), vector_size(0) {}
	vector(vector&& another) : array(another.array), underlying_array_size(another.underlying_array_size), vector_size(another.vector_size) {
		another.array = nullptr;
		another.underlying_array_size = 0;
		another.vector_size = 0;
	}
	vector(const vector&) = delete;
	vector& operator=(const vector&) = delete;
	vector& operator=(vector&& another){
		free(array); //can it be used with arrays?
		array = another.array;
		underlying_array_size = another.underlying_array_size;
		vector_size = another.vector_size;
		another.array = nullptr;
		another.underlying_array_size = 0;
		another.vector_size = 0;
		return *this;
	}
	
	bool push_back(const T& t){
		if (vector_size == underlying_array_size) if (!extend_size()) return false;
		if (!(vector_size < underlying_array_size)) return false;
		new (&array[vector_size++]) T(t); // constructor must be called here not the assignment op check how this is done in the official std::vector.
		return true;
	}
	bool push_back(T&& t){
		if (vector_size == underlying_array_size) if (!extend_size()) return false;
		if (!(vector_size < underlying_array_size)) return false;
		new (&array[vector_size++]) T(static_cast<T&&>(t)); // constructor must be called here not the assignment op
		return true;
	}
	
	using iterator = T*;
	using const_iterator = T const *;
	
	iterator begin(){ return array; }
	iterator end(){ return array + vector_size; }
	const_iterator cbegin(){ return array; }
	const_iterator cend(){ return array + vector_size; }

};

class f_test_runner
{
	bool any_fail{ false };
	bool error_log_fail{ false };
	vector<vector<char>> errors;
	const char* last_error{ nullptr };
	
public:
	f_test_runner(){};
	f_test_runner( const f_test_runner &c ) = delete;
	f_test_runner& operator=( const f_test_runner &c ) = delete;

	inline void check(const char* description, bool claim){
		if (error_log_fail) return;
		if (!claim){
			any_fail = true;
			last_error = description;
			vector<char> d;
			for(auto it = description; *it != '\0'; ++it) d.push_back(*it);
			if (!errors.push_back(static_cast<vector<char>&&>(d))){ //not allowed, think about copy or move constructor for vector.
				error_log_fail = true;
			}
		}
	}

}; //f_target_device_test

#endif //__F_TARGET_DEVICE_TEST_H__
