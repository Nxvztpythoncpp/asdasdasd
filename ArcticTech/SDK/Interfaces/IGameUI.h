#pragma once
#include "../../Utils/VitualFunction.h"

class IGameUI
{
public:
	void msg_box(const char* pTitle, const char* pMessage, bool showOk, bool showCancel, const char* okCommand, const char* cancelCommand, const char* closedCommand, const char* pszCustomButtonText, const char* unknown) {

		using fn = void(__thiscall*)(IGameUI*, const char*, const char*,
			bool, bool,
			const char*, const char*,
			const char*,
			const char*, const char*);

		return CallVFunction<fn>(this, 19)(this,
			pTitle,
			pMessage,
			showOk,
			showCancel,
			okCommand,
			cancelCommand,
			closedCommand,
			pszCustomButtonText,
			unknown);
	}
};