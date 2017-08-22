/*
 * f_macros.h
 *
 * Created: 22.08.2017 20:01:08
 *  Author: Maximilian Starke
 */ 


#ifndef F_MACROS_H_
#define F_MACROS_H_

#define macro_interrupt_critical_begin		uint8_t __sreg__ = SREG; cli() // rename this shit!!! to something like macro_critical_begin
#define macro_interrupt_critical_end		SREG = __sreg__
#define macro_interrupt_critical(code)		critical_begin; { code } critical_end




#endif /* F_MACROS_H_ */