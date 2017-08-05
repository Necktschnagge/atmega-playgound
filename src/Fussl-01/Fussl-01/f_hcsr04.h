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
#include "f_cmi.h"

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
	
	// only for namespace backward compatibility:	
	template<typename Metric, uint8_t channels>
	using Kanalysator = analyzer::CMI<Metric,channels>;

	
	
	
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