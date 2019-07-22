/*
 * f_ledline.cpp
 *
 * Created: 23.08.2016 17:32:54
 *  Author: Maximilian Starke
 */

/************************************************************************/
/* FILE PROGRESS STATE
	status:
	there aren't any known bugs or omissions
	
to do:
	only <<< stuff remaining,
	code table should be created first!!!!
	
effective macros:
	#ifdef WEAK_ERROR:
		printDigit(), printInt()
	                                                                     */
/************************************************************************/

#include "f_ledline.h" 
#include <stdlib.h> //needed for abs()
#ifndef NO_AVR
#include "f_hardware.h" // needed for hardware::delay() (busy waiting)
#include <avr/interrupt.h> // needed for cli()
#include <avr/pgmspace.h> // needed for pgm_read_byte, etc.
#endif // !NO_AVR


constexpr uint8_t DOTPOSITION = 7; // dot bit
constexpr uint8_t DOTSIGN = 1<<DOTPOSITION; // use pushByte to push an only-dot

static uint8_t LINELENGTH; // is set by the init function
static uint64_t _LEDLINE_; // contains the current state which is displayed on the led line
							// and which was pushed to it somewhere in past.
							// functions that modify output have to update this value.

/* code table for led::signCode() */
const static uint8_t codeTable[] 
#ifndef NO_AVR
PROGMEM
#endif
										//		0		1	2		3	4		5	6		7	8		9	10		11	12		13	14		15	16		17	18		19	20		21	22		23	24		25	26		27	28		29	30		31
										//		!		"	#		$	%		&	'		(	)		*	+		,	-		.	/		0	1		2	3		4	5		6	7		8	9		:	;		<	=		>	?		@

										= {   0xC8, 0x0A, 0x7F, 0x6E, 0x99, 0x3F, 0x00, 0x16, 0x4C, 0x2B, 0x49, 0x40, 0x01, 0x80, 0x19, 0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x81, 0xC0, 0x39, 0x21, 0x63, 0x9D, 0x5D,
	
									//	32		33	34		35	36		37	38		39	40		41	42		43	44		45	46		47	48		49	50		51	52		53	54		55	56		57
									//	A		B	C		D	E		F	G		H	I		J	K		L	M		N	O		P	Q		R	S		T	U		V	W		X	Y		Z
										0x5F, 0x73, 0x31, 0x79, 0x37, 0x17, 0x76, 0x53, 0x12, 0x78, 0x3B, 0x32, 0x5E, 0x51, 0x71, 0x1F, 0x4F, 0x57, 0x66, 0x4C, 0x70, 0x2A, 0x7A, 0x5B, 0x6B, 0x3C,
		
									//  58		59	60		61	62		63	64		65	66		67	68
									//	[		\	]		^	_		`	{		|	}		~	[127]
										0x36, 0x43, 0x6C, 0x0E, 0x20, 0x08, 0x27, 0x5A, 0x2D, 0x75, 0x0F};

#ifndef NO_AVR
void led::init(const uint8_t lineLength) {
	DDRA = 0b00000111; // LATCH BIT ::: CLOCK BIT ::: DATA BIT
	PORTA = 0b11111000;
	_LEDLINE_ = 0; // comment: is not necessary
	led::clear();
	hardware::delay(20);
	led::clear();
	hardware::delay(20);
	for (uint8_t i = 0; i < lineLength; ++i) led::pushByte(1<<7);
	led::latch();
	hardware::delay(300);
	led::clear();
	LINELENGTH = lineLength;
}

void led::latch() {
	PORTA = PORTA ^ 0b00000100;
	PORTA = PORTA ^ 0b00000100;
}

/*inline*/ void led::clock() {
	PORTA = PORTA ^ 0b00000010;
	PORTA = PORTA ^ 0b00000010;
}

void led::pushBitIntern(const bool bit) {
	PORTA = (PORTA & 0b11111110) | bit;
	led::clock();
}

void led::pushBit(const bool bit){
	_LEDLINE_ = (_LEDLINE_<<1) + bit;
	led::pushBitIntern(bit);
}

void led::pushByte(const uint8_t bitcode){
	_LEDLINE_ = (_LEDLINE_<<8) + bitcode;
	for(int8_t shift = 7; shift >= 0; --shift){
		led::pushBitIntern(bitcode & (1<<shift));
	}
}

void led::push64(const uint64_t bitcode){
	_LEDLINE_ = bitcode;
	for(int8_t shift = 63; shift >= 0; --shift){
		led::pushBitIntern(bitcode & (1LL<<shift));
	}
}

void led::pushMemory(){
	led::push64(_LEDLINE_);
	led::latch();
}

void led::pushByteVisible(const uint8_t bitcode){
	led::pushByte(bitcode);
	led::latch();
}
#endif // !NO_AVR

bool led::isDotted(const char sign){
	return (sign & 0b10000000);
}

void led::setDot(char* const sign, const bool dot){
	*sign = (*sign & 0b01111111) | (dot<<7);
}

bool led::changeDot(char* const sign){
	*sign ^= 1<<7;
	return led::isDotted(*sign); 
}

uint8_t led::signCode(const char sign){
	/* channel coding: look up the sign code in the PROGMEM
		and return the bit code for LED output (supports implicit dotting) */
	/* implicit dotting:
	*	char sign with MSB == 0 will normally be a character without a dot.
		if MSB ==1 an additional dot is printed
		Only in case you print e.g. a '?' or a ';' there will be a dot even if with MSB == 0.
		Actually signs with 'real implicit' dot like '?' will have no dot if MSB == 1
		
		equivalent description
		Every sign of the character set has its own 'real implicit' dot status {with dot | without dot}.
		MSB == 0 is the standard case and the sign will be printed as given in the character set.
		By using MSB == 1 instead of MSB == 0 the dot state will be inverted.
		
		nomenclature:
		 a char sign is basically a code with an >>implicit dot<< (the MSB)
		 the 'real implicit dot' is stored in the look-up table
		 and means the dot of an question mark or an semicolon for example
		 an explicit dot is a dot, you type as argument (extra to the sign) when calling a function
		 and it will override or change the implicit as well as the real implicit dot.
		 
		 
		 current sign coding:
		 
			10 won't be a 'new line'. Instead a space code is returned.
			
			any other sign < 33 will be returned as a unique standard coding,
			where all bits are set HIGH except the dot bit in case [isDotted(sign)] <=> [sign has an implicit dot] .
			
			code table:
			**	: [0b11111111]
			10	: [space]			:	no support of implicit dotting
			**	: [0b11111111]
			33	: !
			34	: "
			35	: #
			36	: $
			37	: %
			38	: &
			39	: ' (on the left) //<<<<< hier ist bisher die falsche kodierung verwendet worden, da es für ein space gehalten wurde. Mal ändern!! mal das programm zum printen von 7segment in die console suchen und in repo hinzufügen. und da nochmal testen<<<<
			40	: (
			41	: )
			42	: *
			43	: +
			44	: ,
			45	: -
			46	: .
			47	: /
			48	: 0
			49	: 1
			50	: 2
			51	: 3
			52	: 4
			53	: 5
			54	: 6
			55	: 7
			56	: 8
			57	: 9
			58	: :
			59	: ;
			60	: <
			61	: =
			62	: >
			63	: ?
			64	: @
			65 ~ 90	: A ~ Z
			91	: [
			92	: \
			93	: ]
			94	: ^
			95	: _
			96	: `
			97 ~ 122 : a ~ z = A ~ Z
			123	: {
			124	: |
			125	: }
			126	: ~
			127	: 0x0F <<<< was ist das???
			//<<<<< make a table which shows the character set using the console app I wrote some time ago
	*/
	
	uint16_t code = sign & 0b01111111;
	if (code == 10){
		/* do something in order to make a line feed */
		return 0x00; // <<<<< the newline char is currently a space sign, please change the function signature / provide an additional function
	}
	if (code == 32){
		return 0;
	}
	if (code < 33){
		/* return a standard sign */
		return 0xFF ^ (DOTSIGN * isDotted(sign)); // <<<<< try to make another standard sign
	}
	if (code >96){// small letters match to capital letters
		if (code > 122){ // jump over the 6 characters between the capital letters and the small letters in ASCII code
			code += 6;
		}
		code -= 32;
	}
	code -=33;

#ifndef NO_AVR
	return static_cast<uint8_t>( pgm_read_byte(&(codeTable[code]))) ^ (isDotted(sign) * DOTSIGN);
		// when trusting the explanation on http://www.nongnu.org/avr-libc/user-manual/pgmspace.html this should work properly
#else
	return static_cast<uint8_t>(codeTable[code] ^ (isDotted(sign) * DOTSIGN));

#endif // !NO_AVR

}

#ifndef NO_AVR
void led::printSign(const char sign){
	led::pushByteVisible(led::signCode(sign));
	// <<<< here we should fix the problem of a new line sign.
}

void led::printDigit(const uint8_t digit){
		if (digit/10){
			#ifdef WEAK_ERROR
				led::error(2);
			#endif
			led::printSign('@');
			return;
		}
	led::printSign(48 + digit);
}

uint8_t led::printInt(const int64_t integer, const bool checkLineLength){
	uint8_t chars = (integer<0); // use chars to count how many characters we need.
	if (chars){ // print '-' if there is one
		led::printSign('-'); // <<< causes annoying flicker !!!! do not use recursion!!! 
	}
	if (abs(integer/10)){ // print recursively the higher part of the int if any
		chars += led::printInt(abs(integer/10), false);
	}
	led::printDigit((abs(integer)) % 10); // print last digit
	++chars;
	#ifdef WEAK_ERROR
	if (checkLineLength){
		if (chars > LINELENGTH){
			error(3);
		}
	}
	#endif
	return chars;
}

void led::printSignDottable(char sign, const bool dot){
	led::setDot(&sign,dot);
	led::printSign(sign);
}

void led::printSignDottableExplicit(char sign, const bool dot){
	led::pushByteVisible(  (led::signCode(sign) & (~led::DOT))    |    (dot * led::DOT)  );
}

void led::printString(const char * const string){
	uint8_t i {0};
	while (string[i]!='\0'){
		led::printSign(string[i]);
		++i;
	}
}

void led::printString(const char* string, uint8_t length){
	uint8_t i {0};
	while ((string[i]!='\0') && (i < length)){
		led::printSign(string[i]);
		++i;
	}
}

void led::clear(){
	for(uint8_t i = 0; i<LINELENGTH; ++i){
		led::pushByte(0);
	}
	led::latch();
}

void led::LFPrintString(const char* const string){
	//////////////////////////////////////////////////////////////////////////
	/*char* ptr = string;
	while (*string!='\0'){
		++ptr;
	}
	for(uint8_t i = 0; i<(ptr-string); ++i){
		ledPushByte(0);
	}
	led::latch();
	ledPrintString(string);
	*/
	//////////////////////////////////////////////////////////////////////////
	
	//we dont have more than one line in our controller
	// for our controller we want to be flash memory friendly and not fast so we do:
	// <<<<<<<< change this back some day
	led::clear();
	led::printString(string);
}

void led::LFPrintString(const char* string, uint8_t length){
	led::clear();
	led::printString(string,length);
}

void led::printDotsOnly(const uint8_t dotCode){//<<<<<< this function isnt ready for scalable linelength, please change sometime
	for (uint8_t i = 0; i<8; ++i){
		_LEDLINE_ &= ~(static_cast<uint64_t>(1)<<(DOTPOSITION + 8 * i)); // make it 0
		_LEDLINE_ |= ((static_cast<uint64_t>(1)<<(DOTPOSITION + 8 * i)) * (!! (dotCode & (1<<i) ) )    );
	}
	pushMemory();
}

void led::error(const uint16_t code) {
	//static_assert(LINELENGTH>=4,"The ledError() needs at least 4 led elements!");  <<<<<<<< config c++11
	// one day we will put this all into a class. the assertion above should be part of the ctor <<<<<<<

	uint8_t SREG_temporal = SREG; // save interrupt state
	cli();
	led::clear();
	led::printSign('E');
	hardware::delay(3000);
	// led::pushByteVisible(0x00); to push a space between 'E' and code
	if (code < 100) led::printDigit(0);
	if (code < 10) led::printDigit(0);
	led::printInt(code, false);
	hardware::delay(60000);// add some fancy blinking <<<<<<<< /// in some cases it wouldn't be useful to stop evben for 60s. make some flag for that in class later
	if (code > 99) {
		while (1) {
			led::pushBitIntern(false);// this is only to avoid compiler optimization trashing this infinite loop
		}
	}
	SREG = SREG_temporal; // activate interrupts
						  // <<< is watchdog turned of with cli()?? what behavior do you prefer?
}
#endif // !NO_AVR
