/*
 * f_arch.h
 *
 * Created: 01.09.2016 17:38:32
 *  Author: Maximilian Starke
 */ 

/************************************************************************/
/* FPS:
	everything has to be checked!!!
	


	                                                                    */
/************************************************************************/

/************************************************************************/
/* This is for controlling the arch light line
   For using you have to run init() function first
   
   Once you runned init() you have to run controller()
   as often as you can in the background of your central control loop
   to enable the prefetching of the arch
   
   The blinking code - FUVM Code has to be stored on the EEPROM
   
   you can set free the occupied AVR controller pins with finalize()
   than you can (but you don't have to) stop running the prefetcher
   
   
   PORTS:	PortB	Bit	0	:	DATA
					Bit 1	:	CLOCK
					Bit 2	:	LATCH
			everything hardcoded
                                                                        */
/************************************************************************/


#ifndef F_ARCH_H_
#define F_ARCH_H_

#include <stdint.h>

#include "f_hardware.h"
#include <avr/io.h>

namespace arch {
	
		/* let the FUVM System run a given program on recursion level 1 */
		/* current running program will be stopped immediately and state will be deleted */
	void runProgram(uint8_t program);
	
		/* push a bit to the end of the light line */
	void pushBit(bool bit);
	
		/* send a latch signal to the light line */
	void latch(void);
	
		/* push 15 bit to the light line (MSB first) and make it visible */
	void pushLineVisible(uint16_t line);// for user needing manipulation
	
		/* init() the output pins. push some signal if not silent. run program 0 */
	inline void init(bool silence = false){//for init
		// IO register B for the lights:
		PORTB &= 0b11111000;
		DDRB |= 0b00000111;// LATCH BIT ::: CLOCK BIT ::: DATA BIT

		pushLineVisible(0x0000);
		if (!silence){
			hardware::delay(1000);
			
			uint16_t sig = 0;
			for(uint8_t count = 1; count <= 33; ++count){
				pushLineVisible(sig);
				hardware::delay(100);
				sig = (sig << 1) + !(sig & 0x8000);
			}
			hardware::delay(1000);
		}
		
		// maybe this could be left out, but for alarm function this could be useful
		runProgram(0);
	}
	
		/* set free the occupied port pins. stop reader == kill timer */	
	inline void finalize(){
		
		// stop running timer######
		
		pushLineVisible(0x0000);
		
		PORTB |= 0b0111;
		//DDRB &= 0b11111001;
		//hardware::delay(500); i don't know what this was supposed to mean for a while
		DDRB &= 0b11111000;
	}
	
		/* check whether we can read from the light buffer. if so: execute one buffer entry.
			if not: set the bufferWait Flag */
	void readBuffer();
	
		/* return the length of the instruction in bytes when the first byte is given */
	uint8_t instructionLength(const uint8_t firstByte);
	
		/* deprecated for user and high-level-programmer */
		/* combined calculating of:
					EEPROM program address (given program ID, instruction of program)		(iff counting==false)
					program counting	(up to 255)											(iff counting)
																														*/
	uint16_t EEPAddressHelper_(uint8_t program, uint8_t instruction, bool counting = false);
	
		/* calculate the eeprom address of instruction (instruction) in program (program) */
	inline uint16_t getAddress(uint8_t program, uint8_t instruction){
		return EEPAddressHelper_(program,instruction);
	}
	
		/* count the amount of programs which are present in th eeprom */
		/* the FLASH hard coded program 0 which turns all led off is not counted !!! */
	inline uint8_t programCount(void){//for gui programmer
		return (uint8_t) EEPAddressHelper_(0,0,true);
	}
	
		/* read the program name of a given program from the EEPROM */
		/* if program == 0 "OFF\0" will be written */
	void getProgramName(uint8_t program, char* string_8_bytes);// for gui programmer
	
		/* interprete the 14 INIT Bytes of the begin of a program (and update the pc) */
	
		/************************************************************************/
		/* BLINKING PROGRAM HEADER
		   0:		pointer to the next program		H Byte		------------------------------------ address of the 0th instruction (INIT PART)
		   1:										L Byte
		   2:		program name (8 characters)		Byte 0 (first letter, on the left) -> MSB
		   3:										Byte 1
		   4:										Byte 2
		   5:										Byte 3
		   6:										Byte 4
		   7:										Byte 5
		   8:										Byte 6
		   9:										Byte 7 (last letter, on the right) -> LSB
		   A:		initialization of register		Reg 0
		   B:										Reg 1
		   C:										Reg 2
		   D:										Reg 3
		   E:													------------------------------------- address of the 1st instruction (PROGRAM PART)
		   F:		...															*/
		/************************************************************************/
	void programHeaderInterpreter();
	
		/* Interpreter which deals with exactly one task, returns true if idle ( = the buffer is full) */
		/* feeds the state.light.buffer */
	bool fetchBuffer(void);// check the prg logic !!! ######
	
		/* central control sequence of light controller */
	void controller();// for user
		
}

#endif  /* F_ARCH_H_ */
