/*
 * f_cmi.h
 *
 * Created: 05.08.2017 18:30:03
 *  Author: Maximilian Starke
 
 *		FPS:
		everything was checked a few times and is supposed to be stable.
		
		improve descriptions, especially at the configuration stuff which has to be implmented by the uprogrammer
		
		some <<< for optimisation or additional stuff is present and can be done later.
 */ 


#ifndef F_CMI_H_
#define F_CMI_H_

#include <stdint.h>

namespace analyzer {

/************************************************************************/
/* Function and Type Declaration                                        */
/************************************************************************/
	
		/* Channeling Measurement Interpreter */
	template <typename Metric, uint8_t channels>
	class ChannellingMeasurementInterpreter {
		static_assert(channels <= 255,"too many channels!");
	public:
		class Configuration;
		static constexpr uint8_t NO_CHANNEL { 255 };
		
	private:
	/* private types */
	
		class Channel {
			/// <<< I don't know very well about compiler optimization,
			// but when putting class channel's methods directly into CMI,
			// we would not need to pass config pointers
			// -> better in storage usage on stack and also execution time (*config derefencing)
		private:
			Metric _average;		// average of accumulated values
			uint8_t _badness;	// channel invalid <=> badness == 255, channel would be best if badness = 0

		public:	
				/* create an invalid Channel */
			Channel() : _average(0), _badness(255) {}
				
				/* create a valid Channel */
			Channel(const Metric& initial_value, const Configuration* config);
			
				/* returns reference to best average */
			inline const Metric& get_average() { return _average; }
				
				/* make the Channel 'worse' */
			inline void inc_badness(){ if (_badness < 254) ++_badness; }
			
				/* make the Channel 'better' */
			inline void dec_badness(const Configuration* config){
				_badness = (static_cast<uint16_t>(_badness) * static_cast<uint16_t>(config->badness_reducer))
					/ (static_cast<uint16_t>(config->badness_reducer) + 3); // <<<<< maybe change the 3 to a bigger number ?
						// or make this 3 template ??? ... a better solution could be to make the mapping reducer -> actual factor better
						// maybe change this mapping generally
			}
				
				/* try to apply value
				   on match:	applies value, makes channel better, returns true
				   on mismatch:	makes channel worse, returns false
				   invalid channel means always mismatch */
			inline bool accumulate(const Metric& value, Configuration* config);
			
				/* comparison by badness */
			inline bool operator < (const Channel& op) const { return _badness < op._badness; }
			
				/* make channel invalid */	
			inline void invalidate(){ _badness = 255; }
			
				/* destroy a channel, actually the same as invalidate() */
			inline ~Channel() {	invalidate(); }
		};
		
	/* private data */
	
			/*	channels of the interpreter should be sorted ascending by the badness
				best channel is at index 0 */
		Channel _ch[channels];
		
	/* private methods */
	
			/* swap a channel array entry with its predecessor */
		inline void swap_with_previous(uint8_t channel);
		
			/* only look for the given channel array position
				whether this channel is better than predecessors 
					if so than swap along to build the correct order */
		inline void smart_sort(uint8_t channel);
		
	public:
	/* public types */
	
			/* abstract class for configuration objects */
		class Configuration {
		public:
				/* You should choose the weights and the type 'Metric' such way
				   that neither (weight_old * average) nor (weight_new * average)
				   nor their sum will cause an overflow within type Metric.
				   Otherwise the behavior is undefined. */

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
			
				/* tells you the allowed delta to the average for which new values are accepted and accumulated */
				/* this function must be overridden by an inheriting class */
				/* if you prefer const, and from value independent delta, you can use predefined ConstDeltaConfiguration */
			virtual const Metric& delta(const Metric& value) = 0;
		};
			
			/* Configuration class for const delta */
		class ConstDeltaConfiguration : public Configuration {
		private:
			const Metric _delta;
		public:
			ConstDeltaConfiguration(const Metric& const_delta) : _delta(const_delta) {}
			const Metric& delta(const Metric&) override { return _delta; }
		};
		
		using ConstDeltaConfig = ConstDeltaConfiguration;
		using CDConfig = ConstDeltaConfiguration;
		using CDC = ConstDeltaConfiguration;
		
		template<typename multiplicator>
		class LinearPercentageDeltaConfiguration : public Configuration {
		private:
			multiplicator _mult;
		public:
				/* you have to ensure by your self that value multiplied with percentage won't overflow! */
			LinearPercentageDeltaConfiguration(multiplicator percentage) : _mult(percentage) {}
			const Metric& delta(const Metric& value) override {
				return ((value<0 ? -value : value) * _mult) / 100;  
			}
		};
		template<typename multiplicator>
		using LinPercDeltaConfig = LinearPercentageDeltaConfiguration<multiplicator>;
		template<typename multiplicator>
		using LPDConfig = LinearPercentageDeltaConfiguration<multiplicator>;
		template<typename multiplicator>
		using LPDC = LinearPercentageDeltaConfiguration<multiplicator>;
		
		/*class AffineDeltaConfiguration {
			uint16_t _factor;
			uint16_t _divisor;
			Metric _abs_delta;
			//make c-tor...
		};*/ ///<<<<<< for a later version
		
	/* public data */
	
			/* pointer to the configuration record */
		Configuration* configuration;
	
	/* public methods */
	
			/* enter a new measured value */
			/* returns the Channel {0, ... ,channels-1} which matched,
			   returns NO_CHANNEL iff no channel matched and creates a new channel */
		uint8_t input(const Metric& value);
		
			/* return current measurement result */
			/* never interrupt output by input or vice versa */
			/* attention: you'll get a reference to the value stored inside the CMI
			   if you update the value indirectly via an input(..) the value behind
			   your reference will be updated too and will be the new correct output. */
		inline const Metric& output(){ return _ch[0].get_average(); }
			
			/* create a CMI with all channels invalid
			   attention: you have to provide a Configuration object */
		ChannellingMeasurementInterpreter(Configuration& configuration) : configuration(&configuration) {}
			
		ChannellingMeasurementInterpreter(const ChannellingMeasurementInterpreter&) = delete;
		ChannellingMeasurementInterpreter(ChannellingMeasurementInterpreter&&) = delete;
		ChannellingMeasurementInterpreter& operator = (const ChannellingMeasurementInterpreter&) = delete;
		ChannellingMeasurementInterpreter& operator = (ChannellingMeasurementInterpreter&&) = delete;
		
		~ChannellingMeasurementInterpreter(){
			for (uint8_t i = 0; i < channels; ++i) _ch[i].invalidate();
		}
		
	};

	/* defining synonyms */
	
	template<typename Metric, uint8_t channels>
	using CMI = ChannellingMeasurementInterpreter<Metric,channels>;


/************************************************************************/
/* Function Implementation                                              */
/************************************************************************/

	template<class Metric, uint8_t channels>
	inline ChannellingMeasurementInterpreter<Metric,channels>::Channel::Channel(const Metric& initial_value, const Configuration* config){
		_average = initial_value;
		_badness = config->initial_badness;
		if (_badness == 255) --_badness; // just to avoid that someone tries to create a invalid channel.
	}
	
	template<class Metric, uint8_t channels>
	inline bool ChannellingMeasurementInterpreter<Metric,channels>::Channel::accumulate(const Metric& value, Configuration* config){
		if (_badness == 255) return false; // channel invalid
		if ((value < _average - config->delta(_average))  || (value > _average + config->delta(_average))){
				// no match:
			inc_badness();
			return false;
		}
		// match:
		_average = (_average * config->weight_old + value * config->weight_new) / config->weight_sum();
		dec_badness(config);
		return true;
	}
	
	template<class Metric, uint8_t channels>
	inline void ChannellingMeasurementInterpreter<Metric,channels>::swap_with_previous(uint8_t channel){
		Channel swap { _ch[channel] };
		_ch[channel] = _ch[channel-1];
		_ch[channel-1] = swap;
		/* some byte-wise mem swap would be better of course */
		/* but I think this is okay because it is only function scope stack memory. */
		/* but there is no function like memcpy doing this in the standard library */
	}
	
	template<class Metric, uint8_t channels>
	inline void ChannellingMeasurementInterpreter<Metric,channels>::smart_sort(uint8_t channel){
		again_smart_sort:
		if (channel == 0) return;
		if (_ch[channel] < _ch[channel-1]){
			swap_with_previous(channel);
			--channel;
			goto again_smart_sort;
		}
	}
	
	template<class Metric, uint8_t channels>
	uint8_t ChannellingMeasurementInterpreter<Metric,channels>::input(const Metric& value){
		uint8_t i;
		for (i = 0; i<channels; ++i){ // go through all channels and try to accumulate new value
			if (_ch[i].accumulate(value,configuration)) {
				uint8_t matched_channel {i};
				smart_sort(i); // i was updated and got 'better' so it might need to be pushed to the left.
				++i; // skip the current (matching) channel
				while(i<channels){ // make all residual channels worse too
					_ch[i].inc_badness();
					++i;
				}
				return matched_channel;
			}
		}
		// no_match: // applies if the value did not match any of the channels
		//ch[channels-1].~Channel(); // empty d-tor
		///new (&ch[channels-1]) Channel(value, configuration); // replace the worst channel in array by a new channel
		_ch[channels-1] = Channel(value, configuration); // replace the worst channel in array by a new channel
		smart_sort(channels-1); // maybe the new channel is not the worst
		return NO_CHANNEL;
	}
	
}

#endif /* F_CMI_H_ */