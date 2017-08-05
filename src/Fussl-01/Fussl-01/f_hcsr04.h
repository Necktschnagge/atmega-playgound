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

namespace sensor {
#if false	
	class HCSR04 {
		public:
		static constexpr int16_t OUT_OF_RANGE {0};
		
		private:
		
		
		// states powered on, powered off,
		public:
		
		/* takes handles to pins */
		HCSR04();
		
		/* measures and returns the distance, returns OUT_OF_RANGE if bad measurement */
		int16_t get_distance();
	};
#endif
	
	/* channeling measurement Interpreter */
	template <typename Metric, uint8_t channels>
	class CMI {
		static_assert(channels <= 255,"too many channels!");
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
			Channel() : average(0), delta(0), badness(255) {}
				
				/* create a Channel */
			Channel(const Metric& initial_value, const Configuration* config);
			
				/* make the Channel 'worse' */
			inline void inc_badness(){ if (badness != 255) ++badness; }
			
				/* make the Channel 'better' */
			inline void dec_badness(const Configuration* config){
				badness = (static_cast<uint16_t>(badness) * config->badness_reducer)
					/ (static_cast<uint16_t>(config->badness_reducer) + 3);
			}
				
				/* try to apply value */
			inline bool accumulate(const Metric& value, Configuration* config);
			
				/* comparison by badness */
			inline bool operator < (const Channel& op) const { return badness < op.badness; }
			
				/* destroy a channel */
			inline ~Channel() {}
		};
		
	/* private data */
	
		Channel ch[channels];
		
	/* private methods */
	
			/* swap a channel array entry with its predecessor */
		inline void swap_with_previous(uint8_t channel);
		
			/* only look for the given channel array position
				whether this channel is better than predecessors 
					if so than swap along to build the correct order */
		inline void smart_sort(uint8_t channel);
		
	public:
	/* public types */
	
		class Configuration {
		public:
				/* weight of the old values */
			Metric weight_old;
				/* weight to accumulate the new values */
			Metric weight_new;
				
				/* weight sum (divisor in calling function) */
			inline Metric weight_sum(){ return weight_new + weight_old; }
			
				/* badness for a new created Channel*/
			uint8_t initial_badness;
				/* badness will be reduced on match by factor ~ / (~ + 3) */
			uint8_t badness_reducer;
			
			// maybe we want do accumulate the delta and specify an initial delta later here
			// ?type delta_accumulator;
			};
			
	/* public data */
	
		Configuration* configuration;
	
	/* public methods */
	
			/* enter a new measured value */
		void input(const Metric& value);
		
			/* return current measurement result */
			/* do not interrupt output by input or vice versa */
		inline const Metric& output(){ return ch[0].average; }
			
			/* c-tor */
		CMI(Configuration* configuration) : configuration(configuration) {}
	};
	
	template<typename Metric, uint8_t channels>
	using Kanalysator = CMI<Metric,channels>;
	
	
	
	
	
		/* channeling movement recognizer */
	template<typename Metric, uint8_t channels>
	class CMR {
	public:
		/* Forward declarations */
		static constexpr uint8_t NO_ZONE {255};
		class Configuration;
		using Event = uint8_t [2];
			static_assert(sizeof(Event) == 2,"CMR::Event funktioniert nicht");
	private:
		/* private types */
		class Channel {
			
		private:
				/* 0 is faded out, that means we ~Channel(), i.e. delete and ignore the channel */
				/* max is 255. it is set to 255 every time we get a match with new measured distance */
				/* it is decreased from extern when calling the fade_out() method */
				/* this method is to allow a kind of "timeout" to all channels */
				/* the fade_out is not need to be called */
				/* fading == 0 <=> Channel is invalid, otherwise Channel is valid */
			uint8_t fading;
				
				/* base to order the channels by. to manage switching channels when overlapping */
				/* badness will be increased at mismatch and decreased at match by several functions */
			uint8_t badness;
			
				/* latest distance from input which matched to this and no better channel */
				/* it will always be updated on a match and never else */
			Metric current_distance;
			
				/* zone, at which the channel "started" in order to recognize an event,
				   always the zone which was the last one entered by current_distances,
				   never == NO_ZONE, except the Channel was new created beginning in no zone
				   and no zone was reached yet,
				   notice: if you reach another zone, you output the event old_z -> new_z
				   and you must set latest_zone to the current zone at this moment */
			uint8_t latest_zone;
				
				/* returns true if and only if Channel is valid */
			inline bool is_valid(){ return fading != 0; }

				/* returns true if and only if Channel is invalid */
			inline bool is_invalid(){ return fading == 0; }
				
				/* resets the fading value to its maximum.
					Should be called if any matching distance was recognized */
			inline void reset_fade(){ fading = 255; }
			
				/* make Channel better
					must be called on a channel match */
			inline void better(Configuration* config){
				badness /= 2;
			}
				/*	This is the inverse function of worse()
					We decided to generally worse() all channels initially.
					This is to call in the case where worse() was called but shouldn't be */
			inline void unworse(){	badness = badness + (badness==0) - 1;	}

			
		public:
				/* standard ctor: create invalid channel, i.e. with fading == 0 */
			inline Channel() : fading(0)/*, latest_zone(NO_ZONE), badness(255), current_distance(0)*/ {}
				
				/* raw constructor to create a valid channel */
			inline Channel(uint8_t zone, const Metric& start_distance, uint8_t initial_badness) :
				fading(255), current_distance(start_distance), latest_zone(zone), badness(initial_badness) {}
			
				/* create new valid Channel with explicit given start zone */
			inline Channel(uint8_t zone, const Metric& start_distance, Configuration* config) :
				Channel(zone,start_distance,config->initial_badness) {}
			
				/* create new valid Channel where start_zone is calculated by start_distance and config */
			inline Channel(const Metric& start_distance, Configuration* config) :
				Channel(get_zone(start_distance,config),start_distance,config->initial_badness) {}
			
				/* supposed to call by a timer,
					if user wants old channels to be deleted after certain time */
				/*	it decreases an the fading value. if it is zero, the channel will be deleted,
					it is automatically set to 255 whenever the channel got a match */
			inline void fade_out(){
				if (is_valid()) {
					--fading;
					// if (fading == 0) this->~Channel(); not necessary within the current implementation
					}
			}
			
				/* make the Channel worse */
			inline void worse(Configuration* config){
				badness = (badness - (badness == 255)) + 1;
				if (badness > config->max_badness) badness = config->max_badness;
			}			
			
				/*	checks if given distance matches to Channel
					if matching and channel valid:
						apply the value, write Event (zone, zone) / (old, new), return true
					if not matching or Channel invalid return false;
						In this case Event won't be touched anyway.
				*/
			bool accumulate(const Metric& new_distance, Configuration* config, Event& event){
				if (is_invalid()) return false;
				if ((current_distance - config->epsilon <= new_distance) && (new_distance <= current_distance + config->epsilon)){
					current_distance = new_distance;
					event[0] = latest_zone;
					event[1] = get_zone(new_distance,config);
					if (event[1] != NO_ZONE) latest_zone = event[1];
					reset_fade();
					unworse();
					better();
					return true;
				}
				return false;
			}
			
				/* compare two channels by comparing their badness */
			bool operator < (const Channel& op) { return this->badness < op.badness; }

				/* destroy a channel. i.e. mark it as invalid */
			inline ~Channel(){
				//latest_zone = NO_ZONE; 
				// current_distance = 0;
				fading = 0; // obvious property for being invalid
				// badness = 255;
			}
				
		};
		
		/* private data */
		
			/* the CMRs channel array */
			/* channels have to be sorted from lowest badness to highest badness.
				i.e. for all i,j : (i <= j) implies (ch[i].badness <= ch[j].badness) */
		Channel ch[channels];
		
		/* private methods */
		
			/* returns the zone corresponding to given distance.
				zone table will be read from config.
				if no zone matches, NO_ZONE will be returned */
		static uint8_t get_zone(const Metric& distance, Configuration* config){
			Zone* zptr = config->zones;
			for (uint8_t zone = 0; zone < config->count_zones; ++zone){
				if ((zptr->lower_border <= distance) && (distance <= zptr->upper_border)){
					return zone;
				}
			}
			return NO_ZONE;
		}
		
			/* returns the zone of given distance, therefor see version of get_zone with 2 params */
		inline uint8_t get_zone(const Metric& distance) { return get_zone(distance,config); }
		
			/* sorts the channel array (ch) when one array element got better
				takes the channel that has to be toggeled to the front until order is correct */
		void smart_sort(uint8_t channel){
			while (1){
				if (channel == 0) return; // we are at the front
				if (!(ch[channel] < ch[channel-1])) return; // we don't need to reorder
				// we need to sort:
				//swap channels:
				Channel swap;
				swap = ch[channel-1];
				ch[channel-1] = ch[channel];
				ch[channel] = swap;
				--channel;
			}
		}
		
	public:
		/* public types */
		
		struct Zone {
		public:
			Metric lower_border;
			Metric upper_border;
			};
			
		class Configuration {
		public:	
			Metric epsilon;
			
			// notice: overlapping zones may be possible but always the first zone in array will be chosen:
			Zone* zones; // pointer to the first tone of an Zone[count_zones] - array.
			uint8_t count_zones; // amount of zones in array
			
			uint8_t max_badness;
			uint8_t initial_badness;
		};
		
		/* public data */
		
			/* pointer to the configuration */
		Configuration* config;
		
		/* public methods */
		
			/* interprets a given measured distance and returns the event which was recognized
			   that is: returning (NO_ZONE, zone) if new non-matching distance in "zone" was recognized 
						returning (NO_ZONE, NO_ZONE) if new non-matching distance was recognized but matches no zone
						returning (old_zone, new_zone) if matching to a channel, old_zone is the last zone at which the channel was
							(old_zone also may be NO_ZONE iff channel never reached any zones since created)
									new_zone is ...		old_zone	if still at same zone
														NO_ZONE		if channel is currently "between" or "away from" zones
														the new zone into that the channel entered
															-> in this case old_zone@next_time will be new_zone@now */
		void put(const Metric& distance, uint8_t& from, uint8_t& to){
			Event event;
			for (uint8_t i = 0; i<channels; ++i){
				ch[i].worse(config);
			}
			for (uint8_t i = 0; i<channels; ++i){
				if (ch[i].accumulate(distance, config, event)){
					// one Channel matched:
					from = event[0];
					to = event[1];
					smart_sort(i);
					return;
				}
			}
			// no Channel matched:
			// ch[channels-1].~Channel(); // destroy worst channel // optional because we override it
			ch[channels-1] = Channel(distance,config); // replace worst channel
			from = NO_ZONE;
			to = ch[channels-1].latest_zone;
			smart_sort(channels-1);
		}
	};
	
}

/************************************************************************/
/* Function Implementation                                              */
/************************************************************************/

template<class Metric, uint8_t channels>
inline sensor::CMI<Metric,channels>::Channel::Channel(const Metric& initial_value, const Configuration* config){
	average = initial_value;
	delta = initial_value / 20; // wegkapseln... mit config 5% ist vielleicht nicht immer was man will oder braucht
	badness = config->initial_badness;
}

template<class Metric, uint8_t channels>
inline bool sensor::CMI<Metric,channels>::Channel::accumulate(const Metric& value, Configuration* config){
	if ((value < average - delta)  || (value > average + delta)){
		inc_badness();
		return false;
	}
	#if false
	// kann man in nested classes auf non-static member der nesting class zugreifen? 				// und was bitteschön soll dann da passieren?
	//nein kann man nicht. Die nested class enthält keine referenz auf outer class (nesting class).
	// man könnte ihr höchstens eine geben
	// komischerweise akzeptiert dieser avrgcc das erstmal beim Compilieren. Getestet hab ich das Verhalten, welches produziert wird aber nicht.
	// MSVC und (g++ (in C++11 und 14)) akzeptieren es nicht. Was der Standard schreibt, müsste ich  noch lesen.
	// in sämtlichen Foren simnd die Leute sich zum Teil uneinig und wissen es auch nicht richtig.
	#endif
	average = (average * config->weight_old + value * config->weight_new) / config->weight_sum();
	dec_badness(config);
	// maybe update accumulated delta
	return true;
}

template<class Metric, uint8_t channels>
inline void sensor::CMI<Metric,channels>::swap_with_previous(uint8_t channel){
	Channel swap { ch[channel] };
	ch[channel] = ch[channel-1];
	ch[channel-1] = swap;
	/* some byte-wise mem swap would be better of course */
}

template<class Metric, uint8_t channels>
inline void sensor::CMI<Metric,channels>::smart_sort(uint8_t channel){
	again_smart_sort:
	if (channel == 0) return;
	if (ch[channel] < ch[channel-1]){
		swap_with_previous(channel);
		--channel;
		goto again_smart_sort;
	}
}

template<class Metric, uint8_t channels>
void sensor::CMI<Metric,channels>::input(const Metric& value){
	uint8_t i;
	for (i = 0; i<channels; ++i){ // go through all channels and try to accumulate new value
		if (ch[i].accumulate(value,configuration)) {
			smart_sort(i); // i was updated and got 'better' so it might need to be pushed to the left.
			++i; // skip the current (matching) channel
			while(i<channels){ // make all residual channels worse too
				ch[i].inc_badness();
				++i;
			}
			return;
		}
	}
	// no_match: // applies if the value did not match any of the channels
	//ch[channels-1].~Channel(); // empty d-tor
	///new (&ch[channels-1]) Channel(value, configuration); // replace the worst channel in array by a new channel
	ch[channels-1] = Channel(value, configuration); // replace the worst channel in array by a new channel
	smart_sort(channels-1); // maybe the new channel is not the worst
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