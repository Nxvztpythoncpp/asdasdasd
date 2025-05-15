#pragma once
#include "../../Utils/VitualFunction.h"

class IMaterialSystemHardwareConfig
{
public:
	void set_hdr_enabled(bool enabled) {
		using fn = void(__thiscall*)(IMaterialSystemHardwareConfig*, bool);
		return CallVFunction<fn>(this, 50)(this, enabled);
	}
};