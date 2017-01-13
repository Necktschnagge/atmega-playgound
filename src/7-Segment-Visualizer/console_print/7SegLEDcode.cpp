
#include <iostream>
#include <stdint.h>
 
static uint8_t codeMEM;

bool seg(uint8_t pos){
	return (1<<pos) & codeMEM;
}

void printLED(uint8_t code){
	using namespace std;
	codeMEM = code;
	cout << "+++++++++++++++\n";
	if (seg(2))
		std::cout << "+       ----   \n";
	else cout << "+\n";
	cout << "+     ";
	if (seg(1))
		std::cout << "/";
	else cout <<' ';
	if (seg(3)) cout << "    /   \n";
	else cout << '\n';
	if (seg(0))
		std::cout << "+     ----     \n";
	else cout << "+\n";
	if (seg(4)) 	std::cout << "+   /";
	else cout << "+    ";
	if (seg(6)) cout <<"    /     \n";
	else cout << '\n';
	if (seg(5)) std::cout << "+   ----  ";
	else cout <<             "+         ";
	if (seg(7)) std::cout << static_cast<char>(2);
	std::cout << "    \n";
	
	/*
	for (char i = 1; i!=0; ++i){
		std::cout << "Z " << static_cast<int16_t>(i);
		std::cout << "  " << i << "  \t";
	}
	*/
}

int main (){
	
	
									//		!		"	#		$	%		&	' '		(	)		*	+		,	-		.	/		0	1		2	3		4	5		6	7		8	9		:	;		<	=		>	?		@
	const uint8_t codeTable[]         = {   0xC8, 0x0A, 0x7F, 0x6E, 0x99, 0x3F, 0x00, 0x16, 0x4C, 0x2B, 0x49, 0x40, 0x01, 0x80, 0x19, 0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x81, 0xC0, 0x39, 0x21, 0x63, 0x9D, 0x5D,
		
									/*	A		B	C		D	E		F	G		H	I		J	K		L	M		N	O		P	Q		R	S		T	U		V	W		X	Y		Z*/
										0x5F, 0x73, 0x31, 0x79, 0x37, 0x17, 0x76, 0x53, 0x12, 0x78, 0x3B, 0x32, 0x5E, 0x51, 0x71, 0x1F, 0x4F, 0x57, 0x66, 0x4C, 0x70, 0x2A, 0x7A, 0x5B, 0x6B, 0x3C,
										
									/*	[		\	]		^	_		`	{		|	}		~*/
										0x36, 0x43, 0x6C, 0x0E, 0x20, 0x08, 0x27, 0x5A, 0x2D, 0x75};
	
	for(uint8_t i = 0; i<sizeof(codeTable); ++i){
		std::cin.get();
		printLED(codeTable[i]);
	}
	
}
