/*
* f_hcsr04.h
*
* Created: 16.06.2017
* Author: Maximilian Starke
*
*	FILE PROGRESS STATE:
*
*/

#ifndef F_HCSR04_H
#define F_HCSR04_H

#include <stdint.h>

namespace Sensor {
	
	class HCSR04 {
		public:
		static constexpr int16_t OUT_OF_RANGE {-1};
		
		private:
		
		
		// states powered on, powered off,
		public:
		
		/* takes handles to pins */
		HCSR04();
		
		/* measures and returns the distance, returns OUT_OF_RANGE if bad measurement */
		int16_t get_distance();
		
		
	};
	
	/* channeling measurement channel Interpreter */
	template <typename Metric, uint8_t channels>
	class CMI {
		static_assert(channels <= 255,"non-possible error occurred");
	public:
		class Configuration;
		
	private:
	/* private types */
		class Channel {
		public:
			Metric average;
			Metric delta;
			uint8_t badness;
			
				/* create a Channel */
			Channel(const Metric& initial_value, const Configuration* config){
				average = initial_value;
				delta = initial_value / 20; // wegkapseln... mit config 5% ist vielleicht nicht immer was man will oder braucht
				badness = config->initial_badness;
			}
			
				/* make the Channel 'worse' */
			inline void inc_badness(){
				if (badness != 255) ++badness;
			}
			
				/* make the Channel 'better' */
			inline void dec_badness(const Configuration* config){
				badness = (static_cast<uint16_t>(badness) * config->badness_reducer)
					/ (static_cast<uint16_t>(config->badness_reducer) + 3);
			}
				
			/* try to apply value */
			inline bool accumulate(const Metric& value, const Configuration* config){
				if ((value < average - delta)  || (value > average + delta)){
					inc_badness();
					return false;
				}
				// kann man in nested classes auf non-static member der nesting class zugreifen? 				// und was bitteschön soll dann da passieren?
					//nein kann man nicht. Die nested class enthält keine referenz auf outer class (nesting class).
					// man könnte ihr höchstens eine geben 
				// komischerweise akzeptiert dieser avrgcc das erstmal beim Compilieren. Getestet hab ich das Verhalten, welches produziert wird aber nicht.
				// MSVC und (g++ (in C++11 und 14)) akzeptieren es nicht. Was der Standard schreibt, müsste ich  noch lesen.
				// in sämtlichen Foren simnd die Leute sich zum Teil uneinig und wissen es auch nicht richtig.
				average = (average * config->weight_old + value * config->weight_new) / config->weight_sum();
				dec_badness(config);
				// maybe update accumulated delta
				return true;
			}
			
			inline bool operator < (const Channel& op) const {
				return badness < op.badness;
			}
			
		};
	/* private data */
		Channel ch[channels];
		
	/* private methods */
	
			/* swap a channel array entry with its predecessor */
		void swap_with_previous(uint8_t channel){
			Channel swap { ch[channel] };
			ch[channel] = ch[channel-1];
			ch[channel-1] = swap;
			/* some byte-wise mem swap would be better of course */
		}
		
			/* only look for the given channel array position
				whether this channel is better than predecessors 
					if so than swap along to build the correct order */
		void smart_sort(uint8_t channel){
			again_smart_sort:
			if (channel == 0) return;
			if (ch[channel] < ch[channel-1]){
				swap_with_previous(channel);
				--channel;
				goto again_smart_sort;
			}
		}
		
	public:
	/* public types */
		class Configuration {
		public:
			Metric weight_old;
			Metric weight_new;
			inline Metric weight_sum(){ return weight_new + weight_old; }
			
			uint8_t initial_badness;
			uint8_t badness_reducer;
			// ?type delta_accumulator;
			
			};
			
	/* public data */
		Configuration* configuration;
	
	/* public methods */
			/* enter a new measured value */
		void input(const Metric& value){
			uint8_t i;
			for (i = 0; i<channels; ++i){ // go through all channels and try to accumulate new value
				if (ch[i].accumulate(value,configuration)) {
					smart_sort(i); // i was updated and got 'better' so it might need to be pushed to the left.
					goto match;
				}
			}
			no_match: // applies if the value did not match any of the channels
				new (&ch[channels-1]) Channel(value, configuration); // replace the worst channel in array by a new channel
				smart_sort(channels-1); // maybe the new channel is not the worst
				return;
			match: // applies if the value matched to one channel
				++i; // skip the current (matching) channel
				while(i<channels){ // make all residual channels worse too
					ch[i].inc_badness();
					++i;
				}
				return;
		}
			/* return current measurement result */
		inline const Metric& output(){ return ch[0].average; }
	};
	
}

#endif /*F_HCSR04_H*/


/*
1->3
2->6
3->1
4->7
5->8
6->2
7->4
8->5
*/