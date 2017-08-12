/*
 * f_cmi.h
 *
 * Created: 05.08.2017 18:30:03
 *  Author: Maximilian Starke
 
 *		FPS:
 
		history:
		
	issue 
		all over it is ready for use with positive values in a metric.
		I suppose (but never tried) there is a fatal bug when using negative values.
		This bug can be fixed when putting some get_delta function pointer into Config
		so the user prgmmer has to decide how to choose the allowed delta deviation his- or herself.
		At the moment it is 20% of the initial measured value
			... which is seriously not good if starting by a few cm and ending in 2m e.g. using hc-sr04.
		Change this!!!!!#### there is a ##### mark and some <<<< marks at the regarding lines...
		
	solution
		i added a virtual function to config which provides a delta from average function.
		the delta property of channel was removed .... makes by the way channels smaller
		
		despite this.. everything was checked again once after production. and everthing (else) is okay.
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
	class CMI {
		static_assert(channels <= 255,"too many channels!");
	public:
		class Configuration;
		
	private:
	/* private types */
	
		class Channel {
		public:
			Metric average;		// average of accumulated values
			uint8_t badness;	// channel invalid <=> badness == 255, channel would be best if badness = 0
			
				/* create an invalid Channel */
			Channel() : average(0), badness(255) {}
				
				/* create a valid Channel */
			Channel(const Metric& initial_value, const Configuration* config);
			
				/* make the Channel 'worse' */
			inline void inc_badness(){ if (badness < 254) ++badness; }
			
				/* make the Channel 'better' */
			inline void dec_badness(const Configuration* config){
				badness = (static_cast<uint16_t>(badness) * static_cast<uint16_t>(config->badness_reducer))
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
			inline bool operator < (const Channel& op) const { return badness < op.badness; }
			
				/* make channel invalid */	
			inline void invalidate(){ badness = 255; }
			
				/* destroy a channel, actually the same as invalidate() */
			inline ~Channel() {	badness = 255;	}
		};
		
	/* private data */
	
			/*	channels of the interpreter should be sorted ascending by the badness
				best channel is at index 0 */
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
		using LinPercDeltaConfig = LinearPercentageDeltaConfiguration<Metric>;
		using LPDConfig = LinearPercentageDeltaConfiguration<Metric>;
		using LPDC = LinearPercentageDeltaConfiguration<Metric>;
		
		class AffineDeltaConfiguration {
			uint16_t _factor;
			uint16_t _divisor;
			Metric _abs_delta;
			//### make c-tor...
		};
		
	/* public data */
	
			/* pointer to the configuration record */
		Configuration* configuration;
	
	/* public methods */
	
			/* enter a new measured value */
		void input(const Metric& value);
		
			/* return current measurement result */
			/* never interrupt output by input or vice versa */
			/* attention: you'll get a reference to the value stored inside the CMI
			   if you update the value indirectly via an input(..) the value behind
			   your reference will be updated too and will be the new correct output. */
		inline const Metric& output(){ return ch[0].average; }
			
			/* c-tor */
		CMI(Configuration* configuration) : configuration(configuration) {}
		// also time to check about copy and move c-to and = op ######
	};

	/* defining synonyms */ // <<<<< I don't know whether this is good or not
	
	template<typename Metric, uint8_t channels>
	using ChannellingMeasurementInterpreter = CMI<Metric,channels>;


/************************************************************************/
/* Function Implementation                                              */
/************************************************************************/

	template<class Metric, uint8_t channels>
	inline CMI<Metric,channels>::Channel::Channel(const Metric& initial_value, const Configuration* config){
		average = initial_value;
		badness = config->initial_badness;
		if (badness == 255) --badness; // just to avoid that someone tries to create a invalid channel.
	}
	
	template<class Metric, uint8_t channels>
	inline bool CMI<Metric,channels>::Channel::accumulate(const Metric& value, Configuration* config){
		if (badness == 255) return false; // channel invalid
		if ((value < average - config->delta(average))  || (value > average + config->delta(average))){
				// no match:
			inc_badness();
			return false;
		}
		// match:
		average = (average * config->weight_old + value * config->weight_new) / config->weight_sum();
		dec_badness(config);
		return true;
	}
	
	template<class Metric, uint8_t channels>
	inline void CMI<Metric,channels>::swap_with_previous(uint8_t channel){
		Channel swap { ch[channel] };
		ch[channel] = ch[channel-1];
		ch[channel-1] = swap;
		/* some byte-wise mem swap would be better of course */
		/* but I think this is okay because it is only function scope stack memory. */
		/* but there is no function like memcpy doing this in the standard library */
	}
	
	template<class Metric, uint8_t channels>
	inline void CMI<Metric,channels>::smart_sort(uint8_t channel){
		again_smart_sort:
		if (channel == 0) return;
		if (ch[channel] < ch[channel-1]){
			swap_with_previous(channel);
			--channel;
			goto again_smart_sort;
		}
	}
	
	template<class Metric, uint8_t channels> /// <<<<<<< return instead the (old) index od channel, which matched. and also throw some _NO_Channel
	void CMI<Metric,channels>::input(const Metric& value){
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
	
}

#endif /* F_CMI_H_ */