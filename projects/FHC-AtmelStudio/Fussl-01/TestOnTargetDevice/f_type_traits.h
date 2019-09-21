/*
 * f_type_traits.h
 *
 * Created: 14.09.2019 11:37:07
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_TYPE_TRAITS_H_
#define F_TYPE_TRAITS_H_

namespace fsl {
	namespace ver_1_0 {
		
		template<class T, T v>
		struct integral_constant {
			static constexpr T value = v;
			typedef T value_type;
			typedef integral_constant type;
			constexpr operator value_type() const noexcept { return value; }
			constexpr value_type operator()() const noexcept { return value; }
		};
		
		using true_type = fsl::ver_1_0::integral_constant<bool, true>;
		using false_type = fsl::ver_1_0::integral_constant<bool, false>;
		
		template< class T > struct remove_reference      {typedef T type;};
		template< class T > struct remove_reference<T&>  {typedef T type;};
		template< class T > struct remove_reference<T&&> {typedef T type;};
		
		template<class T> struct is_lvalue_reference     : fsl::ver_1_0::false_type {};
		template<class T> struct is_lvalue_reference<T&> : fsl::ver_1_0::true_type {};
		
		template< class T >
		inline constexpr T&& forward( typename fsl::ver_1_0::remove_reference<T>::type& t ) noexcept {
			return static_cast<T&&>(t);
		}
		template< class T >
		inline constexpr T&& forward( typename fsl::ver_1_0::remove_reference<T>::type&& t ) noexcept {
			static_assert(!fsl::ver_1_0::is_lvalue_reference<T>::value, "Cannot forward an rvalue as an lvalue."); // why???
			return static_cast<T&&>(t);
		}
		
		template<class T, class U>
		struct is_same : fsl::ver_1_0::false_type {};
		
		template<class T>
		struct is_same<T, T> : fsl::ver_1_0::true_type {};
	}
}



#endif /* F_TYPE_TRAITS_H_ */