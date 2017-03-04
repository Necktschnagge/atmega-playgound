/*
 * f_gui.h
 *
 * Created: 12.09.2016 10:25:26
 * Author: Maximilian Starke
 */ 


#ifndef F_GUI_H_
#define F_GUI_H_

#include "f_arch.h"
#include "f_hardware.h"

class Callable {
	public:
	virtual void operator()() const = 0;
};

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
	/* this causes ambiguousness */ //##check this description, I renamed this from setEvent to ..all
	template <typename T>
	inline void setEventsAll(T callback, bool enabled = true){
		for (uint8_t i = 0; i<10; ++i) setEvent(i, callback, enabled);
	}
	
	bool enableEvent(uint8_t eventId);
		// returns true if it could enable, otherwise false
	
	void disableEvent(uint8_t eventId, bool deleting = false);
	
	inline void disableEventsAll(bool deleting = false){
		for (uint8_t id = 0; id<10; ++id) disableEvent(id, deleting);
	}
	
	inline void deleteEvent(uint8_t eventId) {disableEvent(eventId);}
	
	inline void deleteEventsAll() {disableEventsAll(true);}
	
	inline void enableEventsAll(){
		for (uint8_t id = 0; id<10; ++id) enableEvent(id);
	}
	
	template <typename T>
	inline void enableEventsAll(T callback){
		setEvent(callback, true);
	}
	/*	returns the eventID of the single button event which occurred in case there was one single event captured
		if more than one single event occurred, it returns MULTI_CHANGE
		if there was no event, it returns NO_CHANGE
		*/
	uint8_t getEventCode();
	
}

/*********************************************************************************************************************************************************************************************************************/
class ItemManager;
class ItemSelector;


class ItemManager {
	
	private:
	inline bool onCancelItem(){ /* return whether the selected item is the cancel item*/
		return (canCancel()) && (static_cast<uint8_t> (position) == mod()-1);
	}
	
	void (*cancelProcedure)(); /* pointer to void cancel function */
	
	inline uint16_t mod(){ /* return the amount of items (the mod to iterate) */
		return canCancel() + getSize();
	}
	
	inline bool canCancel(){ /* return true if there is a cancel procedure as item */
		return cancelProcedure!=nullptr;
	}
	
	protected:
	
	int16_t position; /* position of the selected item */
	// ## think about size restrictions!!!
	
	virtual void getItemLabelInternal(char* string_8_chars) = 0;
	
	virtual void runItemProcedureInternal() = 0;
	
	virtual uint8_t getSize() = 0;
	
	
	public:
	
	/*typedef struct item_s* pitem_t;*/
	typedef struct item_s {
		char label[8];
		void (*procedure)();
	} item_t;
	
	ItemManager() : cancelProcedure(nullptr), position(0) {}
	
	inline bool isEmpty(){ /* return true if there isn't any item */
		return mod()==0;
	}
	
	bool runCancelProcedure(); /* run the cancel procedure if (cancancel) and return if cancelProcedure was executed */
	
	inline virtual void init(void (*cancelProcedure)()){ /* set the private /protected virables */
		this->cancelProcedure = cancelProcedure;
		this->position = 0;
	}
	
	inline virtual void finalize(){ /* delete cancelProcedure */
		cancelProcedure = nullptr;
		position = 0;
	}
	
	ItemManager& operator++(); /* increase the item pointer, warning: screen update is still your task */
	
	ItemManager& operator--(); /* decrease the item pointer, warning: screen update is still your task */
	
	void getItemLabel(char* string_8_bytes); /* set the value of an 8 byte char array so that it will contain the item label, string might be non-(null terminated) */
	
	void runItemProcedure(); /* run the procedure of the selected item and finalize the ItemManager */
	
	virtual ~ItemManager(){}
};


class ItemSelector {
	
	public:
	
	static constexpr uint8_t NO_BUTTON {255};
		
	private:
	class OkayCall : public Callable {
		private:
			ItemSelector* host;
			
		public:
			OkayCall(ItemSelector* itemSelector) : host(itemSelector) {}
			
			virtual void operator()() const override;
	};
	
	class NextCall : public Callable {
		private:
		ItemSelector* host;
		
		public:
		NextCall(ItemSelector* itemSelector) : host(itemSelector) {}
		
		virtual void operator()() const override {
			//itemSelector->next();
			++(*host->itemManager);
			host->printItem();
		}
	};
	
	class PreviousCall : public Callable {
		private:
		ItemSelector* host;
		
		public:
		PreviousCall(ItemSelector* itemSelector) : host(itemSelector) {}
			
		virtual void operator()() const override {
			--(*host->itemManager);
			host->printItem();
		}
	};
	
	OkayCall okayCall {this};
	NextCall nextCall {this};
	PreviousCall previousCall {this};
	
	ItemManager* itemManager {nullptr} ;
	uint8_t button_okay;
	uint8_t button_next;
	uint8_t button_prev;
	
	uint8_t ok_pressed;
	
	void occupyButtons(); 
	/* enable button events (for internal use only) */
	
	void freeButtons(); 
	/* disable the NEXT, PREVIOUS & OKAY methods associated to the buttons (for internal use only) */
	
	public:
	
	ItemSelector(){};
	
	void init(ItemManager* itemManager, uint8_t button_okay, uint8_t button_next, uint8_t button_prev); /* init variables, the ItemManager* is just a pointer to an ItemManager. You have to care about construction, destruction yourself */
	// checkout bevaviour button = 7;
	
	inline void finalize(){ /* disable button events (for using programmer) */
		freeButtons();
	}
	
	void printItem(); /* print the current item label where position is pointing to */
			///<<<<<< make this function virtual, create a new class for ledline-printing ItemSelectors
			// you are not supposed to use this. Maybe it could be useful if you build a extern screen saver.

	bool run(); /* method for the user programmer to call to start the selecting engine */
		/* returns false only if there is no item given and no valid cancel_procedure */
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

class ArcProgramItemManager final : public ItemManager {
	
	private:
	
		void (*finalProcedure)(); /* this is the final procedure that will be executed when leaving the selection by any item */
		bool includingOffProgram; /* decide whether OFF should be an item or not */
		
	protected:
	
		inline virtual uint8_t getSize() override { /* return the size of the item list */
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




#endif /* F_GUI_H_ */
