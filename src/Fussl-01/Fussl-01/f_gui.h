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
			suggestions, the controller might be more uniform to use
	*/
	constexpr bool BUTTON_DOWN {1};	// button pressed - event
	constexpr bool BUTTON_UP {0};	// button released - event
	/*		
	Each button event has an unique id (eventID):
	
		eventID = [buttonNumber] * 2 + [button_state]
			where button_state is BUTTON_DOWN if the button >was pressed< down and >is now< down, and BUTTON_UP otherwise
			
		buttonID is a value 0 .. 9
		
		we will talk about a "single button event" if we talk about the changing state of one single button
	
	The next values are used for special cases:
	*/	
	constexpr int8_t MULTI_CHANGE {-1};					// more than one button changed it's state
	constexpr int8_t NO_CHANGE {10};					// no button changed it's state
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
				If you only want to point to static (class) methods there won't be any problem with using (1).
				
	You can always use only one of both kinds of callbacks for each single button event
	but you can change this decision anytime you want.
	Different buttons may have different kinds of callbacks at the same time.
	
	In order to this behavior callbacks are nested in a union to save bytes of RAM
	*/
	
	typedef union callback_u* pcallback_t;
	typedef union callback_u {
		void (*callbackProcedure)();		// enabled by 1
		Callable* callbackObject;		// enabled by 2
		} callback_t;
	
	/*
	
	Each stored event handler reference comes with an enable flag
	case: an event handler is enabled (enable>0):
		This flag tells whether a callback function or a callback object is used.
	case: no event handler is enabled (enable <= 0);
	*/
	
	typedef struct event_s* pevent_t; // struct to hold a pointer to a function and an enable flag
	typedef struct event_s {
		callback_t callbackReference;
		int8_t enable;
		} event_t;
	
	// struct to hold an event array (one event_t for each single-event)	
	typedef struct event_container_s* p_event_container_t; 
	typedef struct event_container_s {
		event_t buttonEvent[5][2];
		// hidden things shadow events or maybe...
		} event_container_t;
	
	extern uint8_t gbutton; // current button state 
	extern uint8_t gchange; // current button changes (difference between gbutton and gbutton @ before)
	extern event_container_t inputEvents; // events to be executed when someone calls exec
	
	inline void init(){ /* init input pins of controller and variables of this library */
		DDRA  &= 0b00000111; // set all inputs (should be so before) <<<<< test via reading register
		PORTA |= 0b11111000; // activate pull up resistors
		gbutton = 0;
		gchange = 0;
		//disableEvents();###
	}
	
	inline int8_t makeEvent(uint8_t button, bool up_or_down){ /* return the int code of a given event */
		return 2*button+up_or_down;
	}
	
	//int8_t getEventOld();/* returns a code of the last recognized event */
		///* returns  (-1) if there were more than one button state changes		(getEvent_MULTI_CHANGE)	*/
		///* returns  (10) if there was no event recognized						(getEvent_NO_CHANGE)	*/
		///* returns  changed_button * 2 + (0 if released | 1 if pressed)				*/
	//
	//void fetchEvents(); /* update the gbutton and gchange, call event method in inpuEvents */
	//
	//
	//void enableEventOld(uint8_t buttonEvent, void (*procedure)()); /* set a single button event */
	//
	//inline void guiInputEnableAll(void (*proc)()){ /* set all button events to the same handler function */
		//for(uint8_t i = 0; i<10; ++i) enableEventOld(i,proc);
	//}
	
	//inline void disableEventOld(uint8_t buttonEvent){ /* disable a single button event */
	//	inputEvents.buttonEvent[buttonEvent/2][buttonEvent%2].enable = false;
	//}
	
	//void disableEvents(); /* disable all button events */	
	
	/// refactor:::
	/*
	readInput()
	exec();
	execOne(); // executes only one event
	execAll(); // executes all events
	
	rex()
	rexOne(); // read and execute one event
	rexAll(), // read and execute all events
	
	setEvent(event-id, procedure, enabled = true)
	getEvent(event-id) returns the event_t structure
	
	isEnabled(event_id)
	enableEvent(event-id) returns event from get event
	disableEvent(event-id) 
	
	
	
	*/
		/* read the 5 button states
			update gbutton (containing the button states at reading time)
			update gchange (containing the difference between last button state and current)
			returns true if and only if any button changed <=> gchange!=0
			*/
	bool readInput();
	
		/*	if you set all = false (default):
			case "true":
				execute one and only one event handler method:
				constraints: the executed event handler function ptr must be non-nullptr,
				enable must be true, the event happened
				
				order: button 4 > button 3 > .. > button 0; event of higher button comes first.
				returns true
				
			case "false":
				there is no event function that can be executed (unsatisfied constraints)
				no  function is called.
				returns false
			be careful: if you hold and event with proc == nullptr, enabled == true and the event happened.
			This event counts as the one event which was executed, despite no handler function is called at all.
			
			if you set all = true:
				you should know exactly what you're doing if you use this.
				exec will run the handler functions which have to be runned in the order as shown above.
				returns true if there was at least one enabled event to run.
				
				using this case when you don't know what you can be dangerous
				because e.g. one handler function might change the event handler container,
				so different events are executed, not the original ones.
		*/
	bool exec(bool all = false);
		
	inline bool execOne(){	return exec(false); }
	
	inline bool execAll(){	return exec(true); 	}
	
		/* readInput and exec(argument) */
	inline bool rex(bool all = false){
		readInput();
		return exec(all);
	}
	
	inline bool rexOne(){	return rex(false); }
		
	inline bool rexAll(){	return rex(true); }
	
	void setEvent(int8_t eventId, Callable * callback, bool enabled = true);
	void setEvent(int8_t eventId, void (*callbackProcedure)(), bool enabled = true);
	
	template <typename T>
	inline void setEvent(uint8_t button, bool up_or_down, T callback, bool enabled = true){
		setEvent(makeEvent(button,up_or_down), callback, enabled);
	}
		/* set all event handlers the same function */
		
	/* this causes ambiguousness
	
	template <typename T>
	inline void setEvent(T callback, bool enabled = true){
		for (uint8_t i = 0; i<10; ++i) setEvent(i, callback, enabled);
	}
	*/
	
		/* it is not recommended to use this function. You should hold your state in your own prg logic*/
	//const event_t& getEvent(uint8_t eventId);
	
		/* it is not recommended to use this function. You should hold your state in your own prg logic*/
	/*inline const event_t& getEvent(uint8_t button, bool up_or_down){
		return getEvent(makeEvent(button,up_or_down));
	}*/
		/* it is not recommended to use this function. You should hold your state in your own prg logic*/
	/*inline bool isEnabled(uint8_t eventId){
		return getEvent(eventId).enable;
	}*/
	
	bool enableEvent(uint8_t eventId);
		// returns true if it could enable, otherwise false
	
	void disableEvent(uint8_t eventId);
	
	inline void disableEventsAll(){
		for (uint8_t id = 0; id<10; ++id) disableEvent(id);
	}
	
	inline void enableEventsAll(){
		for (uint8_t id = 0; id<10; ++id) enableEvent(id);
	}
	
	template <typename T>
	inline void enableEventsAll(T callback){
		setEvent(callback, true);
	}
	
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
