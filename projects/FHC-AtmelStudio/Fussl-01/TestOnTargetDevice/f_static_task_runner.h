/**
@file f_static_task_runner.h
@author Maximilian Starke
@date 2019
@copyright All rights reserved. For any license please contact the author.
*/

#ifndef F_STATIC_TASK_RUNNER_H_
#define F_STATIC_TASK_RUNNER_H_

#include "f_type_traits.h"

/*!
	@brief A non-empty tuple of callbacks of the same type.
	@details Provides calling all callbacks with same arguments in given order.
	Provides concatenating such tuples and appending callbacks, all by using meta programming and variadic templates.
*/
template <class _Callback, _Callback first, _Callback... rest>
struct static_task_runner{
	static constexpr _Callback First = first;
	using FIRST = static_task_runner<_Callback, first>;
	using REST = static_task_runner<_Callback, rest...>;
	using THIS_TYPE = static_task_runner<_Callback, first, rest...>;
	
	template<class... T>
	inline static void run(T... args){
		FIRST::template run<T...>(fsl::ver_1_0::forward<T>(args)...);
		REST::template run<T...>(fsl::ver_1_0::forward<T>(args)...);
	}
	
	template<_Callback c>
	using append_front = static_task_runner<_Callback, c, first, rest...>;
	
	template<_Callback c>
	using append_back = static_task_runner<_Callback, first, rest..., c>;
	
	template<class T>
	using after = typename REST::template after<typename T::template append_back<first>>;
};

template <class _Callback, _Callback first>
struct static_task_runner<_Callback, first>{
	static constexpr _Callback First = first;
	using THIS_TYPE = static_task_runner<_Callback, first>;
	template<class... T>
	inline static void run(T... args){
		first(fsl::ver_1_0::forward<T>(args)...);
	}
	
	template<_Callback c>
	using append_front = static_task_runner<_Callback, c, first>;
	
	template<_Callback c>
	using append_back = static_task_runner<_Callback, first, c>;
	
	template<class T>
	using after = typename T::template append_back<First>;
	
};

/*!
	@brief Represents concatenation of multiple #static_task_runner types
*/
template <class T, class... U>
struct concatenation {
	using result = typename concatenation<U...>::result::template after<T>;
};

template<class T>
struct concatenation<T> {
	using result = T;
};

/*!
	@brief Performs concat. (Wrapper for #concatenation::result)
*/
template<class... T>
using concat = typename concatenation<T...>::result;

/*!
	@brief Allows partial template specification for #static_task_runner with fixed @ref _Callback.
*/
template<class _Callback>
struct static_task_runners {
	template <_Callback first, _Callback... rest>
	using static_task_runner = ::static_task_runner<_Callback, first, rest...>;
};








#endif /* F_STATIC_TASK_RUNNER_H_ */