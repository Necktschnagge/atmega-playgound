/*
 * main.cpp
 *
 * Created: 09.08.2016 13:19:00
 * Author : Maximilian Starke
 */ 


/* extern headers */
#include <stdint.h>				// uint8_t..

/* own headers */
#include "f_ledline.h"
#include "f_hardware.h"
#include "f_arch.h"
#include "display_server.h"

#include "f_iopin.h"
#include "sch2.h"

void guiBootScreen(){
	hardware::delay(10);	
	led::LFPrintString("-FUSSEL-");
	hardware::delay(1000);
	uint8_t dots {0};
	for(uint8_t i = 0 ; i < 8; ++i){
		dots += 1 << (7-i);
		led::printDotsOnly(dots);
		hardware::delay(500);
	}
	for(uint8_t i = 0 ; i < 5; ++i){
		led::printDotsOnly(0x00);
		hardware::delay(500);
		led::printDotsOnly(0xFF);
		hardware::delay(500);
	}
	hardware::delay(500);
	led::clear();
	hardware::delay(500);
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

	/* returns the raw value from the HCRS04 */
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
	//bool toggle;
	
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
#if false // test for the old version of cmi - the one with hardcoded 5% delta

	/* no return */
	
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
#endif
}

void test_cmr(){
#if false // old cmi version
		init_sr();
		
		sensor::Kanalysator<int32_t,4>::Configuration config;
		config.weight_old = 950; // changed from 95 npercent to 99
		config.weight_new =  50;
		
		config.initial_badness = 80;
		config.badness_reducer = 9;
		sensor::Kanalysator<int32_t,4> analytiker(&config);
		
		analyzer::ChannellingMovementRecognizer<int32_t,4>::Zone zones[3];
		zones[0].lower_border = 150;
		zones[0].upper_border = 300;
		zones[1].lower_border = 500;
		zones[1].upper_border = 650;
		zones[2].lower_border = 800;
		zones[2].upper_border = 1000;
		
		
		
		analyzer::ChannellingMovementRecognizer<int32_t,4>::Configuration cmr_config;
		cmr_config.epsilon = 100;
		cmr_config.initial_badness = 50;
		cmr_config.max_badness = 200;
		cmr_config.pzones = &(zones[0]);
		cmr_config.rgzones = 3;
		
		analyzer::ChannellingMovementRecognizer<int32_t,4> my_cmr(cmr_config);
		
		
		
		int32_t messwert;
		int32_t ausgabe;
		int8_t _1_to_2 {0};
		int8_t _2_to_3 {0};
		//uint8_t
		while (1){
			messwert = messen();
			analytiker.input(messwert);
			
			ausgabe = analytiker.output();
			ausgabe = ausgabe * 6632 / 10000;
			uint8_t f = 0;
			uint8_t t = 0;
			my_cmr.put(ausgabe, f, t);
			if ((f == 0)&&(t == 1))
			{
				++_1_to_2;
			}
			if ((f==1)&&(t==2)){
				++_2_to_3;
			}
			if ((f == 1)&&(t == 0))
			{
				--_1_to_2;
			}
			if ((f==2)&&(t==1)){
				--_2_to_3;
			}
			led::clear();
			led::printInt(_1_to_2); // 0.06632 cm/tick. Messunng vom 13.07.2017; C++ Stunde mit Hannes
			led::printString("  ");
			led::printInt(_2_to_3);
			
			
			/*led::clear();
			led::printInt(static_cast<uint8_t>(f+1)); // 0.06632 cm/tick. Messunng vom 13.07.2017; C++ Stunde mit Hannes
			led::printString(" - ");
			led::printInt(static_cast<uint8_t>(t+1));
			*/
			
		}
#endif
}

int main(void){
	//scheduler2<10> object;
	//scheduler2<10>::interrupt_handler();
	
	led::init(8);
	guiBootScreen();
	led::clear();
	fsl::hw::IOPin iopin(fsl::hw::IOPin::Port::A,0);

	
	led::printString("D-SERVER");
	hardware::delay(2000);
	led::clear();
	
	main_of_display_server();
	
	//testing_CMI_Interpreter_with_one_HCSR04_sensor(); //noreturn
	//test_cmr();
	
	led::LFPrintString("MAIN-ERR");
	while (1) {}
}