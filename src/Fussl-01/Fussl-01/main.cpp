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
#include "scheduler.h"
#include "f_hcsr04.h"

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

void init_sr(){
	DDRB = 0b00000011;
	PORTB = 0b00000010;
	// und init für netsend:
	DDRB = 0b00011011;
	PORTB = 0b00011010;
	//feedback_trigger, trigger_out, data 
}

void reset_sr(){
	PORTB = 0b0;
	hardware::delay(5);
	PORTB = 0b00000010;
}

int32_t messen(){
		PORTB |= 0b00000001;
		hardware::delay(1);
		PORTB &= 0b11111110;

		while ((PINB & 0b00000100) == 0){}
		TCCR1A = 0x00; // ~ NO PWM
		TCNT1 = 0; // counter starts at zero
		TCCR1B = 0b00000011; // CTC (Clear Timer on Compare Match) and start timer running from ext source on,
		while (1){
			if ((PINB & 0b00000100) == 0){
				TCCR1B = 0;
				return TCNT1;
			}
			if (TCNT1 > 6000){
				TCCR1B = 0;
				reset_sr();
				return 0;
			}
		}
}

void show_string(const char* str){
	led::printString(str);
	hardware::delay(1000);
	led::clear();
	
}

bool get_feedback(){
	return (PINB & (1<<5));
}

void sendbit(bool value){
	static bool last_feedback {0};
	while (get_feedback()==last_feedback);
	PORTB &= 0b11110111;
	uint8_t v = value;
	PORTB |= (v<<3);
	bool toggle;
	
}

void unidirectional_led_line_test_by_testing_single_segments_numbers_and_some_characters_001(){
	for (uint8_t pos = 0; pos < 8; ++pos){
		// Test all segments on that particular LED character
		
		// single segments:
		for (uint8_t shift = 0; shift <8; ++shift){
			led::clear();
			led::pushByte(1<<shift);
			for (uint8_t padding = 0; padding < pos; ++padding){
				led::pushByte(0);
			}
			led::latch();
			hardware::delay(3000);
		}
		
		// Try numbers:
		for (uint8_t ziffer = 0; ziffer < 10; ++ziffer)
		{
			led::clear();
			led::printDigit(ziffer);
			for (uint8_t padding = 0; padding < pos; ++padding){
				led::pushByteVisible(0);
			}
			hardware::delay(3000);
		}
		
		// Try characters from A to F
		led::clear();
		led::printSign('A');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
		
		led::clear();
		led::printSign('B');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
		
		led::clear();
		led::printSign('C');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
		
		led::clear();
		led::printSign('D');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
		
		led::clear();
		led::printSign('E');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
		
		led::clear();
		led::printSign('F');
		for (uint8_t padding = 0; padding < pos; ++padding){
			led::pushByteVisible(0);
		}
		hardware::delay(3000);
	}
}

void measurement_sequence_500_and_display_afterwards(){

	int32_t data[501];

	nochmal:
	led::LFPrintString("miss");
	//int32_t n = 0;
	int16_t i = 0;
	while (1){
		
		data[i] = messen();
		//m = (m * 68679) / 100000;
		//n = (n * 95 + m *5 ) / 100;
		//if (!i) {
		//	led::clear();
		//	led::printInt(n);
		//}
		
		++i;
		//i %= 100;
		//hardware::delay(1000);
		if (i > 500){
			// ausgeben
			led::LFPrintString("Ausgabe");
			hardware::delay(4000);
			led::clear();
			
			for (uint16_t j = 0; j < 501; ++j){
				led::printInt(j);
				led::printSign('|');
				hardware::delay(1000);
				
				led::clear();
				led::printInt(data[j]);
				hardware::delay(4000);
				led::clear();
			}
			goto nochmal;
		}
		
	}
	
}

void arch_prototype_release_blink_up_and_l_to_r() {
	/* no return */ 
	// please check the board frequency of quartz and redefine fcpu in hardware.cpp
	// define dot position in ledline.cpp -> please find out it's hardware configuration
	
	// our code sequence for the first release:
	/*
	this comment was a single flickering LED to be sure ISP progging was successfull:
	DDRC |= 0b00000111;// LATCH BIT ::: CLOCK BIT ::: DATA BIT
	PORTC &= 0b11111000;
	for (uint8_t a= 0; a <10; ++a){
		hardware::delay(2000);
		PORTC = 1;
		hardware::delay(2000);
		PORTC = 0;
	}
	*/
	arch::init();
	while (1){
		for(uint16_t speed_by_delay = 2000; speed_by_delay > 599; speed_by_delay-=200){
			arch::pushLineVisible(0b0);
			hardware::delay(speed_by_delay);
			arch::pushLineVisible(0b100000001);
			hardware::delay(speed_by_delay);
			arch::pushLineVisible(0b110000011);
			hardware::delay(speed_by_delay);
			arch::pushLineVisible(0b111000111);
			hardware::delay(speed_by_delay);
			arch::pushLineVisible(0b111101111);
			hardware::delay(speed_by_delay);
			arch::pushLineVisible(0b111111111);
			hardware::delay(speed_by_delay);
		}
		arch::pushLineVisible(0);
		hardware::delay(2000);
		for(uint8_t i = 0; i<= 12; ++i){
			arch::pushLineVisible(1<<i);
			hardware::delay(1300);
		}
	}
}

void testing_CMI_Interpreter_with_one_HCSR04_sensor(){
	/* no return */
	led::init(8);
	guiBootScreen();
	led::clear();

	init_sr();
	
	sensor::Kanalysator<int32_t,4>::Configuration config;
	config.weight_old = 990; // changed from 95 npercent to 99
	config.weight_new =  10;
	
	config.initial_badness = 80;
	config.badness_reducer = 9;
	sensor::Kanalysator<int32_t,4> analytiker(&config);
	
	int32_t messwert;
	int32_t ausgabe;
	//uint8_t
	while (1){
		messwert = messen();
		analytiker.input(messwert);
		
		ausgabe = analytiker.output();
		led::clear();
		led::printInt(ausgabe * 6632 / 10000); // 0.06632 cm/tick. Messunng vom 13.07.2017; C++ Stunde mit Hannes
	}
}

int main(void){
	
	testing_CMI_Interpreter_with_one_HCSR04_sensor(); //noreturn
	
	led::LFPrintString("MAIN-ERR");
	while (1) {}
}