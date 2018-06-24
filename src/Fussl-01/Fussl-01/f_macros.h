/*
 * f_macros.h
 *
 * Created: 22.08.2017 20:01:08
 *  Author: Maximilian Starke
 */ 


#ifndef F_MACROS_H_
#define F_MACROS_H_

#include <avr/interrupt.h>

/* does necessary stuff to enter a critical section, i.e. deactivate interrupts and save settings before */
#define macro_interrupt_critical_begin		uint8_t __sreg__ = SREG; cli()
/* restores settings of interrupts, to be used right after critical section */
#define macro_interrupt_critical_end		SREG = __sreg__
/* excludes interrupts in the critical section (code given as parameter) */
#define macro_interrupt_critical(code)		{ macro_interrupt_critical_begin; { code } macro_interrupt_critical_end; }

/* excludes interrupts in the critical section (code given as parameter) */
#define macro_atomic_on_sublevel(code)					macro_interrupt_critical(code)
/* excludes interrupts in the critical section (code given as parameter) and does not put your code in additional code block { } */
#define macro_atomic_no_sublevel(code)		macro_interrupt_critical_begin; code { macro_interrupt_critical_end; }



#endif /* F_MACROS_H_ */