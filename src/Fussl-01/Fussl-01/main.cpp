/*
 * Fussl-01.cpp
 *
 * Created: 09.08.2016 13:19:00
 * Author : F-NET-ADMIN
 */ 

#include <stdint.h>				// uint8_t..
#include <avr/eeprom.h>			//eeprom_read_byte()

#include "f_ledline.h"
#include "f_hardware.h"
#include "f_arch.h"
#include "f_gui.h"
#include "f_test.h"


void foo(){
	

//#include <avr/pgmspace.h>

//constexpr uint8_t PASSWORD = 77;			// password for main menu to edit critical settings

//#define DOT 128					// constant to add to signs to make them dotted 'A' + DOT is a dotted A;




//uint8_t arch::programCount();
//const uint16_t PROGRAMCOUNT = arch::programCount(); // there is a better c++ alternative // <<<<<<<
}



void guiBootScreen(){
		led::LFPrintString("-FUSSEL-");
		for(uint8_t i = 0 ; i < 5; ++i){
			led::printDotsOnly(0xFF);
			hardware::delay(500);
			led::printDotsOnly(0x00);
			hardware::delay(500);
		}
		led::clear();
}

void after_selecting(){
	// do nothing
	led::clear();
}

int main(void){
	//###
	// please check the board frequency of quartz and redefine fcpu in hardware.cpp
	// define dot position in ledline.cpp -> please find out it's hardware configuration
	
	/* test code */
	test::led_t::run();
	
	
	led::init(8);
	guiBootScreen();
	
	led::LFPrintString("ARCH...");
	hardware::delay(2000);
	arch::init();
	led::clear();
	
	ArcProgramItemManager programItemManager;
	programItemManager.init(nullptr,after_selecting,true);
	ItemSelector::initialisation(2,0,1,&programItemManager);
	ItemSelector::run();
	while (1){
		arch::controller();
	}
}