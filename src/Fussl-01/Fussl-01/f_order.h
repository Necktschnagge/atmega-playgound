/*
 * f_order.h
 *
 * Created: 15.07.2018 11:31:12
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_ORDER_H_
#define F_ORDER_H_

namespace fsl {
	
	namespace lg {
		
		template <typename T>
		inline const T& min(const T& lhs, const T& rhs){ return lhs < rhs ? lhs : rhs; }

		template <typename T>
		inline const T& max(const T& lhs, const T& rhs){ return lhs < rhs ? rhs : lhs; }
		
	}
}



#endif /* F_ORDER_H_ */