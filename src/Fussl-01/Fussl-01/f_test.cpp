/*
* f_test.cpp
*
* Created: 18.09.2016 12:50:16
* Author: Maximilian Starke
*
*
*/ 
#include "f_ledline.h"
#include "f_hardware.h"
#include "f_test.h"

using namespace hardware;

#define PAUSE delay(2000);
#define WAIT delay(1000);
#define SLEEP delay(6000);

inline void throwerror(){
	using namespace led;
	LFPrintString("error");
}

namespace test {
	
	void led_t::run(){
		using namespace led;
		
		init(8);
		delay(1000);
		
		// pushBit nad latch
		for (uint8_t i = 0; i < 70; ++i){
			pushBit(i%2);
			if (!(i%3)){
				latch();
			};
			delay(1000);
		}
		
		clear();
		PAUSE
		
		//pushByte and latch
		for (uint8_t i= 0; i<8; ++i)
		{
			pushByte(1<<i);
			latch();
			delay(1000);
		}
		clear();
		PAUSE
		
		// pushByteVisible
		for (uint8_t i = 0; i<16; ++i){
			pushByteVisible(i);
			delay(1000);
		}
		for (uint8_t i = 0; i<16; ++i){
			pushByteVisible( (i<<4) + 0x0F);
			delay(1000);
		}
		clear();
		PAUSE
		
		//printsign
		printSign('A');
		printSign('B');
		printSign('C');
		printSign('D');
		PAUSE;
		PAUSE
		clear();
		SLEEP
		
		printSign('(');
		printSign('e');
		printSign('x');
		printSign('p');
		printSign(')');
		printSign('-');
		printSign('3');
		PAUSE PAUSE PAUSE PAUSE
		
		clear();
		PAUSE
		
		// printstring
		char mystring[] = "true";
		char s2 [] = "str-prnt";
		printString(mystring);
		PAUSE; PAUSE PAUSE
		printString(s2);
		PAUSE PAUSE PAUSE PAUSE
				
		clear();
		
		// lfprintstring
		char s3 [] = "LF PRN S";
		char s4 [] = "lf 4";
		char s5 [] = "12345";
		LFPrintString(s3);
		PAUSE PAUSE PAUSE
		LFPrintString(s4);
		PAUSE PAUSE PAUSE
		LFPrintString(s5);
		SLEEP
		LFPrintString("test-1");
		SLEEP
		LFPrintString("last-lf");
		SLEEP
		
		
		// isDotted
		LFPrintString("isdotted");
		PAUSE
		printSign('0');
		if (isDotted('a'+DOT)) { } else { throwerror();}
		SLEEP;
		printSign('1');
		if (isDotted('5')) { throwerror(); }
		SLEEP
		SLEEP
		
		clear();
		SLEEP
		
		// setdot and printsigndottable
		char a = 'a';
		setDot(&a,true);
		printSign(a);
		setDot(&a,false);
		printSign(a);
		printSignDottable(a,true);
		
		printSignDottable(a,false);
		
		// print digigt and integer
		printDigit(0);
		printDigit(3);
		printDigit((7));
		SLEEP
		
		clear();
		printInt(2106);
		SLEEP
		SLEEP
		LFPrintString("THE END");
	}
	
	
}