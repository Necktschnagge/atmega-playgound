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

template<typename T>
class vector {
	T* array;
	size_t array_size;
	size_t vector_size;
	
	bool resize(size_t size){
		if (vector_size > size) return false;
		T* array = static_cast<T*>(malloc(size*sizeof(T)));
		if (array == nullptr) return false;
		
		auto begin = reinterpret_cast<uint8_t*>(this->array);
		auto end = reinterpret_cast<uint8_t*>(this->array + array_size);
		if (end < begin) { free(array); return false; }
		auto nbegin = reinterpret_cast<uint8_t*>(array);
		for(uint8_t* it = begin; it < end; ++it) *(nbegin++) = *it;
		
		free(this->array);
		this->array  = array;
		array_size = size;
		return true;
	}
	
	public:
	vector() : array(nullptr), array_size(0), vector_size(0) {}
	// vector(vector&& another) : array(another.array), 
	
	bool push_back(const T& t){
		if (vector_size == array_size) if (!resize(2* (array_size + 2))) return false;
		if (!(vector_size < array_size)) return false;
		array[vector_size++] = t;
		return true;
	}
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
			//if (!errors.push_back(d)){ not allowed, think about copy or move constructor for vector.
				error_log_fail = true;
			}
		}
	}

}; //f_target_device_test

#endif //__F_TARGET_DEVICE_TEST_H__
