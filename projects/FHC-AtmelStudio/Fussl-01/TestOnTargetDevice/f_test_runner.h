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
	
	public:
	bool shrink_to_fit(){ return resize(vector_size); }
	
	
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
		new (&array[vector_size++]) T(t); // constructor must be called here not the assignment op check how this is done in the official std::vector. // check whether this is doing what I exspect
		return true;
	}
	
	bool push_back(T&& t){
		if (vector_size == underlying_array_size) if (!extend_size()) return false;
		if (!(vector_size < underlying_array_size)) return false;
		DDRB |= 27;
		new (&array[vector_size++]) T(static_cast<T&&>(t)); // constructor must be called here not the assignment op // check whether this is doing what I exspect 
		DDRB &= 0x7C;
		return true;
	}
	
	bool pop_back(){
		if (vector_size <= 0) return false;
		auto last = end() - 1;
		last->T::~T();
		--vector_size;
		return true;
	}
	
	using iterator = T*;
	using const_iterator = T const *;
	
	iterator begin(){ return array; }
	iterator end(){ return array + vector_size; }
	
	const_iterator cbegin() const { return array; }
	const_iterator cend() const { return array + vector_size; }
	
};

class f_test_runner
{
	bool _any_fail{ false };
	bool error_log_fail{ false };
	vector<vector<char>> _errors;
	vector<vector<char>> scope;
	const char* last_error{ nullptr };
	
	inline static void copy_raw_string_into_vector(const char* cstr, vector<char>& vec){
		vector<char>* pVec{ &vec };
		
		for(auto it = cstr; *it != '\0'; ++it){
			DDRB = 36;
			vec.push_back(*it);
			DDRB = 14;
		}
		DDRB = 11;
		vec.shrink_to_fit();
		DDRB = 99;
	}
	
	inline static void copy_vector_onto_end_of_vetor(const vector<char>& source, vector<char>& target){
		for (auto iter = source.cbegin(); iter != source.cend(); ++iter){ target.push_back(*iter); }
	}
	
public:
	f_test_runner(){};
	f_test_runner( const f_test_runner &c ) = delete;
	f_test_runner& operator=( const f_test_runner &c ) = delete;
	
	inline const vector<vector<char>>& errors() const { return _errors; }
	
	inline const bool any_fail() const { return _any_fail; }
	
	inline void check(const char* description, bool claim){
		if (error_log_fail) return;
		if (!claim){
			_any_fail = true;
			last_error = description;
			
			// Copy the error Message:
			vector<char> d;
			for(const auto& s : scope){
				copy_vector_onto_end_of_vetor(s,d);
				d.push_back('/');
			}
			copy_raw_string_into_vector(description, d);
			
			// Move copied error message into errors:
			if (!_errors.push_back(static_cast<vector<char>&&>(d))){
				error_log_fail = true;
			}
		}
	}
	
	void enter_scope(const char* scope_description){
		DDRB = 7;
		if (error_log_fail) return;
		DDRB = 0;
		vector<char> d;
		DDRB = 6;
		copy_raw_string_into_vector(scope_description, d); // error on 3 here
		DDRB = 3;
		if (!scope.push_back(static_cast<vector<char>&&>(d))){
		DDRB = 2;
			error_log_fail = true;
		DDRB = 9;
		}
		DDRB = 0;
	}
	
	inline void leave_scope(){ 
		DDRB = 7;
		if (!scope.pop_back()) error_log_fail = true;
		DDRB = 0;
	}

}; //f_target_device_test

#endif //__F_TARGET_DEVICE_TEST_H__
