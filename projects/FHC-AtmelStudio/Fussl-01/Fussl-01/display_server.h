/*
* display_server.h
*
* Created: 19.02.2018 13:13:51
* Author: Maximilian Starke
*/


#ifndef __DISPLAY_SERVER_H__
#define __DISPLAY_SERVER_H__

#include <avr/io.h>

#include "f_hardware.h"
#include "f_ledline.h"

class DServer {
	int32_t int_value;
	char char_value;
	
	uint8_t opcode;
	
	uint8_t bit; // position of reading opcode-argument block.
	
	/*
	0	1		2+0	2+n
	opcode		data		...where n is operand size in bits
	
	opcodes:	meaning:				size n:			encoding:
	00			clear screen			0				-
	01			display int32_t			32				msb is sent first, lsb is sent last
	10			display char			8				msb is sent first, lsb is sent last
	11			undefined, ERR OPC		0
	*/
	
	public:
	DServer() : opcode(0), bit(0) {}
	
	void apply(bool nbit);
	
	void reset(){bit = 0;}
	
};

inline void init_port_C_server(){
	PORTC = 0b00000000;
	DDRC  = 0b00000100;
}

inline bool client_data(){ return !(PINC & 0b001); }
	
inline bool client_latch(){ return !(PINC & 0b010); }
	
inline void set_confirm(bool value){
	(!value) ? (PORTC |= 0b100) : (PORTC &= ~0b100);
}

void endless_server_loop(); // no return

inline void main_of_display_server() { // no return.
	init_port_C_server();
	endless_server_loop();
}

#endif //__DISPLAY_SERVER_H__
