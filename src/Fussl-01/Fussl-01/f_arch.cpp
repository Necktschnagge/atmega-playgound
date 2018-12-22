/*
* f_arch.cpp
*
* Created: 01.09.2016 17:38:51
* Author: Maximilian Starke
*/

/************************************************************************/
/* FPS must be created                                                  */
/************************************************************************/




#include "f_arch.h"

#include <avr/eeprom.h>
#define F_CPU 16000000UL// <<<<<
#include <util/delay.h>
#include "f_ledline.h" // need for errors

constexpr uint16_t PROGRAMSTART {1024};		// start address of the first FUVM blinking program : points at program1 (program0 is the FLASH hard coded program which turns all led off)
constexpr uint16_t ADDRESSSPACE {4096};		// = 1<<12
static_assert(ADDRESSSPACE == 1 << 12, "Address Space Error");

constexpr uint8_t  BUFFERSIZE   {16};
constexpr uint8_t RECURSIONDEPTH {8};


/* activation record = inner state variables of the running program */
typedef struct savrecord* pavrecord;
typedef struct savrecord {
	uint8_t program;		// program number (necessary for jump reference) // max 255 programs. + program 0 (off programm)
	uint16_t pc;			// program counter = EEPROM address of next instruction
	uint8_t returnMatter;	// returning condition. is set by calling instance
	uint8_t reg[4];			// four registers
} tavrecord;


/* a buffer where the FUVM should write into. a timer controlled reader has to read it and push to the ledline */
typedef struct sbuffer* pbuffer;
typedef struct sbuffer {
	uint16_t light;
	// the real light has only 15 bits - the 15 lower bits of light
	// the MSB of light has to be zero if the tbuffer is supposed to
	// push the light config and wait for delay * x * ms
	// set the MSB = 1 to make a delay without changing light
	uint8_t delay;
	// delay time.
	// make a decision what 0 should mean ####
} tbuffer;

/* one whole configuration of the decoding system */
typedef struct slight* plight;
typedef struct slight {
	tavrecord avrecord[RECURSIONDEPTH];
	/* one program can call another as subroutine.
	program on 1. level is on avrecord[0]
	next follows on []++
	*/
	uint8_t ptr; // indexer to avrecord. current program state
	
	tbuffer buffer[BUFFERSIZE]; //global buffer ##### should be more global (I mean not twice present in "state")
	
	uint8_t read; // indexer to the position to read next entry from,
	//but check that read != write mod 16 <=> means the buffer is empty
	// first read entry, than ptr++
	uint8_t write; // indexer to the next free buffer element to write,
	// but check that write + 1 != read mod 16 <=> means ptr is on last free space.
	// You have to let this free space blind because write and ptr++ would cause a state
	// where the consumer thinks that he read everything
	// first write entry, than ptr++
	bool bufferWait; // set 0, only set to 1 if the buffer is not ready to read.
	// actors: buffer reader: set 1 if nothing to read and do not tell the time engine to plan the next buffer reader execution
	//		buffer filler: check if it is 1. if so call the buffer reader once after filling the buffer
	
	uint8_t timerCountDown;
	// what is that var ?????
} tlight;

/************************************************************************/
/* BUFFER BORDER CASES:
f... free Buffer cells
d... data in cell

case: empty buffer. reader has nothing to read:
R
-------------------------------------
| f | f | f | f | f | f | f | f | f |
-------------------------------------
W

case: full buffer. producer has no space to write
R
-------------------------------------
| d | d | d | f | d | d | d | d | d |
-------------------------------------
W

*/
/************************************************************************/

typedef struct sstate* pstate;
typedef struct sstate {
	tlight light;
	//tlight hiddenLight;// used by waking timer;
} tstate;

static tstate state;

namespace {
	/* mark at writing position WAITONLY into light */
	inline void WaitOnlyBufferPrepare(){
		state.light.buffer[state.light.write].light = 0x8000;
	}
	
	/* buffer write ptr ++ */
	inline void BufferWriteIndexerInc(){
		state.light.write = (state.light.write + 1 ) % BUFFERSIZE;
	}
	
	/* buffer read ptr ++ */
	inline void BufferReadIndexerInc(){
		state.light.read = (state.light.read + 1) % BUFFERSIZE;
	}
}

void arch::runProgram(uint8_t program){// for user
	//##### turn off timer (if any program is running it has to be stopped)
	state.light.ptr = 0;
	state.light.avrecord[0].returnMatter = 0x00;
	state.light.avrecord[0].program = program;
	state.light.avrecord[0].pc = arch::getAddress(program,0);
	state.light.read =  0;
	state.light.write = 0;
	
	state.light.bufferWait = true;
	WaitOnlyBufferPrepare();
	// ## timer will be started automatically because of bufferWait
	arch::programHeaderInterpreter();
}

using BUSY_WAIT_TYPE = uint8_t;
static constexpr BUSY_WAIT_TYPE BUSY_WAIT_RANGE{ 16 };

void arch::pushBit(bool bit){
	PORTB = (PORTB & 0b11111110) | bit;
	hardware::busy_wait<BUSY_WAIT_TYPE>(BUSY_WAIT_RANGE);
	PORTB ^= 0b00000010;
	hardware::busy_wait<BUSY_WAIT_TYPE>(BUSY_WAIT_RANGE);
	PORTB ^= 0b00000010;
	hardware::busy_wait<BUSY_WAIT_TYPE>(BUSY_WAIT_RANGE);
}

void arch::latch(void){
	PORTB ^= 0b00000100;
	hardware::busy_wait<BUSY_WAIT_TYPE>(BUSY_WAIT_RANGE);
	PORTB ^= 0b00000100;
	hardware::busy_wait<BUSY_WAIT_TYPE>(BUSY_WAIT_RANGE);
}

void arch::pushLineVisible(uint16_t line){
	//led::clear();
	//led::printString("P-");
	//led::printInt(line);
	for(int8_t i = 15; i>=0; --i){
		arch::pushBit(line & (1<<i));
	}
	//hardware::delay(1000);
	arch::latch();
	//hardware::delay(1000);
}

void arch::readBuffer(void){
	if (state.light.read!=state.light.write){
		if (state.light.buffer[state.light.read].light>>15 == 0){ // when shifting to the right, zeros come filling the left???
			arch::pushLineVisible(state.light.buffer[state.light.read].light);
		}
		//### set timer = state.light.buffer[state.light.read].delay; after time execute arch::readBuffer
		BufferReadIndexerInc();
		} else {
		state.light.bufferWait = true;
		// set a waiting signal to led output??? <<<< gui can do ths
	}
}

uint8_t arch::instructionLength(const uint8_t firstByte){// ###please check this.
	if  (	(firstByte >= (3<<6)) /* ALU Register */
	||	((firstByte & 0b11110100) == 0b10110100) /* DEC (Register) and WAIT-R */	){
		return 1; // 8 bit
	}
	if ((firstByte & 0b11110000)==0x80){ /* Jump on Condition */
		return 3; // 24 bit
	}
	if ((firstByte & 0b11110000)==0xA0){ /* CALL */
		return 8; // 64 bit
	}
	return 2; // 16 bit (for each other instruction)
}

uint16_t arch::EEPAddressHelper_(uint8_t program, uint8_t instruction, const bool counting){
	uint16_t ptr = PROGRAMSTART;
	if ((program == 0) && !counting){
		ptr = hardware::EEPNULL;
		// ledError(3); ## please delete error from manuscript this should be allowed for compatibility
		} else {
		// select the program
		--program;
		if (counting) program = 0;
		while (program || counting){
			--program;
			ptr =  (static_cast<uint16_t>(eeprom_read_byte((uint8_t*)ptr))<<8) | eeprom_read_byte((uint8_t*)(ptr+1));// read two bytes big endian
			if (hardware::isEEPNull(ptr)) {
				if (counting){
					return -program;
				}
				led::error(101);// request for non existent program ####
			}
			if ((program==0) && counting){
				led::error(1);
			}
		}
		if (counting){
			led::error(102);
		}
		// select the certain instruction
		// instruction == 0 : init part of the program;
		if (instruction){
			ptr += 14;
			--instruction;
			while (instruction){
				--instruction;
				ptr += arch::instructionLength(eeprom_read_byte((uint8_t*) ptr));
			}
		}
	}
	return ptr;
}

void arch::getProgramName(uint8_t program, char* string_8_bytes){// for gui programmer
	/* read the program name of a given program from the EEPROM */
	if (program == 0){
		char off[] = "OFF";
		hardware::copyString(string_8_bytes,off,8,false);
		} else {
		uint16_t address = arch::getAddress(program,0) + 2;
		for (uint8_t i = 0; i<8; ++i){
			string_8_bytes[i] = eeprom_read_byte((uint8_t*) (address + i));
		}
	} // ### check if running correctly
}

void arch::programHeaderInterpreter(){
	/* interprete the 14 INIT Bytes of the begin of a program (and update the pc) */
	pavrecord avrecord = &(state.light.avrecord[state.light.ptr]);
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
	if (avrecord->program==0){ // if program 0 is called there is no header to deal with
		return;
	}
	for(uint8_t i = 0; i < 4; ++i){
		avrecord->reg[i] = eeprom_read_byte((uint8_t*)(avrecord->pc+10+i));
	}
	avrecord->pc += 14;
}

bool arch::fetchBuffer(void){// check the prg logic !!! ######
	/* Interpreter which deals with exactly one task, returns true if idle ( = the buffer is full) */
	/* feeds the state.light.buffer */
	
	if ( (state.light.write+1) % BUFFERSIZE == state.light.read ){
		// no space to buffer new instructions
		return true;
	}
	
	pavrecord avrecord = &(state.light.avrecord[state.light.ptr]);
	pbuffer buffer = &(state.light.buffer[state.light.write]);
	
	if (avrecord->program == 0){
		// realize the non ordinary 0-program function
		buffer->light = 0x0000; // set light to buffer
		buffer->delay = 0xFF;  // set delay time after light to buffer
		BufferWriteIndexerInc();
		WaitOnlyBufferPrepare();
		return false;
	}
	
	uint8_t instruction[3] = {eeprom_read_byte((uint8_t*)avrecord->pc),
		eeprom_read_byte((uint8_t*) ((avrecord->pc+1)% ADDRESSSPACE)),
	eeprom_read_byte((uint8_t*) ((avrecord->pc+2)% ADDRESSSPACE))};
	
	uint8_t* const reg2 = &(  avrecord->reg[  (instruction[0] & 0b00001100)>>2  ]  );/// constexpr ??? <<<<<<
	uint8_t* const reg3 = &(  avrecord->reg[    (instruction[0] & 0b00000011)   ]  );
	uint8_t* const reg6 = &(  avrecord->reg[  (instruction[1] & 0b00001100)>>2  ]  );
	uint8_t* const reg7 = &(  avrecord->reg[    (instruction[1] & 0b00000011)   ]  );
	
	uint8_t seg;
	
	if (instruction[0] & 0b10000000){
		if (instruction[0] & 0b01000000){
			// ALU Register Operations: 0b11 XX R2 R3
			seg = (instruction[0] & 0b00110000)>>4;
			if (seg==0){
				//ADD
				*reg2 = *reg2 + *reg3;
			}
			if (seg==1){
				//MUL
				*reg2 = (*reg2) * (*reg3);
			}
			if (seg==2){
				//DIV
				*reg2 = *reg2 / *reg3;
			}
			if (seg==3){
				//MOD
				*reg2 = *reg2 % *reg3;
			}
			avrecord->pc += 1;
			} else {
			if (instruction[0] & 0b00100000){
				if (instruction[0] & 0b00010000){
					//0xB
					if (instruction[0] & 0b00001000){
						if (instruction[0] & 0b00000100){
							// DEC: 0b1011 11 RR
							*reg3 -=1;
							avrecord->pc += 1;
							} else {
							if (instruction[0] & 0b00000010){
								if (instruction[0] & 0b00000001){
									// RET: 0xBB | MATTER
									avrecord->pc += 2;
									if (instruction[1] & (avrecord->returnMatter)){//check matter match
										if (state.light.ptr==0){
											led::error(103);
										}
										--state.light.ptr;
									}
									} else {
									// 0xBA
									seg = (instruction[1] & 0b11110000)>>4;
									if (seg==0){
										// MOVE: 0b 1011 1010 | 0000 RD RS
										*reg6 = *reg7;
									}
									if (seg==1){
										//SET-LR: 0b 1011 1010 | 0001 RH RL
										buffer->light = ((*reg6<<8) + *reg7) & ~(1<<15);
									}
									if (seg==2){
										//SWAP: 0b 1011 1010 | 0010 RR RR
										seg = *reg6;
										*reg6 = *reg7;
										*reg7 = seg;
										seg = 2;
									}
									if (seg==3){
										//NEG: 0b 1011 1010  | 0011 XX RR
										*reg7 = - (*reg7);//
									}
									avrecord->pc += 2;
									if (seg==4){
										//RESET: 0b 1011 1010 | 0100 XXXX
										avrecord->pc = arch::getAddress(avrecord->program,0);
										arch::programHeaderInterpreter();
									}
								}
								} else {
								avrecord->pc += 2;
								if (instruction[0] & 0b00000001){
									//WAIT-I: 0xB9 | Immediate
									buffer->delay = instruction[1];
									BufferWriteIndexerInc();
									WaitOnlyBufferPrepare();
									} else {
									//JMP: 0xB8 | local address
									if (instruction[1] == 0) {
										//### throw error: you should use reset instead
										} else {
										avrecord->pc = arch::getAddress(avrecord->program,instruction[1]);
									}
								}
							}
						}
						} else {
						if (instruction[0] & 0b00000100){
							// WAIT-R: 0b1011 01 RR
							buffer->delay = *reg3;
							BufferWriteIndexerInc();
							WaitOnlyBufferPrepare();
							avrecord->pc += 1;
							} else {
							//SET-RI: 0b1011 00 RR | Immediate
							*reg3 = instruction[1];
							avrecord->pc += 2;
						}
					}
					} else {
					// CALL: 0b1010 QQQQ | program | instruction | return matter | immediate/reg_0 | immediate/reg_1 | i/r_2 | i/r_3
					// QQQQ:		Q==0	->		immediate	->		(8 bit immediate)
					//				Q==1	->		register	->		(XX CC RD RS)	:	RS .. source register; write back in RD iff CC==11
					// this version is always without writing BACK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// iff the return matter is 0 call will empty the avrecord and start from a blanc list again
					
					uint8_t matter = eeprom_read_byte((uint8_t*) ((avrecord->pc+3)% ADDRESSSPACE));
					
					if ((state.light.ptr != RECURSIONDEPTH-1) || (matter == 0)){
						
						// fetch more instructions
						// 5x fetching: % is not necessary because programmer should provide presence of the parameters <<<<<<< catch this as an error and make all lights off or such a thing
						uint8_t values[4] = {	eeprom_read_byte((uint8_t*) ((avrecord->pc+4)% ADDRESSSPACE)),
							eeprom_read_byte((uint8_t*) ((avrecord->pc+5)% ADDRESSSPACE)),
							eeprom_read_byte((uint8_t*) ((avrecord->pc+6)% ADDRESSSPACE)),
						eeprom_read_byte((uint8_t*) ((avrecord->pc+7)% ADDRESSSPACE))};
						
						avrecord->pc += 8; //<<<< catch if user makes such code which will make me an eeprom read because of the pc which is out of range / ADRESSSpace
						state.light.ptr++;
						if (matter == 0) state.light.ptr = 0;
						
						pavrecord av2 = &(state.light.avrecord[state.light.ptr]);
						av2->pc = arch::getAddress(instruction[1],instruction[2]);
						av2->program = instruction[1];
						av2->returnMatter = matter;
						for(uint8_t i = 0; i<4; ++i){
							av2->reg[i] = (instruction[0] & (1<<(3-i))) 	? /*reg*/			avrecord->reg[values[i] & 0b11]
							: /*immediate*/		values[i]
							;
						}
						// if called program header:
						if (instruction[2] == 0) {
							arch::programHeaderInterpreter();
						}
						} else {
						//throw little error <<<<
					}
				}
				} else {
				seg = (instruction[0] & 0b00001100)>>2;
				if (instruction[0] & 0b00010000){
					// ALU IMMEDIATE Operations: 0x1001 XX RR | Immediate
					avrecord->pc += 2;
					if (seg==0){
						// ADD-I
						*reg3 += instruction[1];
					}
					if (seg==1){
						// MUL-I
						*reg3 *= instruction[1];
					}
					if (seg==2){
						// DIV-I
						*reg3 /= instruction[1];
					}
					if (seg==3){
						// MOD-I
						*reg3 = *reg3 % instruction[1];
					}
					} else {
					// JUMP ON CONDITION: 0x 1000 XX RR | Immediate | Instruction - Address
					avrecord->pc += 3;
					if (
					( (seg==0) && (*reg3 == instruction[1]) ) // JMP-EQ
					||
					( (seg==1) && (*reg3 != instruction[1]) ) // JMP-NE
					||
					( (seg==2) && (*reg3 < instruction[1]) ) // JMP-LT
					||
					( (seg==3) && (*reg3 > instruction[1]) ) // JMP-GT
					){
						avrecord->pc = arch::getAddress(avrecord->program,instruction[2]);
						if (instruction[2] == 0){
							arch::programHeaderInterpreter();
						}
					}
				}
			}
		}
		} else {
		//SET-LI: 0b0 L LL LLLL | LLLL LLLL;
		buffer->light = ((instruction[0]<<8) + instruction[1]) & ~(1<<15);
		avrecord->pc += 2;
	}
	return false;
}

void arch::controller()	{// for user
	/* central control sequence of light controller */
	
	constexpr uint8_t max = 20; // <<<< change if this could be good, should be defined on top of this file
	for (uint8_t i = 0; (i<max) && (!arch::fetchBuffer()) ; ++i);
	
	if (state.light.bufferWait){
		state.light.bufferWait = false;
		arch::readBuffer();
	}
}
