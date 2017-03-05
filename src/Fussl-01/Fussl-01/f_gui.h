/*
 * f_gui.h
 *
 * Created: 12.09.2016 10:25:26
 * Author: Maximilian Starke
 *
 *	FILE PROGRESS STATE:
 *	namespace input is ready and prg logic was checked again
 * ItemSelector was checked but do this once again
 *	ItemMangaer checked but do this once again
 *	
 */ 


#ifndef F_GUI_H_
#define F_GUI_H_

#include "f_arch.h"
#include "f_hardware.h"

class Callable {
	public:
	virtual void operator()() const = 0;
};
typedef Callable* PCallable;

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
					You can  give input a pointer to your void-terminated function to call on event.
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
	
	typedef union callback_u* pcallback_t;
	typedef union callback_u {
		void (*callbackProcedure)();		// enabled by 1
		Callable* callbackObject;			// enabled by 2
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
			in case enable == 0 this process will be going to fail.
	*/
	
	typedef struct event_s* pevent_t;
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
	
	extern uint8_t gbutton; // current button state
	extern uint8_t gchange; // current button changes (difference between gbutton and gbutton @ before)
	extern EventContainer inputEvents; // events to be executed when someone calls exec
	
	/*	activate the input pins of the controller,
		init the event array by deleting all events.
		*/
	inline void init();
	
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
	void setEvent(int8_t eventId, Callable* callback, bool enabled = true);
	
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
	
	/* enables event of given eventId*/
	/* returns true if it was possible to enable, otherwise false*/
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

/*********************************************************************************************************************************************************************************************************************/
class ItemManager;
class ItemSelector;


class ItemSelector final {
	
	/************************************************************************/
	/* HOW TO USE:
		1.	Create an ItemSelector.
		2.	init() the ItemSelector.
		3.	run() the ItemSelector
		[	you may pause() and resume() the ItemManager several times	]
		4.	finalize the ItemSelector yourself or
			let it be finalized automatically by an okay button event.
			                                                                */
	/************************************************************************/
	
	public:
	
	static constexpr uint8_t NO_BUTTON {255};
	
	private:
	class OkayCall final : public Callable {
		private:
		ItemSelector* host;
		
		public:
		OkayCall(ItemSelector* itemSelector) : host(itemSelector) {}
		
		virtual void operator()() const final override;
	};
	
	class NextCall final : public Callable {
		private:
		ItemSelector* host;
		
		public:
		NextCall(ItemSelector* itemSelector) : host(itemSelector) {}
		
		virtual void operator()() const final override;
	};
	
	class PreviousCall final : public Callable {
		private:
		ItemSelector* host;
		
		public:
		PreviousCall(ItemSelector* itemSelector) : host(itemSelector) {}
		
		virtual void operator()() const final override;
	};
	
	OkayCall okayCall {this};
	NextCall nextCall {this};
	PreviousCall previousCall {this};
	
	ItemManager* itemManager {nullptr} ;
	uint8_t button_okay {NO_BUTTON};
	uint8_t button_next {NO_BUTTON};
	uint8_t button_prev {NO_BUTTON};
	
	uint8_t ok_pressed;
	
	/* check whether buttons and PItemManager are valid */
	inline bool isConfigValid();
	
	/* enable button events (for internal use only) */
	/* setEvent for ok_bttn AND (next_bttn OR prev_bttn) */
	/* button_down is used. the button_ups will be disabled */
	void occupyButtons();
	
	/* disable the NEXT, PREVIOUS & OKAY methods associated to the buttons (for internal use only) */
	void freeButtons();
	
	public:
	
	/* minimalistic constructor. please init data via init() */
	ItemSelector(){};
	
	/* init variables, the ItemManager* is just a pointer to an ItemManager. You have to care about construction, destruction yourself */
	inline void init(ItemManager* itemManager, uint8_t button_okay, uint8_t button_next, uint8_t button_prev);
	// checkout bevaviour button = 7;
	
	/* stop the ItemSelector */
	/* disable button events (for using programmer) */
	inline void finalize(){
		freeButtons();
		button_okay = NO_BUTTON;
		//button_next = NO_BUTTON;
		//button_prev = NO_BUTTON;
		//itemManager = nullptr;
		// it is enough to make the ItemSelector invalid.
	}
	
	/* stop the ItemSelector but keep all data to start running again from current state */
	inline void pause(){
		freeButtons();
	}
	
	/* wrapper: resume after pause() [means calling run() again] */
	inline bool resume(){
		return run();
	}
	
	/* print the current item label (...where position of ItemManager is pointing to) */
	///<<<<<< make this function virtual, create a new class for ledline-printing ItemSelectors
	// you are not supposed to use this. But maybe it could be useful if you build an extern screen saver.
	void printItem();

	/* method for the user programmer to call to start the selecting engine */
	/* returns false if there is no item given and no valid cancel_procedure */
	/* returns false if you [init()]ed illegal buttons */
	/* returns true if the selector was started */
	bool run();
	
};


class ItemManager {
	
	/************************************************************************/
	/*	ItemManager is an abstract class which is supposed to be base class
		of any further item manager.
		
		pure virtual functions that you need to override are:
			getItemLabelInternal
			runItemProcedureInternal
			getSize
			[maybe] init
			overwritten init has to init its own stuff first and call init of super class afterwards.
		                                                                     */
	/************************************************************************/
	
	private:
	/* return whether the selected item is the cancel item*/
	inline bool onCancelItem(){
		return (canCancel()) && (position == countItems()-1);
	}
	
	void (*cancelProcedure)(); /* pointer to void terminated cancel function */
	
	/* return the amount of items (inclusive cancel item if present) (the mod to iterate) */
	inline int16_t countItems(){
		return canCancel() + getSize();
	}
	
	/* return true <=> existing cancel procedure as item <=> cancelProcedure!=nullptr */
	inline bool canCancel(){
		return cancelProcedure!=nullptr;
	}
	
	protected:
	
	/* selection position / item position */
	/* standard constraints: 0 <= position <= 32.001 */
	int16_t position;
	
	/* read the position and write the matching label to given pointer */
	virtual void getItemLabelInternal(char* string_8_chars) = 0;
	
	/* read the position and execute the matching item procedure */
	virtual void runItemProcedureInternal() = 0;
	
	/* constraint:  0 <= getSize() <= 32.000 */
	/* returns the amount of items (except the cancelItem)*/
	virtual int16_t getSize() = 0;
		// explained: (2^15 - 3) (productive items) + 1 cancelItem + 1 (increasing) = must be less than 2^15 //
		// constraint:  0 <= getSize() <= 32.765 = 2^15 -3 to avoid fatal overflows //
	
	public:
	
	/* minimalistic constructor. You need to init data with init() */
	ItemManager() : cancelProcedure(nullptr), position(0) {}

	/* set the cancel procedure and maybe the starting position for selecting */
	inline virtual void init(void (*cancelProcedure)(), int16_t std_position = 0){
		this->cancelProcedure = cancelProcedure;
		this->position = std_position % countItems();// ### it shoud fall under a % first before saving into var. !!! but getSize can only run if subclass was init..ed
	}
	
	/* return true if there isn't any item, even no cancelItem */
	inline bool isEmpty() {
		return countItems()==0;
	}
	
	/* run the cancel procedure if possible and return canCancel() */
	inline bool runCancelProcedure();
	
	/* increase the item position, warning: screen update is task of ItemSelector*/
	ItemManager& operator++(); 
	
	/* decrease the item position, warning: screen update is task of ItemSelector*/
	ItemManager& operator--();
	
	/* writes the current item label to the pointer position, string might be non-(null terminated) */
	void getItemLabel(char* string_8_bytes);
	
	/* run the procedure of the selected item*/
	void runItemProcedure();
};

/*
class ListItemManager final: public ItemManager{
	
	private:
	
	t_item (*itemMemory)(uint8_t index);
	uint8_t size;
	
	protected:
	
	virtual void getItemLabelInternal(char* string_8_bytes) override {
		hardware::copyString(string_8_bytes,itemMemory(position).label,8,false);//this copying should be done in the item array
	}
	
	virtual void runItemProcedureInternal() override {
		itemMemory(position).procedure();
	}
	
	virtual uint8_t getSize() override {
		return size;
	}
	
	public:
	
	ListItemManager() : ItemManager() , itemMemory(nullptr), size(0){}
	
	virtual void init(void (*cancelProcedure)(),t_item (*itemMemory)(uint8_t index), uint8_t size){
		ItemManager::init(cancelProcedure);
		this->itemMemory = itemMemory;
		this->size = size;
	}
	
	virtual void finalize(){
		size = 0;
		itemMemory = nullptr;
		ItemManager::finalize();
	}
	
	
	virtual ~ListItemManager(){}
};
*/

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
