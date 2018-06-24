/*
 * f_gui.h
 *
 * Created: 12.09.2016 10:25:26
 * Author: Maximilian Starke
 *
 *	FILE PROGRESS STATE:
 *	namespace input is ready and prg logic was checked again
 
	/// <<<< this may better be converted into a class, and add some PIN references to the class.
	// << template class using an template param for the number of buttons.
	// but a button array may be build in another way,
	// so we might encapsulate to parts of this logical part
	// one part reading out the hardware an calculating some vector containing the button states
	// one part using the vector to treat changes.
	
	use an IOPin range to get references to the input pins.
	
 
 *	
 */ 


#ifndef F_GUI_H_
#define F_GUI_H_

#include "f_arch.h"
#include "f_concepts.h"

using PCallable = concepts::Callable*;


namespace input {
	/* input (for now) is like a static object (for java people) which holds the information
	which buttons are pressed or released and what changed from last look up*/
	/* it also stores callbacks in different forms for each single button event
		that you can use to captivate events for your software */
	
	
	/* Buttons:
	In this version we have 5 hard coded buttons (button0, ... , button4).
	
	suggested meanings of the buttons are:
	button		4		->	global config menu button
				3		->	page select button
				2..0	->	programmable buttons for each feature
	Notice: This is only a suggestion. You don't need to accept this but programming consistently to these
			suggestions, the controller might be more uniform (means easier) to use
	*/
	constexpr bool BUTTON_DOWN {1};	// button pressed - event
	constexpr bool BUTTON_UP {0};	// button released - event
	
	/*		
	Each button event has an unique id (eventID):
	
		eventID = [buttonNumber] * 2 + [button_state]
			where button_state is BUTTON_DOWN if the button >was pressed< down and >is now down<, and BUTTON_UP otherwise
			
		eventID is a value 0 .. 9
		
		we will talk about a "single button event" if we talk about the changing state of one single button
	
	The next values are used for special cases:
	*/	
	constexpr int8_t MULTI_CHANGE {10};					// more than one button changed it's state
	constexpr int8_t NO_CHANGE {-1};					// no button changed it's state
	constexpr int8_t MULTI_EVENT = MULTI_CHANGE;		// more than one single button event occurred
	
	/*
	Event handles:
	You need some callbacks to get your intended code executed when the related event occurred.
	for callback references input supports two kinds of callbacks:
		1.:	The (non object orientated) callback function pointer:
					void (*callbackFunction)();
					You can give input a pointer to your void-terminated function to call on event.
					The functions doesn't get any arguments when called.
					(Hint: You can get additional information by using getEventCode() or gbutton, gchange)
		2.:	The (object orientated) callback objects:
					Callable* callbackObject;
					You can give input a pointer to a object of a class
					derived from Callable (defined in this library)
					The (virtual) method operator() will be called when the event occurs.
					operator() takes no arguments and returns in void such as in case (1)
		
		Notice: In C++ it is not possible to use a simple (C-like) callback function pointer in order to
				point to a member function of some object. If you use class members you should use (2).
				In other cases it might be better to use C-like function pointers to not pollute µC memory.
				If you (only) want to point to static (class) methods there won't be any problem with using (1).
				
	You can always use only one of both kinds of callbacks for each single button event
	but you can change each callback reference including its type of referring anytime you want.
	Different buttons may have different kinds of callbacks at the same time.
	
	In order to this behavior callbacks are nested in a union to save bytes of RAM
	*/
	
	typedef union callback_u* pcallback_t; // ## change old style
	typedef union callback_u {
		void (*callbackProcedure)();		// enabled by 1
		concepts::Callable* callbackObject;			// enabled by 2
		} callback_t;
	
	/*
	Each stored event handler reference comes with an enable flag
	case: an event handler is enabled (enable>0):
		This flag tells whether a callback function or a callback object is used.
		enable == 1 : C-like function pointer
		enable == 2	: Pointer to a Callable
	case: no event handler is enabled (enable <= 0):
		enable == 0 : means there is no valid callback.
		enable == -1: there is a C-like function pointer callback, but disabled
		enable == -2: there is a Callable* callback, but diabled
	
	Notice: In case enable<0 it is possible to just enable the stored callback reference.
			in case enable == 0 this process will fail.
	*/
	
	typedef struct event_s {
		callback_t callbackReference;
		int8_t enable;
		} event_t;
	
	class EventContainer {
		public:
		event_t buttonEvent[5][2];
		// hidden things shadow events or maybe...<<<<<< (is it necessary or senseful????)
		};
	typedef EventContainer* PEventContainer;
	
	extern uint8_t gbutton; // current button state (last captured button state)
	extern uint8_t gchange; // current button changes (difference between gbutton and gbutton @ before)
	extern EventContainer inputEvents; // events to be executed when someone calls exec
	
	/*	activate the input pins of the controller,
		init the event array by deleting all events.
		*/
	void init();
	
	//	returns the event id of the given event description
	inline int8_t makeEvent(uint8_t button, bool up_or_down){
		return 2*button+up_or_down;
	}
	
	/*	Update the stored values about the button configuration
	
		in details:
		read the 5 button states
		update gbutton (containing the button states at reading time)
		update gchange (containing the difference between last button state and current)
		returns true if and only if any button changed <=> gchange!=0
	*/
	bool readInput();
	
	/*	This function executes enabled event handlers that match with the occurred event(s).
		"all" means exec all that occurred. "not all" means exec only one.
		For details, see this:
		
		button order: button 4 > button 3 > .. > button 0; event of higher button comes first.

		if you set all = false (default):
			case "true" is returned:
				The first enabled and occurred event in button order will be treated and only this one.
				
				execute one and only one event handler method:
					constraints: (the executed event handler function ptr is non-nullptr ||
					reference is a non-nullptr Callable*) && (enable must be "true") && (the event occurred)
				OR
				treat an enabled but nullptr event:
					nothing will be executed.
				
				returns true
								
			case "false" is returned:
				there is no enabled and occurred event (unsatisfied constraints)
				no  function is called.
				returns false
			
			be careful: if you hold an event with proc == nullptr, enabled == true and the event happened:
			This event counts as the one event which was executed, despite no handler function is called at all.
		
		if you set all = true:
			!!!WARNING!!!
			you should know exactly what you're doing if you use this.
			exec will run the handler functions which have to be run in the order as shown above.
			returns true if there was at least one enabled event which occurred
			(even if it was actually not executed because of nullptr).
			
			using this case when you don't know what you do can be dangerous
			because e.g. one handler function might change the event handler container,
			so different events are executed, not the original ones.
	*/
	bool exec(bool all = false);
	
	//	exec(false);
	//	see description of exec
	inline bool execOne(){	return exec(false); }
	
	//	exec(true);
	//	see description of exec
	//	using this function is not recommended.
	inline bool execAll(){	return exec(true); 	}
	
	//	macro to do:
	//		readInput()
	//		return exec( [your_argument] )
	//		rex(true) is not recommended.
	//		see exec()
	inline bool rex(bool all = false){	readInput();	return exec(all);	}
	
	//	rex(false);
	//	see at rex...
	inline bool rexOne(){	return rex(false); }
	
	//	rex(true);
	//	see at rex ...
	//	using this function is not recommended.
	inline bool rexAll(){	return rex(true); }
	
	//	set callback reference (Callable*) and enable flag for given eventID
	// with an illegal eventId it does nothing
	void setEvent(int8_t eventId, concepts::Callable* callback, bool enabled = true);
	
	//	set callback reference (function*) and enable flag for given eventID
	// with an illegal eventId it does nothing
	void setEvent(int8_t eventId, void (*callbackProcedure)(), bool enabled = true);
	
	//	set callback reference and enable flag for given event description
	template <typename T>
	inline void setEvent(uint8_t button, bool up_or_down, T callback, bool enabled = true){
		setEvent(makeEvent(button,up_or_down), callback, enabled);
	}
	
	/* set all event handlers the same function */
	/* also setting the enabled flag */
	/* up_down_both=  -1:only button down events are effected, 0 all events, 1 only button up events */
	template <typename T>
	inline void setEventsAll(T callback, bool enabled = true, int8_t up_down_both = 0){
		static_assert(BUTTON_DOWN == 1, "loop has undefined behaviour");
		static_assert(BUTTON_UP == 0, "loop has undefined behaviour");
		for (int8_t i = (up_down_both == -1); i<10; i = i + 1 + (up_down_both != 0)) setEvent(i, callback, enabled);
	}
	
	/* enables event of given eventId */
	/* returns true if it was possible to enable, otherwise false */
	/* with illegal eventId nothing will be done and false will be returned */
	bool enableEvent(int8_t eventId);
	
	/* set the event [eventId] disabled */
	/* if [deleting] it is not possible to re-enable the event */
	void disableEvent(int8_t eventId, bool deleting = false);	
	
	/* disables all events */
	/* if [deleting] all events are deleted */
	inline void disableEventsAll(bool deleting = false){
		for (int8_t id = 0; id<10; ++id) disableEvent(id, deleting);
	}
	
	/* disable given event and mark reference invalid */
	inline void deleteEvent(int8_t eventId) {disableEvent(eventId,true);}
	
	/* disable all events and make references invalid */
	inline void deleteEventsAll() {disableEventsAll(true);}
	
	/* enables all events which have a valid callback reference */
	inline void enableEventsAll(){
		for (uint8_t id = 0; id<10; ++id) enableEvent(id);
	}
	
	/*	returns the eventID of the single button event which occurred in case there was one single event captured
		if more than one single event occurred, it returns MULTI_CHANGE
		if there was no event, it returns NO_CHANGE
		*/
	int8_t getEventCode();
	
}


#ifdef ARCH_MAYBE_NOT_READY

class ArcProgramItemManager final : public ItemManager {
	
	private:
		
		/* this is the final procedure that will be executed when leaving the selection by any item */
		void (*finalProcedure)();
		
		bool includingOffProgram; /* decide whether OFF should be an item or not */
		
	protected:
	
		inline virtual int16_t getSize() override { /* return the size of the item list */
			return arch::programCount() + includingOffProgram; // without the OFF Program + 1(iff with OFF)
		}
		
		inline virtual void getItemLabelInternal(char* string_8_bytes) override { /* copy the item Label into the  given string max 8 characters */
			arch::getProgramName(position + (!includingOffProgram) ,string_8_bytes);
		}
		
		virtual void runItemProcedureInternal() override; /* start running slected program in arch // this must be changed for later uses // and run the finalize function if non-null */
		
	public:
		
		ArcProgramItemManager() : finalProcedure(nullptr), includingOffProgram(true){}
		
		inline virtual void init(void (*cancelProcedure)(), void (*finalProcedure)(), bool includingOffProgram) {
			ItemManager::init(cancelProcedure);
			this->finalProcedure = finalProcedure;
			this->includingOffProgram = includingOffProgram;
		}
		
		void operator delete(void* ptr){
			// ## throw an error because this shouldn't be used in any case
		}
		//virtual ~ArcProgramItemManager(){}
};

#endif


#endif /* F_GUI_H_ */
