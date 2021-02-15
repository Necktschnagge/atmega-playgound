/*
* display_server.cpp
*
* Created: 19.02.2018 13:13:51
* Author: Maximilian Starke
*/

#include "display_server.h"

void DServer::apply(bool nbit)
{
	if (bit == 0){ // receiving opcode, receiving started from beginning
		int_value = 0;
		char_value = 0;
		opcode = static_cast<uint8_t>(nbit) * 2;
	}
	if (bit == 1){ // receiving second bit of opcode
		opcode |= static_cast<uint8_t>(nbit);
	}
	
	// read data that has to be displayed:
	if ((2 <= bit) && (bit < 2 +8 )){
		char_value |= (static_cast<char>(nbit) << (8+1-bit) ); // for char
	}
	if ((2 <= bit) && (bit < 2 + 32)){
		int_value |= (static_cast<int32_t>(nbit) << (32 +1 - bit)); // for int32_t
	}

	// check whether operation should be executed:
	// and reset to initial state:
	if ((bit >= 1 + 0) && (opcode == 0b00)) { // clear screen command
		led::clear();
		bit = 0;
		return;
	}
	if ((bit >= 1 + 0) && (opcode == 0b11)) { // non defined opcode - display error
		led::LFPrintString("ERR OPC");
		hardware::delay(800);
		bit = 0;
		return;
	}
	if ((bit >= 1 + 32) && (opcode == 0b01)){ // display int32_t
		led::printInt(int_value);
		bit = 0;
		return;
	}
	if ((bit >= 1 + 8)&&(opcode == 0b10)){ // display char
		led::printSign(char_value);
		bit = 0;
		return;
	}
	
	++bit; // increase reading position
}

void endless_server_loop()
{
	DServer dserver;
	
	uint8_t zeros {0}; // count received zeros for an reset of recognizing.
	uint8_t cycle_5{0};
	
	while (true){
		led::printDotsOnly(0b11000000);
		while (!client_latch()); // wait until latch is high;
		led::printDotsOnly(0b00000000);
		bool data = client_data(); // read data bit;

		cycle_5 = (cycle_5 + 1) % 5; // increase cycle position for ignoring every 5th bit
		if (cycle_5){ // on clycle_5 == 0 data will be ignored.
			dserver.apply(data);
		}
		if (! data){ // received a "0"
			++zeros;
			if (zeros == 5){ // if received five zeros continuously, reset
				cycle_5 = 0;
				zeros = 0;
				dserver.reset();
			}
			} else { // received a "1"
			zeros = 0;
		}
		set_confirm(true);
		led::printDotsOnly(0b11000011);
		while (client_latch());
		led::printDotsOnly(0b00000000);
		set_confirm(false);
	}
}
