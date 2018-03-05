/*
 * f_ledline.h
 *
 * Created: 23.08.2016 17:32:54
 *  Author: Maximilian Starke
 *
 *	FILE PROGRESS STATE:
 *  wait until char set table is ready than look at led::signCode
 *	search for <<<<< comments
 */ 


/************************************************************************/
/*
   first of all you need to run the init() function to make sure
   outputs are enabled and safe.
   init() sets the output pins and pushes an empy string through the led line.
   
   Than you can use all functions as you want
   
   Output bits are hardcoded:
		Port A:		bit 0:	data bit
					bit 1:	clock bit
					bit 2:	latch-clock bit
					
	effective macros:
	#ifdef WEAK_ERROR the weak errors thrown from inside of this library (and of course from outside too)
		are displayed using the error() function.
		read the description of led::error() for additional information.
	Otherwise such Errors are ignored and program goes on without any sensibly kind of busy waiting.
 */
/************************************************************************/

/*	reservation of error codes:
		error code		:		calling function		:		error content					:		conditions
		----------------------------------------------------------------------------------------------------------
			2			:		led::printDigit()		:	no digit {0~9} given				: #ifdef WEAK_ERROR
			3			:		led::printInt()			:	integer is longer than lineLength	: #ifdef WEAK_ERROR

*/


#ifndef F_LEDLINE_H
#define F_LEDLINE_H

/* We need to distinguish for which target we are compiling. So just do it by switching the compiler. */
#ifdef _MSC_VER
/* working on Visual Studio with MS C++ compiler */
	#ifdef __GNUC__
		#error It seems like you use some unknown mutation of several compilers. Checkout f_ledline!
	#else
		#define NO_AVR
		#pragma message("f_ledline: You are running Visual Studio with MS Compiler. Only a few part of f_ledline will be compiled for testing purpose.")
	#endif

#else // No MS compiler (Visual Studio)
/* working on Atmel Studio with GNUC compiler */
	#ifdef __GNUC__
		//#undef NO_AVR // just don't define it.
		#pragma message("f_ledline: You are running AVR Studio with GNUC. The whole f_ledline will be compiled.")
	#else
		#error Your compiler seems to be someone f_ledline does not know!
	#endif
#endif

#include <stdint.h>

namespace led {
	
	constexpr char DOT = 0x80 - 0x100;
	
#ifndef NO_AVR

		/* make PORT A ready for IO usage and clear the led line */
		/* set an intern variable to the line length given as argument */
	void init(uint8_t lineLength);

		/* send a latch clock signal */	
	void latch();
	
		/* should not be used by extern programmer*/
		/* send a clock signal */
	void clock();
	
		/* deprecated for user */
		/* set data bit and send a shift register clock signal (and do NOT update the coherent memory _LEDLINE_) */
	void pushBitIntern(bool bit);
	
		/* push a bit to output stream (coherent to internal memory _LEDLINE_) */
	void pushBit(bool bit);
	
		/* push 8 bit to the output stream. MSB is pushed first, (coherent) */
	void pushByte(uint8_t bitcode);
	
		/* push 64 bit to the output stream. MSB is pushed first (coherent) */
	void push64(uint64_t bitcode);
		
		/* push the memory of the output stream VISIBLE to the output stream */
	void pushMemory();
		
		/* push a byte to the led output stream and update the latch */
		/* see ledPushByte() (MSB first) */
	void pushByteVisible(uint8_t bitcode);
	
#endif // !NO_AVR

		/* returns whether the sign contains an implicit dot or not */
	bool isDotted(char sign);
	
		/* override the implicit dot of the sign with the explicit given dot */
	void setDot(char* sign, bool dot);
	
		/* change the dot of the sign and return whether the sign has a dot @after */
	bool changeDot(char* sign);
	
		/* in general: returns the byte code to push into the led line to display given character */
		//<<<<< update description in cpp and copy paste here: the code table must be ready, than please do this
	uint8_t signCode(char sign);

#ifndef NO_AVR

		/* (visible) print a sign to the end of the led output */
	void printSign(char sign);//for user
	
		/* print a digit {0~9} (visible) to the end of the led output */
		/* DO NOT CALL WITH AN INTEGER GREATER THAN 9 (will cause a weak error #ifdef WEAK_ERROR: error code 2) !!!!!*/
	void printDigit(uint8_t digit);//for user
	
		/* print an integer to the end of the led output (of course with "-" if negative) */
		/* returns the amount of digits (minus is also counted as one digit) which were visibly pushed to the led line. */
		/* if the printed int is longer than the LINELENGTH (set with init()) and (checkLineLength) and #ifdef WEAK_ERROR
			error(3) is called and displays 'E003' after the integer was printed */	
	// <<<<< add a printInt for uint64_t
	uint8_t printInt(int64_t integer, bool checkLineLength = false);//for user
				
		/* see description of printInt() */
	inline uint8_t printIntOVChecked (int64_t integer){
		return printInt(integer, true);
	}
	
		/* push a sign (visible) to the led output with explicit dotting (implicit dot will be overwritten) */
		/* overrides the implicit dot of the sign. Notice if the sign has an real implicit dot, bool dot will invert dot to 0 */
	void printSignDottable(char sign, bool dot);//for user
	
		/* push a sign (visible) to the led output with real explicit dotting */
		/* creates the code to push into the led line first and overrides the dot like given afterwards. */
		/* a dot is printed <=> bool dot == true */
	void printSignDottableExplicit(char sign, bool dot);//for user
	
		/* print a string to the led output (also supports implicit dotting ) */
	void printString(const char* string);//for user

		/* print a string to the led output with given length (also supports implicit dotting ) */	
	void printString(const char* string, uint8_t length); // for user
	
		/* clear the LED line */
	void clear();//for user
	
		/* print the given string and (at least) as much as needed space signs before to push the previous string away */
	void LFPrintString(const char* string);//for user

		/* print the given string and (at least) as much as needed space signs before to push the previous string away */
	void LFPrintString(const char* string, uint8_t length);//for user
	
		/* print only the dots by the given binary code and leave everything else unchanged */
	void printDotsOnly(uint8_t dotCode);
	
		/* print an error code to the led output, format: ENNN (NNN .. error code) */
		/* errors 0 .. 99 will cause a delay of a little time (busy waiting)*/
		/* errors 100 .. 999 will stop the controller activity in an infinite loop */
	void error(uint16_t code);
	
#endif // !NO_AVR
}
		
#endif /* F_LEDLINE_H */
/*EOF*/
