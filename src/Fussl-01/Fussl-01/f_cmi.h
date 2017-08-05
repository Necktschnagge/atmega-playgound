/*
 * f_cmi.h
 *
 * Created: 05.08.2017 18:30:03
 *  Author: Maximilian Starke
 
 *		FPS: all over it is ready for use with positive values in a metric.
		I suppose (but never tried) there is a fatal bug when using negative values.
		This bug can be fixed when putting some get_delta function pointer into Config
		so the user prgmmer has to decide how to choose the allowed delta deviation his- or herself.
		At the moment it is 20% of the initial measured value
			... which is seriously not good if starting by a few cm and ending in 2m e.g. using hc-sr04.
		Change this!!!!!#### there is a ##### mark and some <<<< marks at the regarding lines...
		
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
			Metric delta;		// positive max deviation a value may have from average to be applied
			uint8_t badness;	// channel invalid <=> badness == 255, channel would be best if badness = 0
			
				/* create an invalid Channel */
			Channel() : average(0), delta(0), badness(255) {}
				
				/* create a valid Channel */
			Channel(const Metric& initial_value, const Configuration* config);
			
				/* make the Channel 'worse' */
			inline void inc_badness(){ if (badness < 254) ++badness; }
			
				/* make the Channel 'better' */
			inline void dec_badness(const Configuration* config){
				badness = (static_cast<uint16_t>(badness) * static_cast<uint16_t>(config->badness_reducer))
					/ (static_cast<uint16_t>(config->badness_reducer) + 3); // <<<<< maybe change the 3 to a bigger number ?
			}
				
				/* try to apply value
				   on match:	applies value, makes channel better, returns true
				   on mismatch:	makes channel worse, returns false
				   invalid channel means always mismatch */
			inline bool accumulate(const Metric& value, Configuration* config);
			
				/* comparison by badness */
			inline bool operator < (const Channel& op) const { return badness < op.badness; }
			
				/* destroy a channel */
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
			
			// maybe we want do accumulate the delta and specify an initial delta later here
			// ?type delta_accumulator;
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
		delta = initial_value / 20; // for now we make this hard coded as 5%; // <<<<< maybe change this decision, negativ initial values cause issues ######
		badness = config->initial_badness;
		if (badness == 255) --badness; // just to avoid that someone tries to create a invalid channel.
	}
	
	template<class Metric, uint8_t channels>
	inline bool CMI<Metric,channels>::Channel::accumulate(const Metric& value, Configuration* config){
		if (badness == 255) return false; // channel invalid
		if ((value < average - delta)  || (value > average + delta)){
			inc_badness();
			return false;
		}
		average = (average * config->weight_old + value * config->weight_new) / config->weight_sum();
		dec_badness(config);
		// maybe update accumulated delta <<<<<<<
		return true;
	}
	
	template<class Metric, uint8_t channels>
	inline void CMI<Metric,channels>::swap_with_previous(uint8_t channel){
		Channel swap { ch[channel] };
		ch[channel] = ch[channel-1];
		ch[channel-1] = swap;
		/* some byte-wise mem swap would be better of course */
		/* but I think this is okay because it is only function scope stack memory. */
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
	
	template<class Metric, uint8_t channels>
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