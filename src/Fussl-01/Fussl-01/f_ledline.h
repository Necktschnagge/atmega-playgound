/*
 * f_ledline.h
 *
 * Created: 23.08.2016 17:32:54
 *  Author: F-NET-ADMIN
 */ 


/************************************************************************/
/* first of all you need to run the init() function to make sure
   outputs are enabled and safe.
   init() sets the output pins and pushes an empy string through the led line.
   
   Than you can use all functions as you want
   
   Output bits are hardcoded:
		Port A:		bit 0:	data bit
					bit 1:	clock bit
					bit 2:	latch-clock bit
                                                                        */
/************************************************************************/

#ifndef F_LEDLINE_H
#define F_LEDLINE_H

#include <stdint.h>

namespace led {
	
	constexpr char DOT = 0x80;
	
	void init(const uint8_t lineLength);
		/* make PORT A ready for IO usage and clear the led line */
		/* set an intern variable to the linelength given ar argument */
	
	void latch();
		/* send a latch clock signal */
	
	void pushBitIntern(const bool bit);
		/* deprecated for user */
		/* set data bit and send a shift register clock signal (and do NOT update the coherent memory) */
	
	void pushBit(const bool bit);
		/* push a bit to output stream (coherent) */
	
	void pushByte(const uint8_t bitcode);
		/* push 8 bit to the output stream. MSB is pushed first, (coherent) */
	
	void push64(uint64_t bitcode);
		/* push 64 bit to the output stream. MSB is pushed first (coherent) */
		
	void pushMemory();
		/* push the memory of the output stream VISIBLE to the output stream */
		
	void pushByteVisible(uint8_t bitcode);
		/* push a byte to the led output stream and update the latch */
		/* see ledPushByte() (MSB first) */
	
	bool isDotted(char sign);
		/* returns whether the sign contains an implicit dot or not */
	
	void setDot(char* sign, bool dot);
		/* override the implicit dot of the sign with the explicit given dot */
	
	bool changeDot(char* sign);
		/* change the dot of the sign and return whether the sign has a dot @after */
	
	uint8_t signCode(char sign);
		/* channel coding: look up the sign code in the EEPROM and return the bit code for LED output (supports implicit dotting) */
	
	void printSign(char sign);//for user
		/* (visible) print a sign to the end of the led output */
	
	void printDigit(uint8_t digit);//for user
		/* print a digit {0~9} (visible) to the end of the led output */
		/* DO NOT CALL WITH AN INTEGER GREATER THAN 9 (will cause a weak error) !!!!!*/
	
	void printInt(int16_t integer);//for user
		/* print an integer to the end of the led output (of course with "-" if negative) */
		
	void printSignDottable(char sign, bool dot);//for user
		/* push a sign (visible) to the led output with explicit dotting (implicit dot will be overwritten) */
	
	void printString(const char* string);//for user
		/* print a string to the led output (also supports implicit dotting ) */
	
	void clear(void);//for user
		/* clear the LED line */
	
	void LFPrintString(const char* string);//for user
		/* print the given string and as much as needed space signs before to push the previous string away */
	
	void printDotsOnly(uint8_t dotCode); /* print only the dots by the given binary code and leave everything else unchanged */
	
	void error(const uint16_t code);
		/* print an error code to the led output, format: ENNN (NNN .. error code) */
		/* errors 0 .. 99 will cause a delay of a little time (busy waiting)*/
		/* errors 100 .. 999 will stop the controller activity in an infinite loop */
}
		
#endif /* F_LEDLINE_H */
/*EOF*/