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
	using REST = static_task_runner<_Callback, rest...>;
	
	template<class... T>
	inline static void run(T... args){
		static_task_runner<_Callback, first>::run(args...);
		static_task_runner<_Callback, rest...>::run(args...);
	}
	
	template<_Callback c>
	using append_front = static_task_runner<_Callback, c, first, rest...>;
	
	template<_Callback c>
	using append_back = static_task_runner<_Callback, first, rest..., c>;
	
	template<class T>
	using concat = typename T::append_front; //<first>; //::concat(static_task_runner<_Callback, rest...>);
	// static_task_runner<_Callback, first, rest..., T::First>;
	
	using rest_reordered = typename static_task_runner<_Callback, rest...>::reverse_ordered; // ???
	
	//using reverse_ordered = typename rest_reordered::append_back<first>; // ???
	//using reverse_ordered = reverse_orderedp<first>;
	
};

template <class _Callback, _Callback first>
struct static_task_runner<_Callback, first>{
	static constexpr _Callback First = first;
	template<class... T>
	inline static void run(T... args){
		first(args...);
	}
	
	template<_Callback c>
	using append_front = static_task_runner<_Callback, c, first>;
	
	template<_Callback c>
	using append_back = static_task_runner<_Callback, first, c>;
	
	using reverse_ordered = static_task_runner<_Callback, first>;
	
};

template<class _Callback>
struct callback_traits {
	template <_Callback first, _Callback... rest>
	using static_task_runner = ::static_task_runner<_Callback, first, rest...>;
};




#endif /* F_STATIC_TASK_RUNNER_H_ */