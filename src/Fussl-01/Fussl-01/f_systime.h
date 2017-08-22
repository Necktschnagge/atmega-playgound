/*
 * f_systime.h
 *
 * Created: 22.08.2017 18:41:29
 *  Author: Maximilian Starke
 */ 


#ifndef F_SYSTIME_H_
#define F_SYSTIME_H_

/************************************************************************/
/* 
history:
	There was a previous half-ready implementation before.
	But I decided to renew this one, because it used a kind of 'refinement'
	which was an additional frequency = real freq. - ideal freq. 
	after all implementing, I found this kinda weird.
	So this is a new implementation with only one frequency value for oscillator


concept:

there are 3 main entities to deal with:
	osc_freq:
		Frequency of the oscillator.
		Unit: Hz
	
	precision:
		Frequency on which the system now time is updated
		Unit: Hz
	
	Compare Match Value (CMV):
		The Value on which the timer must cause an interrupt
		definition equation:
			CMV := round_down( osc_freq / precision + residual_ticks )
		Unit: uint16_t (via TCNT...) 
		
	residual_ticks:
		Ideally CMV would be (osc_freq / precision), but CMV has to be integral.
		So we round_down CMV to be integral and need to carry broken part.
		definition equation:
			residual_ticks@after := trunc(osc_freq / precision + residual_ticks@before)
		it is alway a value in interval [0,1), exactly [0,1] because of technical floating point format
		Unit: 1 (no Unit)

Constraints:
	osc_freq			is given by hardware. programmer has no influence
	
	precision			may be a wish of operating system designer
					(a)	must be chosen such that we can add 1/precision to an ExtendedMetricTime
	
	CMV				(b)	must be an unsigned 16 bit integer [1,...,2^16 -1]
					(c)	it should be not too small (1,2,3..??) because ISR needs some time to be executed.
	residual_ticks		(no restrictions)
	
Evaluation of constraints:
	(d) => (a) where
		(d)	precision in {2^0, 2^1, 2^2, ..., 2^16}
			because:	EMT enables refinement up to 1/2^16 second
						the potencies of 2 are not all possible values but enough, I think,
						and easy to work with when increasing system now time
						1s (=2^0) is no necessary l_border, but nobody wan't schedulers with
						operating in 1s tact.
						
			so we use a var named precision_log of type uint8_t
	(def)		we define precision := 2^precision_log
	(e) => (d)   and   (d) => (a) where
		(e)	precision_log in {0,1,.., 16}
			
	(f) and (g)  =>  (b) and (c) where
		(get f via...)				1*x <= CMV
			<==			x <= round_down( osc_freq / precision + residual_ticks )
			<==			x <= round_down( osc_freq / precision ) // residual is 0..1
			<==			x <= osc_freq / precision
			<==			x * precision <= osc_freq
		(f)		// via x which should be any integer value >=1 (but not too large) we provide
				// the constraint (c) depending on a parameter.
		
		(get g via...)				CMV  <=  2^16 - 1
			<==			round_down( osc_freq / precision + residual_ticks ) <= 2^16 - 1	
			<==			round_down( osc_freq / precision + 1 ) <= 2^16 - 1
			<==			round_down( osc_freq / precision ) + 1 <= 2^16 - 1
			<==			round_down( osc_freq / precision ) <= 2^16 - 2
			<==			osc_freq / precision <= 2^16 - 2
			<==			osc_freq <= (2^16 - 2) * precision
		(g)
		
	SUMMARY:::
		with (e) ^ (f) ^ (g) we get a, b and c.
		
		select any precision_log such that (e) is fulfilled // wish of OS programmer / user
		if not g:	precision(_log) must be chosen greater
		if not f:	precision(_log) must be chosen smaller

	this all has to be checked when creating a SystemTime object!
	algorithm:
		get osc_freq (as copy);
		get wish_precision_log;
		
		if not e:
			log illegal precision_log wish
			set e := max := 16 // max precision
		2^8: counter
		while (!e or !f or !g):
			if not e or counter > 16:
				"return" error, no possible systime object. 
			if not f and not g:
				"return" error, no possible systime object.
			if !f:
				--precision_log // this might change value of !g
			if !g: (else to !f)
				++precision_log
			++counter;
		whileEND
		
		construction complete
		"return" actual precision_log
						
                                                             */
/************************************************************************/


namespace scheduler {
	
	class SystemTime {
		
	};
}



#endif /* F_SYSTIME_H_ */