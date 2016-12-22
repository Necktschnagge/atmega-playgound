/*
 * f_arch.h
 *
 * Created: 01.09.2016 17:38:32
 *  Author: F-NET-ADMIN
 */ 


/************************************************************************/
/* This is for controlling the arch light line
   For using you have to run init() function first
   
   When you already runned init() you have to run controller()
   as often as you can in the background of your central control loop
   to enable the prefetching of the arch
   
   The blinking code - FUVM Code has to be stored on the EEPROM
   
   you can set free the occupied AVR controller pins with finalize()
   than you can (but you don't have to) stop running the prefetcher
                                                                        */
/************************************************************************/


#ifndef F_ARCH_H_
#define F_ARCH_H_



#include <stdint.h>

#include "f_hardware.h"
#include <avr/io.h>

namespace arch {	
	
	void runProgram(uint8_t program);// for user
	
	void pushBit(bool bit);
		/* push a bit to the end of the light line */
	
	void latch(void);
		/* send a latch signal to the light line */
	
	void pushLineVisible(uint16_t line);// for user needing manipulation
		/* push 15 bit to the light line (MSB first) and make it visible */
	
	inline void init(){//for init
		
		// IO register B for the lights:
		DDRB |= 0b00000111;// LATCH BIT ::: CLOCK BIT ::: DATA BIT
		PORTB &= 0b11111000;
		
		pushLineVisible(0xFFFF);
		hardware::delay(1000);
		pushLineVisible(0x0000);
		
		// maybe this could be left out, but for alarm function this could be useful
		runProgram(0);
	}
	
	inline void finalize(){// for finalizing
		/* set free the occupied port pins */
		
		// stop running timer##
		
		pushLineVisible(0x0000);
		
		PORTB |= 0b0111;
		DDRB &= 0b11111001;
		hardware::delay(500);
		DDRB &= 0b11111000;
	}
	
	void readBuffer(void);
		/* check whether we can read from the light buffer. if so: execute one buffer entry. if not: set the bufferWait Flag */
	
	uint8_t instructionLength(const uint8_t firstByte);
		/* return the length of the instruction in bytes when the first byte is given */
	
	uint16_t EEPAddressHelper_(uint8_t program, uint8_t instruction, const bool& counting = false);
		/* deprecated for user and high-level-programmer */
		/* cinombined calculating of:
					EEPROM program address (given program ID, instruction of program)		(iff counting==false)
					program counting	(up to 255)											(iff counting)
																														*/
	
	inline uint16_t getAddress(uint8_t program, uint8_t instruction){
		/* calculate the eeprom address of instruction (instruction) in program (program) */
		return EEPAddressHelper_(program,instruction);
	}
	
	inline uint8_t programCount(void){//for gui programmer
		/* count the amount of programs which are present in th eeprom */
		/* the FLASH hard coded program 0 which turns all led off is not counted !!! */
		return (uint8_t) EEPAddressHelper_(0,0,true);
	}
	
	void getProgramName(uint8_t program, char* string_8_bytes);// for gui programmer
		/* read the program name of a given program from the EEPROM */
		/* if program == 0 "OFF\0" will be written */
	
	void programHeaderInterpreter();
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
	
	bool fetchBuffer(void);// check the prg logic !!! ######
		/* Interpreter which deals with exactly one task, returns true if idle ( = the buffer is full) */
		/* feeds the state.light.buffer */
	
	void controller();// for user
		/* central control sequence of light controller */
		
}

#endif  /* F_ARCH_H_ */
