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
					
	effective macros:
	#ifdef WEAK_ERROR the weak errors thrown from inside of this library are displayed using the error() function.
		read the description of led::error() for additional information.
	Otherwise such Errors are ignored and program goes on without any sensibly kind of busy waiting.
                                                                        */
/************************************************************************/

#ifndef F_LEDLINE_H
#define F_LEDLINE_H

#include <stdint.h>

namespace led {
	
	constexpr char DOT = 0x80;
	
	void init(uint8_t lineLength);
		/* make PORT A ready for IO usage and clear the led line */
		/* set an intern variable to the line length given as argument */
	
	void latch();
		/* send a latch clock signal */
	
	void pushBitIntern(bool bit);
		/* deprecated for user */
		/* set data bit and send a shift register clock signal (and do NOT update the coherent memory _LEDLINE_) */
	
	void pushBit(bool bit);
		/* push a bit to output stream (coherent) */
	
	void pushByte(uint8_t bitcode);
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
		//### update description in cpp and copy paste here
		
	void printSign(char sign);//for user
		/* (visible) print a sign to the end of the led output */
	
	void printDigit(uint8_t digit);//for user
		/* print a digit {0~9} (visible) to the end of the led output */
		/* DO NOT CALL WITH AN INTEGER GREATER THAN 9 (will cause a weak error #ifdef WEAK_ERROR: error code 2) !!!!!*/
	
	uint8_t printInt(int64_t integer, bool checkLineLength = false);//for user
		/* print an integer to the end of the led output (of course with "-" if negative) */
		/* returns the amount of digits (minus is also counted as one digit) which were visibly pushed to the led line. */
		/* if the printed int is longer than the LINELENGTH (set with init()) and (checkLineLength) and #ifdef WEAK_ERROR
			error(3) is called and displays 'E003' after the integer was printed */
	
	// <<<<< add a printInt for uint64_t
				
	inline uint8_t printIntOVChecked (int64_t integer){
		/* see description of printInt() */
		return printInt(integer, true);
	}
	
	void printSignDottable(char sign, bool dot);//for user
		/* push a sign (visible) to the led output with explicit dotting (implicit dot will be overwritten) */
		/* overrides the implicit dot of the sign. Notice if the sign has an real implicit dot, bool dot will invert dot to 0 */
	
	void printSignDottableExplicit(char sign, bool dot);//for user
		/* push a sign (visible) to the led output with real explicit dotting */
		/* creates the code to push into the led line first and overrides the dot like given afterwards. */
		/* a dot is printed <=> bool dot == true */
	
	void printString(const char* string);//for user
		/* print a string to the led output (also supports implicit dotting ) */
	
	void clear();//for user
		/* clear the LED line */
	
	void LFPrintString(const char* string);//for user
		/* print the given string and (at least) as much as needed space signs before to push the previous string away */
	
	void printDotsOnly(uint8_t dotCode); /* print only the dots by the given binary code and leave everything else unchanged */
	
	void error(uint16_t code);
		/* print an error code to the led output, format: ENNN (NNN .. error code) */
		/* errors 0 .. 99 will cause a delay of a little time (busy waiting)*/
		/* errors 100 .. 999 will stop the controller activity in an infinite loop */
}
		
#endif /* F_LEDLINE_H */
/*EOF*/