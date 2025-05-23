#pragma once
#include "../../SDK/Interfaces/IGameEvents.h"
#include <iostream>

class CEventListner : public IGameEventListener2 {
public:
	void FireGameEvent(IGameEvent* event);
	int GetEventDebugID();
	void Register();
	void Unregister();
};

extern std::unique_ptr<CEventListner> EventListner;