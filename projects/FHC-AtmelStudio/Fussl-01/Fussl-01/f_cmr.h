/*
 * f_cmr.h
 *
 * Created: 05.08.2017 20:11:54
 *  Author: Maximilian Starke
 
 *		FPS:	one check after getting ready is done.
				second check which I forced done.
				CMR is supposed to be stable. but until now there are no tests aviable.

				some <<< for later improvements. not important. no semantic changes.
				
				// <<< check sizeof( class ...::Event )! it should be 2 at max. otherwise change design!!!
				// <<< check behaviour of put() once again against the description.
				// <<< maybe come better explaining comments at config.
 */ 


#ifndef F_CMR_H_
#define F_CMR_H_

#include <stdint.h>

namespace analyzer {

/************************************************************************/
/* Type and Function Declaration                                        */
/************************************************************************/

		/* channeling movement recognizer */
		/* A class you feed with measured and cleaned (like with f_cmi) values
		   and predefined value zones. It tells you if the input list of values
		   moved from one zone to another zone.
		   Therefore it gives you zone numbers back. */	
	template<typename Metric, uint8_t channels>
	class ChannellingMovementRecognizer {
	public:
		/* Forward declarations */
		static constexpr uint8_t NO_ZONE {255};
		class Configuration;
		class Event;
			static_assert(sizeof(Event) == 2,"CMR::Event has wrong size");
	private:
		/* private types */
		class Channel {
		private:
				/* fading == 0 <=> Channel is invalid, otherwise Channel is valid */
				/* 0 is faded out, that means we invalidate() the channel if we get there */
				/* max is 255. it has to be set to 255 every time we get a match with new measured value */
				/* it may be decreased from extern when calling the fade_out() method */
				/* this method is to allow a kind of "timeout" to all channels */
				/* the fade_out() is not need to be called */
			uint8_t fading;
				
				/* badness will be increased at mismatch and decreased at match by several functions */
				/* between valid channels it says how bad the channel represents the recent inputs */
				/* the smaller badness the better the channel approaches the reality */
			uint8_t badness;
			
				/* latest distance from input which matched to this and no better channel */
				/* it has to be updated on a match and never else */
			Metric current_value;
			
				/* zone, at which the channel "started" in order to recognize an event,
				   always the zone which was the last one entered by current_value,
				   never == NO_ZONE, except the Channel was new created beginning in no zone
				   and there wasn't reached any zone yet */
			uint8_t latest_zone;
				
				/* returns true if and only if Channel is valid */
			inline bool is_valid(){ return fading != 0; }

				/* returns true if and only if Channel is invalid */
			inline bool is_invalid(){ return !is_valid(); }
				
				/* resets the fading value to its maximum.
					Should be called if any matching value was recognized */
			inline void reset_fade(){ fading = 255; }
			
				/* make Channel better, must be called on a channel match */
			inline void better(){	badness /= 2;	}
			
				/*	This is the undo function for worse()
					I decided to generally worse() all channels initially.
					This is to call in the case where worse() was called but shouldn't be */
			inline void unworse(){	badness = badness + (badness==0) - 1;	}
			
		public:
				/* standard ctor: create invalid channel */
			inline Channel() : fading(0) /*, latest_zone(NO_ZONE), badness(255), current_value(0)*/ {}
				
				/* raw constructor to create a valid channel */
			inline Channel(uint8_t start_zone, const Metric& start_value, uint8_t initial_badness) :
				fading(255), badness(initial_badness), current_value(start_value), latest_zone(start_zone) {}
			
				/* create new valid Channel with explicit given start zone */
			inline Channel(uint8_t start_zone, const Metric& start_value, Configuration* config) :
				Channel(start_zone,start_value,config->initial_badness) {}
			
				/* create new valid Channel where everything is calculated by sart_Value and config
					(start_zone is calculated by start_value and config) */
			inline Channel(const Metric& start_value, Configuration* config) :
				Channel(get_zone(start_value,config),start_value,config->initial_badness) {}
			
				/* get the latest_zone of this channel */
			inline uint8_t get_latest_zone(){ return latest_zone; }

				/* make the Channel worse */
			inline void worse(Configuration* config);
			
				/* compare two channels by comparing their badness and whether they are valid,
				   a channel is smaller (i.e. better) than another,
				   if it is ( less bad and both are valid ) or strict more valid */
			bool operator < (const Channel& op);
			
			/* a channel is strict greater than another if it is (strict more invalid) or (more bad when both are valid) */
			inline bool operator > (const Channel& op){	return op < *this;	}

				/*	checks whether given value matches to this Channel
					if matching and channel valid:
						unworse() and better() this channel
						apply the value, write Event (zone, zone) / (old, new), return true
					if not matching or Channel invalid return false;
						In this case event won't be touched anyway. */
			bool accumulate(const Metric& new_value, Configuration* config, Event& event);
			
				/*	it decreases fading by amount. It will be set to zero if it underflows
					if it becomes zero, the channel will automatically become invalid.
					returns true if and only if the channel was faded out,
					i.e. became 0 but wasn't before, just to mark you may need to sort array */
			inline bool fade_out(uint8_t amount = 1);
			
				/* just make the channel invalid */
			inline void invalidate(){	fading = 0;	}
			
				/* destroy a channel. i.e. mark it as invalid */
			inline ~Channel(){	invalidate();	}
		};
		
		/* private data */
		
			/*	array of all channels.
				valid channels have to be sorted from lowest badness to highest badness.
				i.e. for all i,j : (i <= j) implies (ch[i].badness <= ch[j].badness)
				valid channels first, invalid ones afterwards. */
		Channel ch[channels];
		
		/* private methods */
		
			/*	returns the zone corresponding to given value.
				zone table will be read from config.
				if no zone matches, NO_ZONE will be returned */
		static uint8_t get_zone(const Metric& distance, Configuration* config);
		// <<<< this is a kind of extern defined method of class Configuration.
		// I think this is the better way like done here,
		// because no user is wondering about creepy methods in his config object.
		// so ... well, see cmi because there is a weight_sum function in config.
		// maybe there could be done the same as a static method of cmi.
				
			/* sorts the channel array (ch) when one array element got better or worse
				takes the channel that has to be toggled to the front / back until order is correct */
		void smart_sort(uint8_t channel);
		
	public:
		/* public types */
		
			/* represents the transition from one zone to another */
		class Event {
			uint8_t array[2]; // 0..from_zone, 1..to_zone
		public:
			uint8_t& from {array[0]}; // reference to from-zone
			uint8_t& to   {array[1]}; // reference to to-zone
			inline Event() : array{NO_ZONE,NO_ZONE} {}
			inline Event(uint8_t from, uint8_t to) : array{from, to} {}
			inline uint8_t& operator[](uint8_t index){	return array[index];	}
		};
		
			/* represents a zone via its borders */
		struct Zone {
		public:
				/*	lower_border must be <= upper_border
					to have (at least one) value[s] inside this zone */
			Metric lower_border;
			Metric upper_border;
			};
			
			/* configuration class containing zones, epsilon, ... */
		class Configuration {
		public:
				/*	the maximum distance a new value may have
					to the old value to allow accumulating to its channel
					must be positive ! */
			Metric epsilon;
			
			// notice: overlapping zones may be possible but always the first zone in array will be chosen:
				/*	pointer to the first zone of an Zone[count_zones] - array.
					you have to build this array and provide its memory on your own. */
			Zone* pZones;
			
				/*	number of zones in the zone array.
					you can set it to any value {0..255}.
					if 0 then pZones may be anything since it won't be touched.
						this case put() can't produce any output with information */
			uint8_t rgZones;

				/* maximum badness a channel can reach */			
			uint8_t max_badness;
			
				/* initial badness which new channels will get */
			uint8_t initial_badness;
		};
		
		/* public data */
		
			/* pointer to configuration object */
		Configuration* config;
		
		/* public methods */
		
			/* interprets a given measured value and returns the event which was recognized
			   that is: returning (NO_ZONE, zone) if new non-matching value in "zone" was recognized 
						returning (NO_ZONE, NO_ZONE) if new non-matching value in no zone was recognized
						returning (old_zone, new_zone) if matching to a channel, old_zone is the last zone at which the channel was
							(old_zone also may be NO_ZONE iff channel never reached any zones since created)
									new_zone is ...		old_zone	if still at same zone
														NO_ZONE		if channel is currently "between" or "away from" zones
														the new zone into that the channel entered
															-> in this case old_zone@next_time will be new_zone@now */
		void put(const Metric& value, uint8_t& from, uint8_t& to);
		
			/* fade_out allows the programmer a kind of timeout for channels
			   if they don't receive any matching values for some time */
			/* every channel has a fading, a value from 0 to 255.
			   every time a channel match occurs fading is reset to 255,
			   you can decrease fading with this function by amount
			   if it was decreased to 0 the channel will be made invalid */
		void fade_out(uint8_t amount = 1);
		
		ChannellingMovementRecognizer(Configuration& config){
			this->config = &config;
		}
	}; // CMR
	
	/* definition of shorter synonyms */
	
	template<typename Metric, uint8_t channels>
	using CMR = ChannellingMovementRecognizer<Metric,channels>;



/************************************************************************/
/* Function Implementation                                              */
/************************************************************************/

	template<typename Metric, uint8_t channels>
	inline void ChannellingMovementRecognizer<Metric,channels>::Channel::worse(Configuration* config){
		badness = (badness - (badness == 255)) + 1;
		if (badness > config->max_badness) badness = config->max_badness;
	}

	template<typename Metric, uint8_t channels>
	bool ChannellingMovementRecognizer<Metric,channels>::Channel::operator < (const Channel& op){
		return (this->is_valid()) && ( (op.is_invalid()) || (this->badness < op.badness) );
		
		/*						badness
			this	op			<		result	i.e.
			valid | valid  | true		true	<
			valid | valid  | false		false	>=
			valid | invalid| *			true	<
			invalid| *	   | *			false	>=
		*/
	}

	template<typename Metric, uint8_t channels>
	bool ChannellingMovementRecognizer<Metric,channels>::Channel::accumulate(const Metric& new_value, Configuration* config, Event& event){
		if (	(is_valid())
					&&
				(current_value - config->epsilon <= new_value) && (new_value <= current_value + config->epsilon)
					){
			current_value = new_value;
			event[0] = latest_zone;
			event[1] = get_zone(new_value,config);
			if (event[1] != NO_ZONE) latest_zone = event[1];
			reset_fade();
			unworse();
			better(config);
			return true;
		}
		return false;
	}

	template<typename Metric, uint8_t channels>
	inline bool ChannellingMovementRecognizer<Metric,channels>::Channel::fade_out(uint8_t amount){
				if (is_valid()) { // we have to fade
					if (amount < fading ){
						// we don't fade out
						fading -= amount;
						} else {
						// we fade out
						fading = 0; // <==> marked as invalid
						// this.~Channel(); // does nothing, is already invalid
						return true;
					}
				}
				// else channel invalid / already faded out
				return false;
			}

	template<typename Metric, uint8_t channels>
	uint8_t ChannellingMovementRecognizer<Metric,channels>::get_zone(const Metric& distance, Configuration* config){
		Zone* zptr = config->zones;
		for (uint8_t zone = 0; zone < config->count_zones; ++zone){
			if ((zptr[zone].lower_border <= distance) && (distance <= zptr[zone].upper_border)){
				return zone;
			}
		}
		return NO_ZONE;
	}
	
	template<typename Metric, uint8_t channels>	
	void ChannellingMovementRecognizer<Metric,channels>::smart_sort(uint8_t channel){
		uint8_t i_channel {channel};
		// shuffle to the front:
		while (1){
			if (channel == 0) goto part2; // we are at the front
			if (!(ch[channel] < ch[channel-1])) goto part2; // we don't need to reorder
			// we need to sort, i.e. push this channel left:
			// swap with previous channel:
			Channel swap;
			swap = ch[channel-1];
			ch[channel-1] = ch[channel];
			ch[channel] = swap;
			--channel;
			// <<< better bytewise swapping!! add such function in hardware.h of fussel fframework
		}
		part2:
		//shuffle to the back:
		while (1){
			if (i_channel == channels-1) return; // reached last channel
			if (!( ch[i_channel] > ch[i_channel+1] )) return; // reordering not necessary
			// this channel is greater than next one: push it to the right:
			Channel swap;
			swap = ch[i_channel+1];
			ch[i_channel+1] = ch[i_channel];
			ch[i_channel] = swap;
			++i_channel;
		}
	}

	template<typename Metric, uint8_t channels>
	void ChannellingMovementRecognizer<Metric,channels>::put(const Metric& value, uint8_t& from, uint8_t& to){
		Event my_event;
		for (uint8_t i = 0; i<channels; ++i){
			ch[i].worse(config);
		}
		for (uint8_t i = 0; i<channels; ++i){
			if (ch[i].accumulate(value, config, my_event)){
				// one Channel matched:
				from = my_event.from;
				to = my_event.to;
				smart_sort(i);
				return;
			}
		}
		// no Channel matched:
		// ch[channels-1].~Channel(); // destroy worst channel // optional because we override it
		ch[channels-1] = Channel(value,config); // replace worst channel
		from = NO_ZONE;
		to = ch[channels-1].get_latest_zone();
		smart_sort(channels-1);
	}
	
	template<typename Metric, uint8_t channels>
	void ChannellingMovementRecognizer<Metric,channels>::fade_out(uint8_t amount){
		// when fading out, the only change with effects to array order is
		// that a channel can become invalid and has to be toggled to the end.
		// When beginning at the end of the array,
		// we don't risk that a Channel will be left out because of sorting / toggelling.
		for (uint8_t c = channels; c !=0;){
			--c;
			if (ch[c].fade_out(amount)) smart_sort(c);
			// when faded out we might need to refresh array order (shuffle to back)
		}
	}

} // namespace analyzer



#endif /* F_CMR_H_ */