

/*
 * f_hardware.h
 *
 * Created: 01.09.2016 15:10:01
 *  Author: F-NET-ADMIN
 */ 


/************************************************************************/
/* hardware contains some utilities often required
   you can just use them without anything to prepare
                                                                        */
/************************************************************************/


#include <stdint.h>

#ifndef F_HARDWARE_H_
#define F_HARDWARE_H_

#define EEPNULL 0xFFFF			// NULL ptr for the EEPROM

namespace hardware {
	void delay(uint16_t ms);
	
	bool isEEPNull(uint16_t address);
	/* return whether an eeprom address is out of range (NULL), throw an error if a non standard NULL address (!=EEPNULL) was used */

	void copyString(char* destination, const char* source, uint8_t count, bool nullTerminated);
	/* copy a string from source to destination */
	/* it will only copy (count) characters and will add an '\0' if nullTerminated */
	/* the real string length of destination will be count + 1 (if nullTerminated) */
	/* but if source has an '\0' before copying will be stopped immediately */
}

#endif /* F_HARDWARE_H_ */