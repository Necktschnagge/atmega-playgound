/*
* f_ncrg.h
*
* Created: 13.07.2018 04:36:10
*  Author: F-NET-ADMIN
*/


#ifndef F_NCRG_H_
#define F_NCRG_H_

#include <stdint.h>
#include "f_time.h"

namespace fsl {
	
	namespace dev {
		
		template <typename metric, time::EMT time_per_metric_unit>
		class non_continuously_running_gadget {
			
			struct config {
				metric max_running_time;
				metric turn_on_costs;
				metric rest_factor;
				metric min_remaining_start_time;
			};
			
			config* cptr;
			metric current_time_consumption;
			time::EMT last_update_time;
			fsl::hw::gpio_pin pin;
			
			metric target_running_time_remaining;
			
			static constexpr fsl::lg::single_flags::flag_id ON{ 0 }, PIN_INVERTED{ 1 }, INSTANTLY{ 2 }, ONCE_UNTIL_REST{ 3 }, FULL_REST{ 4 }, EARLIEST_GET_READY{ 5 };
			
			fsl::lg::single_flags flags;
			/*
			enum class running_mode : uint8_t {
				OFF = 0;
				INSTANTLY_FULL_REST = 1;
				INSTANTLY_EARLIEST_TURN_ON = 2;
				REMAINING_TIME_FULL_REST;
				REMAINING_TIME_EARLIEST_TURN_ON;
				REMAINING_TIME_EARLIEST_GET_READY;
				ONCE_UNTIL_REST_PHASE;
				// x is pin inverted
				};
			*/
			inline void turn_on(){ pin.write_output(!flags.get(PIN_INVERTED)); }
			void turn_off(){ pin.write_output(flags.get(PIN_INVERTED)); }
			bool on(){ return pin.read_PORT() ^ flags.get(PIN_INVERTED); }
			
			public:
			
			// give the gadget a time update
			void update(const time::EMT& now){
				
			}
			
			metric get_current_time_consumption(){ return current_time_consumption; }
			
			inline void turn_on_instantly_full_rest(){ flags.set_true(ON); flags.set_true(INSTANTLY); flags.set_true(FULL_REST); }
			inline void turn_on_instantly_earliest_time_on(){ flags.set_true(ON); flags.set_true(INSTANTLY); flags.set_false(FULL_REST); }
				
			inline void turn_on_once_until_rest_phase(){ target_running_time_remaining}
		};
	}
}



#endif /* F_NCRG_H_ */