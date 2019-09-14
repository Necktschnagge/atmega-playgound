/*
 * f_static_task_runner.h
 *
 * Created: 07.09.2019 11:23:17
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_STATIC_TASK_RUNNER_H_
#define F_STATIC_TASK_RUNNER_H_

// idea:
template <class _Callback, _Callback first, _Callback... rest>
struct static_task_runner;

template <class _Callback, _Callback first, _Callback... rest>
struct static_task_runner{
	static constexpr _Callback First = first;
	using FIRST = static_task_runner<_Callback, first>;
	using REST = static_task_runner<_Callback, rest...>;
	using THIS_TYPE = static_task_runner<_Callback, first, rest...>;
	
	template<class... T>
	inline static void run(T... args){
		FIRST::run(args...);
		REST::run(args...);
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
		first(args...);
	}
	
	template<_Callback c>
	using append_front = static_task_runner<_Callback, c, first>;
	
	template<_Callback c>
	using append_back = static_task_runner<_Callback, first, c>;
	
	template<class T>
	using after = typename T::template append_back<First>; // error <;
	
	//using reverse_ordered = static_task_runner<_Callback, first>;
	
};

template <class T, class... U>
struct concatenation {
	using type = typename concatenation<U...>::type::template after<T>;
};

template<class T>
struct concatenation<T> {
	using type = T;
};

template<class... T>
using concat = typename concatenation<T...>::type;

template<class _Callback>
struct callback_traits {
	template <_Callback first, _Callback... rest>
	using static_task_runner = ::static_task_runner<_Callback, first, rest...>;
};




#endif /* F_STATIC_TASK_RUNNER_H_ */