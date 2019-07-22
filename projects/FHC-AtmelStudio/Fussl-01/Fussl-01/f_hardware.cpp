/*
 * f_hardware.cpp
 *
 * Created: 01.09.2016 15:20:33
 *  Author: Maximilian Starke
 */ 

/************************************************************************/
/*		FPS see at hardware.h
	                                                                    */
/************************************************************************/

#include "f_hardware.h"

#ifndef F_CPU
	#define F_CPU 16000000UL
#else
	#error "F_CPU was defined before"
#endif


#include <util/delay.h>
#include "f_ledline.h"

void hardware::delay(uint16_t ms){
	for(uint16_t i = 0; i<ms; i+=1){
		_delay_ms(1);
	}
}

bool hardware::isEEPNull(uint16_t address){
	if (	(address >= (1<<12))		&&		(address!=EEPNULL)		){
		led::error(0);
	}
	return (address >= (1<<12));
}

void hardware::copyString(char* destination, const char* source, uint8_t count, bool nullTerminated){
	uint8_t index = 0;
	while(index < count){
		destination[index] = source[index];
		if (source[index] =='\0') return;
		++index;
	}
	if (nullTerminated) {destination[index] = '\0';}
}
