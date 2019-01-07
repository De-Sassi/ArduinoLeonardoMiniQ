// States.h

#ifndef _STATES_h
#define _STATES_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class StatesClass
{
 protected:


 public:
	void init();

};

extern StatesClass States;

enum State
{
	MENU,
	FOLLOW,
	HUMAN_CONTROL,
	FOLLOW_AND_CHANGE

};
#endif

