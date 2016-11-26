/*
 * f_gui.h
 *
 * Created: 12.09.2016 10:25:26
 *  Author: F-NET-ADMIN
 */ 


#ifndef F_GUI_H_
#define F_GUI_H_

#include "f_arch.h"
#include "f_hardware.h"

namespace input {
	
	/*		buttons are considered like this:
			
			button4 is config menu button; (on the left)
			button3 is global select button / paging button;
			button2..0 feature-programmable control keys	
	*/
	
	constexpr bool BUTTON_DOWN {1};	// button was pressed
	constexpr bool BUTTON_UP {0};	// button was released
	
	constexpr int8_t getEvent_MULTI_CHANGE {-1};
	constexpr int8_t getEvent_NO_CHANGE {10};

	typedef struct event_s* pevent_t;
	typedef struct event_s {
		void (*proc)();
		bool enable;
		} event_t;
	
	typedef struct event_container_s* p_event_container_t;
	typedef struct event_container_s {
		event_t buttonEvent[5][2];
		// hidden things shadow events or maybe...
		} event_container_t;
	
	extern uint8_t gbutton;
	extern uint8_t gchange;
	extern event_container_t inputEvents;
	
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
	
	int8_t getEvent();/* returns a code of the last recognized event */
		/* returns  (-1) if there were more than one button state changes		(getEvent_MULTI_CHANGE)	*/
		/* returns  (10) if there was no event recognized						(getEvent_NO_CHANGE)	*/
		/* returns  changed_button * 2 + (0 if released | 1 if pressed)				*/
	
	void fetchEvents(); /* update the gbutton and gchange, call event method in inpuEvents */
	
	
	void enableEvent(uint8_t buttonEvent, void (*procedure)()); /* set a single button event */
	
	inline void guiInputEnableAll(void (*proc)()){ /* set all button events to the same handler function */
		for(uint8_t i = 0; i<10; ++i) enableEvent(i,proc);
	}
	
	inline void disableEvent(uint8_t buttonEvent){ /* disable a single button event */
		inputEvents.buttonEvent[buttonEvent/2][buttonEvent%2].enable = false;
	}
	
	void disableEvents(); /* disable all button events */	
}

/*********************************************************************************************************************************************************************************************************************/
class ItemManager;

namespace ItemSelector {
	
	constexpr uint8_t NO_BUTTON {255};
	
	extern ItemManager* itemManager; // version 1.1 try to change to a reference c++ like <<<<<<<<<
	extern uint8_t button_okay;
	extern uint8_t button_next;
	extern uint8_t button_prev;
	
	void enableButtons(); /* enable button events (for internal use only) */
	
	void setButtonsFree(); /* disable the NEXT, PREVIOUS & OKAY methods associated to the buttons (for internal use only) */
	
	void initialisation(uint8_t button_okay, uint8_t button_next, uint8_t button_prev, ItemManager* itemManager); /* init variables, the ItemManager* is just a pointer to an ItemManager. You have to care about construction, destruction yourself */
	
	inline void finalize(){ /* disable button events (for using programmer) */
		setButtonsFree();
	}
		
	void okay_down(); /* the 'first' okay button event method */
	void okay_up(); /* the 'second' okay button event method, it also finalize()s the semi-singleton-object */
	
	void next(); /* the next-button - event method */
	void previous(); /* the previous-button - event method */
	
	void printItem(); /* print the current item label where position is pointing to */

	bool run(); /* method for the user programmer to call to start the selecting engine */
		/* returns false only if there is no item given and no valid cancel_procedure */
}



class ItemManager {
	
	private:
		inline bool onCancelItem(){ /* return wehther the selected item is the cancel item*/
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