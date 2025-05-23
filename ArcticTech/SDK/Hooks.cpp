#include "Hooks.h"
#include "../Utils/Utils.h"
#include "../Utils/Hash.h"
#include "Config.h"
#include "Globals.h"
#include <intrin.h>
#include "../Utils/Console.h"
#include "../UI/UI.h"
#include "Misc/MoveMsg.h"

#include "../Features/Misc/GameMovement.h"
#include "../Features/Visuals/ESP.h"
#include "../Features/Visuals/Glow.h"
#include "../Features/Lua/Bridge/Bridge.h"
#include "../Features/Visuals/Chams.h"
#include "../Features/Visuals/World.h"
#include "../Features/Visuals/GlovesChanger.h"
#include "../Features/Visuals/GrenadePrediction.h"
#include "../Features/Misc/Prediction.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/RageBot/LagCompensation.h"
#include "../Features/RageBot/Exploits.h"
#include "../Features/Misc/AutoPeek.h"
#include "../Features/RageBot/Ragebot.h"
#include "../Features/RageBot/AutoWall.h"
#include "../Features/RageBot/Resolver.h"
#include "../Features/RageBot/AnimationSystem.h"
#include "../Features/Misc/Misc.h"
#include "../Features/Misc/EventListner.h"
#include "../Features/Visuals/SkinChanger.h"
#include "../Features/ShotManager/ShotManager.h"
#include "../Features/Misc/PingReducer.h"
#include "../Features/Misc/PingSpike.h"

#include "Misc/xorstr.h"

#include "../custom sounds/c_sounds.h"
#include <playsoundapi.h>
#include <fstream>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace c_sounds {
	static __forceinline void modify_volume_sound(char* bytes, ptrdiff_t file_size, float volume)
	{
		int offset = 0;
		for (int i = 0; i < file_size / 2; i++)
		{
			if (bytes[i] == 'd' && bytes[i + 1] == 'a' && bytes[i + 2] == 't' && bytes[i + 3] == 'a')
			{
				offset = i;
				break;
			}
		}

		if (offset == 0)
			return;

		char* data_offset = (bytes + offset);
		DWORD sample_bytes = *(DWORD*)(data_offset + 4);
		DWORD samples = sample_bytes / 2;

		SHORT* sample = (SHORT*)(data_offset + 8);
		for (DWORD i = 0; i < samples; i++)
		{
			SHORT sh_sample = *sample;
			sh_sample = (SHORT)(sh_sample * volume);
			*sample = sh_sample;
			sample++;
			if (((char*)sample) >= (bytes + file_size - 1))
				break;
		}
	};

	__forceinline void play_sound_from_memory(uint8_t* bytes, size_t size, float volume)
	{
		static std::unordered_map< uint8_t*, std::pair< std::vector< uint8_t >, float > > cache{ };
		if (cache.count(bytes) == 0)
			cache[bytes].first.resize(size);

		auto& current = cache[bytes];
		uint8_t* stored_bytes = current.first.data();

		// modify sound only when changed volume
		if (current.second != volume)
		{
			std::memcpy(stored_bytes, bytes, size);
			current.second = volume;
			modify_volume_sound((char*)stored_bytes, size, volume);
		}

		PlaySoundA((char*)stored_bytes, NULL, SND_ASYNC | SND_MEMORY);
	}
}

GrenadePrediction NadePrediction;

template <typename T>
inline T HookFunction(void* pTarget, void* pDetour) {
	return (T)DetourFunction((PBYTE)pTarget, (PBYTE)pDetour);
}

inline void RemoveHook(void* original, void* detour) {
	DetourRemove((PBYTE)original, (PBYTE)detour);
}

LRESULT CALLBACK hkWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (!Render->IsInitialized() || !Menu->IsInitialized())
		return CallWindowProc(oWndProc, Hwnd, Message, wParam, lParam);

	if (Message == WM_KEYDOWN && !ctx.console_visible) {
		AntiAim->OnKeyPressed(wParam);
	}

	if (Menu->IsOpened() && Menu->WndProc(Hwnd, Message, wParam, lParam))
		return 0;

	return CallWindowProc(oWndProc, Hwnd, Message, wParam, lParam);
}

HRESULT __stdcall hkReset(IDirect3DDevice9* thisptr, D3DPRESENT_PARAMETERS* params) {
	static tReset oReset = (tReset)Hooks::DirectXDeviceVMT->GetOriginal(16);

	auto result = oReset(thisptr, params);

	Render->Reset();

	return result;
}

HRESULT __stdcall hkPresent(IDirect3DDevice9* thisptr, const RECT* src, const RECT* dest, HWND window, const RGNDATA* dirtyRegion) {
	if (thisptr != DirectXDevice)
		return oPresent(thisptr, src, dest, window, dirtyRegion);

	if (!Render->IsInitialized()) {
		Render->Init(thisptr);
		Menu->Setup();

		return oPresent(thisptr, src, dest, window, dirtyRegion);
	}

	Cheat.InGame = EngineClient->IsConnected() && EngineClient->IsInGame();

	if (thisptr->BeginScene() == D3D_OK) {
		static bool sound_started = false;
		static bool console_p = false;

		if (!console_p) {

			EngineClient->ExecuteClientCmd("clear");
			Console->ColorPrint("\n\nSunset - information\n\n\n", Color(105, 163, 255, 255));
			Console->ColorPrint(std::format("build: {}\n", __DATE__), Color(95, 153, 245, 255));
			console_p = true;
		}

		if (!sound_started) {
			c_sounds::play_sound_from_memory((uint8_t*)load_sound, sizeof(load_sound), 1.f); // uwukson: why not
			sound_started = true;
		}

		Render->RenderDrawData();
		Menu->Draw();
		thisptr->EndScene();
	}

	return oPresent(thisptr, src, dest, window, dirtyRegion);
}

void __fastcall hkHudUpdate(IBaseClientDLL* thisptr, void* edx, bool bActive) {
	static auto oHudUpdate = (tHudUpdate)Hooks::ClientVMT->GetOriginal(11);

	Cheat.LocalPlayer = (CBasePlayer*)EntityList->GetClientEntity(EngineClient->GetLocalPlayer());

	ctx.active_weapon = Cheat.LocalPlayer ? Cheat.LocalPlayer->GetActiveWeapon() : nullptr;
	ctx.weapon_info = ctx.active_weapon ? ctx.active_weapon->GetWeaponInfo() : nullptr;

	if (!Render->IsInitialized() || !Menu->IsInitialized())
		return;

	Render->BeginFrame();

	Render->UpdateViewMatrix(EngineClient->WorldToScreenMatrix());

	WorldESP->Draw();
	WorldESP->OtherESP();
	WorldESP->RenderMarkers();

	NadePrediction.Start();
	NadePrediction.Draw();

	AutoPeek->Draw();
	World->Crosshair();
	DebugOverlay->RenderOverlays();

	LUA_CALL_HOOK(LUA_RENDER);

	Render->EndFrame();

	oHudUpdate(thisptr, edx, bActive);
}

void __fastcall hkLockCursor(ISurface* thisptr, void* edx) {
	static tLockCursor oLockCursor = (tLockCursor)Hooks::SurfaceVMT->GetOriginal(67);

	if (Menu->IsOpened())
		return Surface->UnlockCursor();

	oLockCursor(thisptr, edx);
}

MDLHandle_t __fastcall hkFindMdl(void* ecx, void* edx, char* FilePath)
{
	static auto oFindMdlHook = (tFindMdlHook)Hooks::ModelCacheVMT->GetOriginal(10);

	if (strstr(FilePath, "facemask_battlemask.mdl"))
		sprintf(FilePath, "models/player/holiday/facemasks/facemask_dallas.mdl");

	return oFindMdlHook(ecx, edx, FilePath);
}

void __stdcall CreateMove(int sequence_number, float sample_frametime, bool active, bool& bSendPacket) {
	static auto oCHLCCreateMove = (tCHLCCreateMove)Hooks::ClientVMT->GetOriginal(22);

	oCHLCCreateMove(Client, sequence_number, sample_frametime, active);

	CUserCmd* cmd = Input->GetUserCmd(sequence_number);
	CVerifiedUserCmd* verified = Input->GetVerifiedCmd(sequence_number);

	Miscellaneous::Clantag();

	Exploits->DefenseiveThisTick() = false;
	Exploits->force_charge = false;

	ctx.active_weapon = nullptr;

	if (Cheat.InGame) {
		auto netchannel = EngineClient->GetNetChannelInfo();

		if (netchannel) {
			if (!netchannel->IsLoopback()) {
				ctx.real_ping = netchannel->GetLatency(FLOW_OUTGOING);
			}
			else {
				ctx.real_ping = -1.f;
			}
		}
	}
	else {
		ctx.real_ping = -1.f;
	}

	if (!cmd || !cmd->command_number || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !active)
		return;

	ctx.active_weapon = Cheat.LocalPlayer->GetActiveWeapon();
	ctx.weapon_info = ctx.active_weapon ? ctx.active_weapon->GetWeaponInfo() : nullptr;

	if (!cmd || !cmd->command_number)
		return;

	EnginePrediction->Update();

	ctx.cmd = cmd;
	ctx.send_packet = true;

	Exploits->PrePrediction(); // update tickbase info

	CUserCmd_lua lua_cmd;
	lua_cmd.command_number = cmd->command_number;
	lua_cmd.tickcount = cmd->tick_count;
	lua_cmd.move = Vector(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	lua_cmd.viewangles = cmd->viewangles;
	lua_cmd.random_seed = cmd->random_seed;
	lua_cmd.buttons = cmd->buttons;

	LUA_CALL_HOOK(LUA_CREATEMOVE, &lua_cmd);

	cmd->buttons = lua_cmd.buttons;
	cmd->sidemove = lua_cmd.move.y;
	cmd->forwardmove = lua_cmd.move.x;
	cmd->tick_count = lua_cmd.tickcount;
	cmd->viewangles = lua_cmd.viewangles;

	if ((config.misc.movement.infinity_duck->get() || config.antiaim.misc.fake_duck->get()) && !ctx.no_fakeduck)
		cmd->buttons |= IN_BULLRUSH;

	if (config.misc.movement.auto_jump->get()) {
		if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) && Cheat.LocalPlayer->m_MoveType() != MOVETYPE_NOCLIP && Cheat.LocalPlayer->m_MoveType() != MOVETYPE_LADDER)
			cmd->buttons &= ~IN_JUMP;
	}

	Movement->QuickStop();
	Movement->EasyStrafe();
	AutoPeek->CreateMove();
	AntiAim->FakeDuck();
	AntiAim->SlowWalk();
	AntiAim->JitterMove();
	Movement->AutoStrafe();
	Miscellaneous::AutomaticGrenadeRelease();
	Miscellaneous::FastThrow();
	Miscellaneous::FastSwitch();

	if (ctx.active_weapon && ctx.active_weapon->ShootingWeapon() && Exploits->IsShifting())
		cmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);

	// pre_prediction

	EnginePrediction->Start(cmd);
	QAngle eyeYaw = cmd->viewangles;

	if (Exploits->IsShifting()) {
		Ragebot->Run();

		AntiAim->Angles();

		ctx.send_packet = bSendPacket = ctx.tickbase_shift == 1;

		cmd->viewangles.Normalize();
		EnginePrediction->End();

		Utils::FixMovement(cmd, eyeYaw);

		AntiAim->LegMovement();

		Exploits->UpdateTickbase();
		Exploits->Shift(); // we actually will not shift here, only update tickbase info

		ctx.sent_commands.emplace_back(cmd->command_number);

		verified->cmd = *cmd;
		verified->crc = cmd->GetChecksum();
		return;
	}

	ctx.is_peeking = AntiAim->IsPeeking();

	if (ctx.is_peeking && config.ragebot.aimbot.doubletap_options->get(1))
		Exploits->DefenseiveThisTick() = true;

	Exploits->AllowDefensive() = lua_cmd.allow_defensive;

	if (lua_cmd.override_defensive) {
		Exploits->DefenseiveThisTick() = lua_cmd.override_defensive.as<bool>();
	}

	Exploits->DefensiveDoubletap();

	ctx.last_local_velocity = ctx.local_velocity;
	ctx.local_velocity = Cheat.LocalPlayer->m_vecVelocity();

	Movement->CompensateThrowable();

	// prediction

	if (config.misc.movement.edge_jump->get() && !(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) && EnginePrediction->pre_prediction.m_fFlags & FL_ONGROUND)
		cmd->buttons |= IN_JUMP;

	if ((ctx.active_weapon->ShootingWeapon() || ctx.active_weapon->IsKnife()) && !ctx.active_weapon->CanShoot() && ctx.active_weapon->m_iItemDefinitionIndex() != Revolver)
		cmd->buttons &= ~IN_ATTACK;

	if ((ctx.active_weapon->ShootingWeapon() || ctx.active_weapon->IsKnife()) && ctx.active_weapon->CanShoot() && cmd->buttons & IN_ATTACK) {
		if (Exploits->GetExploitType() == CExploits::E_DoubleTap)
			Exploits->ForceTeleport();
		else if (Exploits->GetExploitType() == CExploits::E_HideShots)
			Exploits->HideShot();

		if (!config.misc.miscellaneous.gamesense_mode->get() && !ctx.active_weapon->IsKnife())
			ShotManager->ProcessManualShot();
	}

	AntiAim->lua_override.reset();

	LUA_CALL_HOOK(LUA_ANTIAIM, &AntiAim->lua_override);

	AntiAim->FakeLag(cmd);
	AntiAim->Angles();

	Ragebot->Run();
	Ragebot->RestorePrediction();

	cmd->viewangles.Normalize(config.misc.miscellaneous.anti_untrusted->get());

	if (config.misc.miscellaneous.gamesense_mode->get() && ctx.active_weapon->ShootingWeapon() && ctx.active_weapon->CanShoot() && cmd->buttons & IN_ATTACK)
		ShotManager->ProcessManualShot();

	if (Exploits->TeleportThisTick())
		ctx.send_packet = false;

	if (ctx.send_packet) {
		if (!config.antiaim.angles.body_yaw_options->get(1) || Utils::RandomInt(0, 10) >= 6)
			AntiAim->jitter = !AntiAim->jitter;

		ctx.breaking_lag_compensation = (ctx.local_sent_origin - Cheat.LocalPlayer->m_vecOrigin()).LengthSqr() > 4096;
		ctx.local_sent_origin = Cheat.LocalPlayer->m_vecOrigin();
	}

	Utils::FixMovement(cmd, eyeYaw);

	AntiAim->LegMovement();

	if (ctx.active_weapon->ShootingWeapon() && ctx.active_weapon->CanShoot() && cmd->buttons & IN_ATTACK) {
		ctx.last_shot_time = GlobalVars->realtime;
		ctx.shot_angles = cmd->viewangles;

		if (!ctx.send_packet) // fix incorrect thirdperson angle when shooting in fakelag
			ctx.force_shot_angle = true;
	}

	if (cvars.sv_infinite_ammo->GetInt() == 0 && ctx.active_weapon && ctx.active_weapon->m_iItemDefinitionIndex() == Taser && ctx.active_weapon->CanShoot() && cmd->buttons & IN_ATTACK)
		ctx.switch_to_main_weapon = true;

	EnginePrediction->End();

	// createmove

	Exploits->UpdateTickbase();
	Exploits->Shift();

	bSendPacket = ctx.send_packet;

	if (ctx.should_buy > 0) {
		ctx.should_buy--;
		std::string buy_command = "";
		if (config.misc.miscellaneous.auto_buy->get(0))
			buy_command += "buy awp; ";
		if (config.misc.miscellaneous.auto_buy->get(1))
			buy_command += "buy ssg08; ";
		if (config.misc.miscellaneous.auto_buy->get(2))
			buy_command += "buy scar20; buy g3sg1; ";
		if (config.misc.miscellaneous.auto_buy->get(3))
			buy_command += "buy deagle; buy revolver; ";
		if (config.misc.miscellaneous.auto_buy->get(4))
			buy_command += "buy fn57; buy tec9; ";
		if (config.misc.miscellaneous.auto_buy->get(5))
			buy_command += "buy taser; ";
		if (config.misc.miscellaneous.auto_buy->get(6))
			buy_command += "buy vesthelm; ";
		if (config.misc.miscellaneous.auto_buy->get(7))
			buy_command += "buy smokegrenade; ";
		if (config.misc.miscellaneous.auto_buy->get(8))
			buy_command += "buy molotov; buy incgrenade; ";
		if (config.misc.miscellaneous.auto_buy->get(9))
			buy_command += "buy hegrenade; ";
		if (config.misc.miscellaneous.auto_buy->get(10))
			buy_command += "buy flashbang; ";
		if (config.misc.miscellaneous.auto_buy->get(11))
			buy_command += "buy defuser; ";

		if (!buy_command.empty() && Cheat.LocalPlayer->m_iAccount() > 1000 && ctx.should_buy == 0)
			EngineClient->ExecuteClientCmd(buy_command.c_str());
	}

	//Console->Log(std::format("{} {} {}", cmd->command_number, bSendPacket, Exploits->GetTickbaseInfo(ctx.cmd->command_number)->extra_commands));

	verified->cmd = *cmd;
	verified->crc = cmd->GetChecksum();
}

__declspec(naked) void __fastcall hkCHLCCreateMove(IBaseClientDLL* thisptr, void*, int sequence_number, float input_sample_frametime, bool active) {
	__asm {
		push ebp
		mov  ebp, esp
		push ebx
		push esp
		push dword ptr[active]
		push dword ptr[input_sample_frametime]
		push dword ptr[sequence_number]
		call CreateMove
		pop  ebx
		pop  ebp
		retn 0Ch
	}
}

void* __fastcall hkAllocKeyValuesMemory(IKeyValuesSystem* thisptr, void* edx, int iSize)
{
	static auto oAllocKeyValuesMemory = (void* (__fastcall*)(IKeyValuesSystem*, void*, int))Hooks::KeyValuesVMT->GetOriginal(2);

	// return addresses of check function
	// @credits: danielkrupinski
	static const void* uAllocKeyValuesEngine = Utils::PatternScan("engine.dll", "55 8B EC 56 57 8B F9 8B F2 83 FF 11 0F 87 ? ? ? ? 85 F6 0F 84 ? ? ? ?", 0x4A);
	static const void* uAllocKeyValuesClient = Utils::PatternScan("client.dll", "55 8B EC 56 57 8B F9 8B F2 83 FF 11 0F 87 ? ? ? ? 85 F6 0F 84 ? ? ? ?", 0x3E);

	// doesn't call it yet, but have checking function
	//static const std::uintptr_t uAllocKeyValuesMaterialSystem = MEM::FindPattern(MATERIALSYSTEM_DLL, XorStr("FF 52 04 85 C0 74 0C 56")) + 0x3;
	//static const std::uintptr_t uAllocKeyValuesStudioRender = MEM::FindPattern(STUDIORENDER_DLL, XorStr("FF 52 04 85 C0 74 0C 56")) + 0x3;

	if (const void* uReturnAddress = _ReturnAddress(); uReturnAddress == uAllocKeyValuesEngine || uReturnAddress == uAllocKeyValuesClient)
		return nullptr;

	return oAllocKeyValuesMemory(thisptr, edx, iSize);
}

bool __fastcall hkSetSignonState(void* thisptr, void* edx, int state, int count, const void* msg) {
	bool result = oSetSignonState(thisptr, edx, state, count, msg);

	if (state == 6) { // SIGNONSTATE_FULL
		Cheat.InGame = true;

		ctx.update_nightmode = true;
		ctx.update_remove_blood = true;

		World->SkyBox();
		World->Fog();
		World->Smoke();

		if (CVar->FindVar("rate")->GetInt() != 786432)
			CVar->FindVar("rate")->SetInt(786432);

		CVar->FindVar("r_jiggle_bones")->SetInt(0);

		if (cvars.r_DrawSpecificStaticProp->GetInt() != 0)
			cvars.r_DrawSpecificStaticProp->SetInt(0);

		CVar->FindVar("cl_predict")->m_nFlags &= ~FCVAR_NOT_CONNECTED;
		CVar->FindVar("cl_simulationtimefix")->m_nFlags &= ~FCVAR_HIDDEN;
		CVar->FindVar("cl_simulationtimefix")->SetInt(0);

		cvars.cl_threaded_bone_setup->SetInt(1);
		cvars.net_earliertempents->SetInt(1);
		cvars.cl_interp->SetFloat(0.015625f);
		cvars.cl_interp_ratio->SetInt(2);

		//useless anim
		cvars.r_eyegloss->SetInt(0);
		cvars.r_eyemove->SetInt(0);
		CVar->FindVar("r_eyesize")->SetInt(0);

		cvars.dsp_slow_cpu->SetInt(1);
		CVar->FindVar("engine_no_focus_sleep")->SetInt(0);
		cvars.cl_pred_doresetlatch->SetInt(0);
		CVar->FindVar("r_player_visibility_mode")->SetInt(0);
		CVar->FindVar("dsp_enhance_stereo")->SetInt(0);

		GrenadePrediction::PrecacheParticles();
		Ragebot->CalcSpreadValues();
		ShotManager->Reset();
		Resolver->Reset();
		SkinChanger->InitCustomModels();
		NadeWarning->Precache();

		LUA_CALL_HOOK(LUA_LEVELINIT);
	}

	return result;
}

void __fastcall hkLevelShutdown(IBaseClientDLL* thisptr, void* edx) {
	static auto oLevelShutdown = (void(__thiscall*)(IBaseClientDLL*))Hooks::ClientVMT->GetOriginal(7);

	Exploits->target_tickbase_shift = ctx.tickbase_shift = 0;
	ctx.reset();
	LagCompensation->Reset();
	AnimationSystem->ResetInterpolation();
	ShotManager->Reset();

	oLevelShutdown(thisptr);
}

void __fastcall hkLevelInitPostEntity(void* ecx, void* edx)
{
	static auto original = (void(__thiscall*)(void*, void*))Hooks::ClientVMT->GetOriginal(6);

	original(ecx, edx);

	MaterialSystemHardwareConfig->set_hdr_enabled(true); // uwukson: from fatality =))))
}

void __fastcall hkOverrideView(IClientMode* thisptr, void* edx, CViewSetup* setup) {
	static tOverrideView oOverrideView = (tOverrideView)Hooks::ClientModeVMT->GetOriginal(18);

	World->ProcessCamera(setup);

	if (Cheat.LocalPlayer && Cheat.LocalPlayer->IsAlive() && ctx.fake_duck)
		setup->origin = Cheat.LocalPlayer->GetAbsOrigin() + Vector(0, 0, 64);

	if (!Input->m_fCameraInThirdPerson && Cheat.LocalPlayer->IsAlive() && config.visuals.effects.removals->get(7)) {
		setup->angles -= Cheat.LocalPlayer->m_aimPunchAngle() * 0.9f;
		setup->angles -= Cheat.LocalPlayer->m_viewPunchAngle();
	}

	setup->angles.roll = 0;

	ctx.camera_postion = setup->origin;

	oOverrideView(thisptr, edx, setup);
}

void __fastcall hkPaintTraverse(IPanel* thisptr, void* edx, unsigned int panel, bool bForceRepaint, bool bForce) {
	static tPaintTraverse oPaintTraverse = (tPaintTraverse)Hooks::PanelVMT->GetOriginal(41);
	static unsigned int hud_zoom_panel = 0;

	if (!hud_zoom_panel) {
		std::string panelName = VPanel->GetName(panel);

		if (panelName == "HudZoom")
			hud_zoom_panel = panel;
	}

	if (hud_zoom_panel == panel && config.visuals.effects.remove_scope->get() > 0)
		return;

	oPaintTraverse(thisptr, edx, panel, bForceRepaint, bForce);

	static auto zoom_sensitivity_ratio_mouse = CVar->FindVar("zoom_sensitivity_ratio_mouse");

	zoom_sensitivity_ratio_mouse->SetInt(0); // uwukson: fix sensitivity in zoom
}

void __fastcall hkDoPostScreenEffects(IClientMode* thisptr, void* edx, CViewSetup* setup) {
	static tDoPostScreenEffects oDoPostScreenEffects = (tDoPostScreenEffects)Hooks::ClientModeVMT->GetOriginal(44);

	Chams->RenderShotChams();
	Glow::Run();

	oDoPostScreenEffects(thisptr, edx, setup);
}

bool __fastcall hkIsPaused(IVEngineClient* thisptr, void* edx) {
	static void* addr = Utils::PatternScan("client.dll", "FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??", 0x29);
	static tIsPaused oIsPaused = (tIsPaused)Hooks::EngineVMT->GetOriginal(90);

	if (_ReturnAddress() == addr)
		return true;

	return oIsPaused(thisptr, edx);
}

void __fastcall hkDrawModelExecute(IVModelRender* thisptr, void* edx, void* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld) {
	static tDrawModelExecute oDrawModelExecute = (tDrawModelExecute)Hooks::ModelRenderVMT->GetOriginal(21);

	if (hook_info.in_draw_static_props)
		return oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

	if (StudioRender->IsForcedMaterialOverride())
		return oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

	if (Chams->OnDrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld))
		return;

	oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
}

void __fastcall hkFrameStageNotify(IBaseClientDLL* thisptr, void* edx, EClientFrameStage stage) {
	static auto oFrameStageNotify = (tFrameStageNotify)Hooks::ClientVMT->GetOriginal(37);
	Cheat.InGame = EngineClient->IsConnected() && EngineClient->IsInGame();
	Cheat.LocalPlayer = Cheat.InGame ? (CBasePlayer*)EntityList->GetClientEntity(EngineClient->GetLocalPlayer()) : nullptr;

	static auto player_res_pat = *(CCSPlayerResource***)(Utils::PatternScan("client.dll", "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7", 0x2));
	if (Cheat.InGame)
		PlayerResource = *player_res_pat;
	else if (!Cheat.InGame)
		PlayerResource = nullptr;

	{
		const auto rate = static_cast<int>(1.0f / GlobalVars->interval_per_tick + 0.5f);

		CVar->FindVar("cl_updaterate")->RemoveCallbacks();
		CVar->FindVar("cl_cmdrate")->RemoveCallbacks();

		CVar->FindVar("cl_updaterate")->SetInt(rate);
		CVar->FindVar("cl_cmdrate")->SetInt(rate);
	}


	switch (stage) {
	case FRAME_RENDER_START: {
		ctx.active_app = EngineClient->IsActiveApp();
		ctx.console_visible = EngineClient->Con_IsVisible();

		AnimationSystem->RunInterpolation();
		Chams->UpdateSettings();
		World->SunDirection();

		{
			/*float current_ratio = cvars.r_aspectratio->GetFloat();
			float slider_ratio = config.visuals.effects.aspect_ratio->get();
			float smoothed_ratio = std::lerp(current_ratio, slider_ratio, 0.1f);
			cvars.r_aspectratio->SetFloat(smoothed_ratio);*/

			auto is_close_func = [&](float a, float b, float epsilon = 0.01f) {
				return std::abs(a - b) <= epsilon;
				};

			float target_value;
			float current_value = cvars.r_aspectratio->GetFloat();
			float slider_value = config.visuals.effects.aspect_ratio->get();
			constexpr float epsilon = 0.01f;

			if (config.visuals.effects.aspect_ratio->get() > 0) {
				target_value = (slider_value != 0.59f) ? slider_value : 1.77f;
			}
			else {
				target_value = 1.77f;
			}

			if (!is_close_func(current_value, target_value, epsilon)) {
				float new_value = std::lerp(current_value, target_value, 0.1f);

				if (!is_close_func(new_value, current_value, epsilon)) {
					cvars.r_aspectratio->SetFloat(new_value);
				}
			}
		}

		cvars.mat_postprocessing_enable->SetInt(!config.visuals.effects.removals->get(0));
		cvars.cl_csm_shadows->SetInt(!config.visuals.effects.removals->get(2));
		cvars.cl_foot_contact_shadows->SetInt(0);
		cvars.r_drawsprites->SetInt(!config.visuals.effects.removals->get(6));

		if (Cheat.LocalPlayer && config.visuals.effects.removals->get(4))
			Cheat.LocalPlayer->m_flFlashDuration() = 0.f;

		CVar->FindVar("mat_disable_bloom")->SetInt(config.visuals.effects.removals->get(8));
		CVar->FindVar("r_drawparticles")->SetInt(!config.visuals.effects.removals->get(9));
		CVar->FindVar("func_break_max_pieces")->SetInt(config.visuals.effects.removals->get(10) ? 0 : 15);
		CVar->FindVar("props_break_max_pieces")->SetInt(config.visuals.effects.removals->get(10) ? 0 : 50);
		CVar->FindVar("mat_disable_fancy_blending")->SetInt(config.visuals.effects.removals->get(11));
		CVar->FindVar("mat_antialias")->SetInt(0);
		CVar->FindVar("r_shadows")->SetInt(!config.visuals.effects.removals->get(2));

		NadeWarning->RenderPaths();

		break;
	}
	case FRAME_RENDER_END: {
		if (ctx.update_nightmode) {
			World->Modulation();
			ctx.update_nightmode = false;
		}

		if (ctx.update_remove_blood) {
			World->RemoveBlood();
			ctx.update_remove_blood = false;
		}

		break;
	}
	case FRAME_NET_UPDATE_START:
		Miscellaneous::PreserveKillfeed();
		Lua->OnFrameUpdate();
		break;
	case FRAME_NET_UPDATE_END:
		LagCompensation->OnNetUpdate();
		EnginePrediction->NetUpdate();
		Exploits->NetUpdate();
		ShotManager->OnNetUpdate();

		if (config.visuals.esp.anti_fatality->get())
			WorldESP->AntiFatality();
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		SkinChanger->AgentChanger();
		SkinChanger->MaskChanger();
		SkinChanger->Run();
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		break;
	}

	AnimationSystem->FrameStageNotify(stage);

	oFrameStageNotify(thisptr, edx, stage);

	LUA_CALL_HOOK(LUA_FRAMESTAGE, stage);
}

void __fastcall hkUpdateClientSideAnimation(CBasePlayer* thisptr, void* edx) {
	if (hook_info.update_csa)
		return oUpdateClientSideAnimation(thisptr, edx);
	if (!thisptr->IsPlayer())
		return oUpdateClientSideAnimation(thisptr, edx);

	CCSGOPlayerAnimationState* animstate = thisptr->GetAnimstate();

	if (thisptr == Cheat.LocalPlayer && animstate) {
		animstate->pEntity = nullptr; // do not update animstate
		oUpdateClientSideAnimation(thisptr, edx);
		animstate->pEntity = thisptr;
	}
}

bool __fastcall hkShouldSkipAnimationFrame(CBasePlayer* thisptr, void* edx) {
	if (ClientState->m_nDeltaTick == -1 || !thisptr->IsPlayer() || !thisptr->IsAlive())
		return oShouldSkipAnimationFrame(thisptr, edx);

	if (thisptr != Cheat.LocalPlayer) {
		if (thisptr->m_iTeamNum() == Cheat.LocalPlayer->m_iTeamNum())
			return true; // uwukson: skip updating animations on your teammates (to get more fps)
	}

	return false;
}

bool __fastcall hkShouldInterpolate(CBasePlayer* thisptr, void* edx) {
	if (thisptr != Cheat.LocalPlayer || !Exploits->ShouldCharge())
		return oShouldInterpolate(thisptr, edx);

	AnimationSystem->DisableInterpolationFlags(thisptr);

	return false;
}

void __fastcall hkDoExtraBoneProcessing(CBaseEntity* player, void* edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* contex) {
	return;
}

bool __fastcall hkIsHLTV(IVEngineClient* thisptr, void* edx) {
	static auto oIsHLTV = (tIsHLTV)Hooks::EngineVMT->GetOriginal(93);

	if (hook_info.setup_bones || hook_info.update_csa)
		return true;

	static const auto setup_velocity = Utils::PatternScan("client.dll", "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0");
	static const auto accumulate_layers = Utils::PatternScan("client.dll", "84 C0 75 0D F6 87");
	static const auto reevaluate_anim_lod = Utils::PatternScan("client.dll", "84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 8B B7");

	if (_ReturnAddress() == setup_velocity || _ReturnAddress() == accumulate_layers || _ReturnAddress() == reevaluate_anim_lod)
		return true;

	return oIsHLTV(thisptr, edx);
}

void __fastcall hkBuildTransformations(CBasePlayer* thisptr, void* edx, void* hdr, void* pos, void* q, const void* camera_transform, int bone_mask, void* bone_computed) {
	if (!thisptr || !thisptr->IsPlayer() || !thisptr->IsAlive())
		return oBuildTransformations(thisptr, edx, hdr, pos, q, camera_transform, bone_mask, bone_computed);

	// uwukson: https://github.com/mmirraacclee/airflow-csgo-cheat/blob/main/v1/Airflow/base/hooks/targets/player_hook.cpp#L138

	auto mask_ptr = *(int*)((uintptr_t)thisptr + 0x26B0); // mask ptr

	auto old_jiggle_bones = thisptr->m_isJiggleBonesEnabled();
	thisptr->m_isJiggleBonesEnabled() = false;

	auto old_mask_ptr = mask_ptr;

	if (thisptr != Cheat.LocalPlayer)
		mask_ptr = 0;

	auto& use_new_animstate = thisptr->m_bUseNewAnimstate();

	auto old_use_state = use_new_animstate;
	use_new_animstate = false;

	oBuildTransformations(thisptr, edx, hdr, pos, q, camera_transform, bone_mask, bone_computed);

	use_new_animstate = old_use_state;
	thisptr->m_isJiggleBonesEnabled() = old_jiggle_bones;

	if (thisptr != Cheat.LocalPlayer)
		mask_ptr = old_mask_ptr;
}

bool __fastcall hkSetupBones(CBaseEntity* thisptr, void* edx, matrix3x4_t* pBoneToWorld, int maxBones, int mask, float curTime)
{
	if (hook_info.setup_bones || !thisptr)
		return oSetupBones(thisptr, edx, pBoneToWorld, maxBones, mask, curTime);

	CBasePlayer* ent = reinterpret_cast<CBasePlayer*>(reinterpret_cast<uintptr_t>(thisptr) - 0x4);

	if (!ent->IsPlayer() || !ent->IsAlive())
		return oSetupBones(thisptr, edx, pBoneToWorld, maxBones, mask, curTime);

	if (hook_info.disable_interpolation) {
		if (pBoneToWorld && maxBones != -1)
			ent->CopyBones(pBoneToWorld);
		return true;
	}

	if (ent == Cheat.LocalPlayer) {

		if (ent->m_vecVelocity().Length() < 10)
			ent->GetAnimlayers()[6].m_flWeight = 0.0f; // uwukson: disable lean while we aren't moving

		memcpy(ent->GetCachedBoneData().Base(), AnimationSystem->GetLocalBoneMatrix(), ent->GetCachedBoneData().Count() * sizeof(matrix3x4_t));
		AnimationSystem->CorrectLocalMatrix(ent->GetCachedBoneData().Base(), ent->GetCachedBoneData().Count());

		if (mask & BONE_USED_BY_ATTACHMENT)
			Cheat.LocalPlayer->SetupBones_AttachmentHelper();

		if (pBoneToWorld && maxBones != -1)
			ent->CopyBones(pBoneToWorld);

		return true;
	}

	if (!hook_info.disable_interpolation && (mask & BONE_USED_BY_ATTACHMENT)) {
		AnimationSystem->InterpolateModel(ent, ent->GetCachedBoneData().Base());
		ent->SetAbsOrigin(AnimationSystem->GetInterpolated(ent));
		ent->SetupBones_AttachmentHelper();
	}

	if (pBoneToWorld && maxBones != -1)
		ent->CopyBones(pBoneToWorld);

	return true;
}

void __fastcall hkRunCommand(IPrediction* thisptr, void* edx, CBasePlayer* player, CUserCmd* cmd, IMoveHelper* moveHelper) {
	static auto oRunCommand = (tRunCommand)(Hooks::PredictionVMT->GetOriginal(19));

	if (!player || !cmd || player != Cheat.LocalPlayer)
		return oRunCommand(thisptr, edx, player, cmd, moveHelper);

	if (cmd->tick_count == INT_MAX) {
		cmd->hasbeenpredicted = true;

		return;
	}

	Exploits->AdjustPlayerTimeBase(player->m_nTickBase(), cmd);

	const float backup_velocity_modifier = player->m_flVelocityModifier();

	oRunCommand(thisptr, edx, player, cmd, moveHelper);

	player->m_flVelocityModifier() = backup_velocity_modifier;

	EnginePrediction->RunCommand(cmd);
	Exploits->RunCommand(cmd);

	MoveHelper = moveHelper;
}

void __fastcall hkPhysicsSimulate(CBasePlayer* thisptr, void* edx) {
	C_CommandContext* context = thisptr->GetCommandContext();

	if (context->cmd.tick_count == INT_MAX) {
		auto old_tick = context->cmd.command_number - 1;
		EnginePrediction->RestoreNetvars(old_tick);
		context->needsprocessing = false;
		return;
	}

	if (thisptr != Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !Cheat.LocalPlayer->GetModel() || thisptr->m_nSimulationTick() == GlobalVars->tickcount || !context->needsprocessing)
		return oPhysicsSimulate(thisptr, edx);

	oPhysicsSimulate(thisptr, edx);

	AnimationSystem->UpdateLocalAnimations(&context->cmd);
	EnginePrediction->StoreNetvars(context->cmd.command_number);

}

void __fastcall hkPacketStart(CClientState* thisptr, void* edx, int incoming_sequence, int outgoing_acknowledged) {
	if (!Cheat.InGame || !Cheat.LocalPlayer->IsAlive())
		return oPacketStart(thisptr, edx, incoming_sequence, outgoing_acknowledged);

	ctx.sent_commands.erase(std::ranges::remove_if(ctx.sent_commands, [&](const uint32_t& cmd) { return abs(static_cast<int32_t>(outgoing_acknowledged - cmd)) >= 150; }).begin(), ctx.sent_commands.end());

	// rollback the ack count to what we aimed for.
	auto target_acknowledged = outgoing_acknowledged;
	for (const auto cmd : ctx.sent_commands)
		if (outgoing_acknowledged >= cmd)
			target_acknowledged = cmd;

	oPacketStart(thisptr, edx, incoming_sequence, target_acknowledged);
}

void __fastcall hkPacketEnd(CClientState* thisptr, void* edx) {
	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
		return oPacketEnd(thisptr, edx);

	if (ClientState->m_ClockDriftMgr.m_nServerTick == ClientState->m_nDeltaTick)
		EnginePrediction->RestoreNetvars(ClientState->m_nLastOutgoingCommand);

	oPacketEnd(thisptr, edx);
}

void __fastcall hkClampBonesInBBox(CBasePlayer* thisptr, void* edx, matrix3x4_t* bones, int boneMask) {
	if (config.antiaim.angles.legacy_desync->get() || hook_info.disable_clamp_bones)
		return;

	auto backup_curtime = GlobalVars->curtime;
	if (thisptr == Cheat.LocalPlayer)
		GlobalVars->curtime = EnginePrediction->curtime();

	oClampBonesInBBox(thisptr, edx, bones, boneMask);

	GlobalVars->curtime = backup_curtime;
}

void __cdecl hkCL_Move(float accamulatedExtraSamples, bool bFinalTick) {
	Cheat.LocalPlayer = (CBasePlayer*)EntityList->GetClientEntity(EngineClient->GetLocalPlayer());

	if (!Cheat.LocalPlayer->IsAlive()) {
		oCL_Move(accamulatedExtraSamples, bFinalTick);
		ctx.sent_commands.emplace_back(ClientState->m_nLastOutgoingCommand);
		return;
	}

	PingReducer->ReadPackets(bFinalTick);

	Exploits->Run();

	if (Exploits->ShouldCharge()) {
		ctx.tickbase_shift++;
		ctx.shifted_last_tick++;

		Exploits->charged_command = ClientState->m_nLastOutgoingCommand;
		Exploits->force_charge = false;

		return;
	}

	EnginePrediction->Update();
	oCL_Move(accamulatedExtraSamples, bFinalTick);

	Exploits->HandleTeleport(oCL_Move);

	if (ClientState->m_nChokedCommands == 0) {
		ctx.sent_commands.emplace_back(ClientState->m_nLastOutgoingCommand);
	}
	else {
		auto net_channel = ClientState->m_NetChannel;

		if (net_channel) {
			net_channel->m_nChokedPackets = 0;
			net_channel->m_nOutSequenceNr--;
			net_channel->SendDatagram();
		}
	}
}

bool __fastcall hkSendNetMsg(INetChannel* thisptr, void* edx, INetMessage& msg, bool bForceReliable, bool bVoice) {
	if (msg.GetType() == 14) // Return and don't send messsage if its FileCRCCheck
		return true;

	if (msg.GetGroup() == 9) // Fix lag when transmitting voice and fakelagging
		bVoice = true;

	return oSendNetMsg(thisptr, edx, msg, bForceReliable, bVoice);
}

void __fastcall hkCalculateView(CBasePlayer* thisptr, void* edx, Vector& eyeOrigin, QAngle& eyeAngle, float& z_near, float& z_far, float& fov) {
	if (!thisptr || thisptr != Cheat.LocalPlayer)
		return oCalculateView(thisptr, edx, eyeOrigin, eyeAngle, z_near, z_far, fov);

	const bool backup = thisptr->m_bUseNewAnimstate();

	thisptr->m_bUseNewAnimstate() = false;
	oCalculateView(thisptr, edx, eyeOrigin, eyeAngle, z_near, z_far, fov);
	thisptr->m_bUseNewAnimstate() = backup;
}

void __fastcall hkRenderSmokeOverlay(void* thisptr, void* edx, bool bPreViewModel) {
	if (!config.visuals.effects.removals->get(3))
		oRenderSmokeOverlay(thisptr, edx, bPreViewModel);
}

void __fastcall hkProcessMovement(IGameMovement* thisptr, void* edx, CBasePlayer* player, CMoveData* mv) {
	mv->bGameCodeMovedPlayer = false;
	oProcessMovement(thisptr, edx, player, mv);
}

int __fastcall hkLogDirect(void* loggingSystem, void* edx, int channel, int serverity, Color color, const char* text) {
	if (hook_info.console_log)
		return oLogDirect(loggingSystem, edx, channel, serverity, color, text);

	if (!config.misc.miscellaneous.filter_console->get())
		return oLogDirect(loggingSystem, edx, channel, serverity, color, text);

	return 0;
}

CNewParticleEffect* hkCreateNewParticleEffect(void* pDef, CBaseEntity* pOwner, Vector const& vecAggregatePosition, const char* pDebugName, int nSplitScreenUser) {
	CNewParticleEffect* result;

	__asm {
		mov edx, pDef
		push nSplitScreenUser
		push pDebugName
		push vecAggregatePosition
		push pOwner
		call oCreateNewParticleEffect
		mov result, eax
	}

	if (IEFFECTS::bCaptureEffect)
		IEFFECTS::pCapturedEffect = result;

	return result;
}

__declspec(naked) void hkCreateNewParticleEffect_proxy() {
	__asm
	{
		push edx
		call hkCreateNewParticleEffect
		pop edx
		retn
	}
}

bool __fastcall hkSVCMsg_VoiceData(CClientState* clientstate, void* edx, const CSVCMsg_VoiceData& msg) {
	if (NetMessages->OnVoiceDataRecieved(msg))
		return true;

	return oSVCMsg_VoiceData(clientstate, edx, msg);
}

void __stdcall hkDrawStaticProps(void* thisptr, IClientRenderable** pProps, const void* pInstances, int count, bool bShadowDepth, bool drawVCollideWireframe) {
	hook_info.in_draw_static_props = true;
	oDrawStaticProps(thisptr, pProps, pInstances, count, bShadowDepth, drawVCollideWireframe);
	hook_info.in_draw_static_props = false;
}

bool __fastcall hkShouldDrawViewModel(void* thisptr, void* edx) {
	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || config.visuals.effects.viewmodel_scope_alpha->get() == 0)
		return oShouldDrawViewModel(thisptr, edx);

	return true;
}

void __fastcall hkPerformScreenOverlay(void* viewrender, void* edx, int x, int y, int w, int h) {
	if (config.misc.miscellaneous.ad_block->get())
		return;

	oPerformScreenOverlay(viewrender, edx, x, y, w, h);
}

int __fastcall hkListLeavesInBox(void* ecx, void* edx, const Vector& mins, const Vector& maxs, unsigned int* list, int size) {
	static void* insert_into_tree = Utils::PatternScan("client.dll", "56 52 FF 50 18", 0x5);

	if (_ReturnAddress() != insert_into_tree)
		return oListLeavesInBox(ecx, edx, mins, maxs, list, size);

	// get current renderable info from stack ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1470 )
	auto info = *(RenderableInfo_t**)((uintptr_t)_AddressOfReturnAddress() + 0x14);
	if (!info || !info->m_pRenderable)
		return oListLeavesInBox(ecx, edx, mins, maxs, list, size);

	// check if disabling occulusion for players ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1491 )
	auto base_entity = info->m_pRenderable->GetIClientUnknown()->GetBaseEntity();
	if (!base_entity || !base_entity->IsPlayer())
		return oListLeavesInBox(ecx, edx, mins, maxs, list, size);

	// fix render order, force translucent group ( https://www.unknowncheats.me/forum/2429206-post15.html )
	// AddRenderablesToRenderLists: https://i.imgur.com/hcg0NB5.png ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L2473 )
	info->m_Flags &= ~0x100;
	info->m_bRenderInFastReflection |= 0xC0;

	// extend world space bounds to maximum ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L707 )
	static const Vector map_min = Vector(-16384.0f, -16384.0f, -16384.0f);
	static const Vector map_max = Vector(16384.0f, 16384.0f, 16384.0f);

	if (!config.visuals.chams.disable_model_occlusion->get())
		return oListLeavesInBox(ecx, edx, mins, maxs, list, size);
	return oListLeavesInBox(ecx, edx, map_min, map_max, list, size);
}

void __fastcall hkAddRenderableToList(void* ecx, void* edx, IClientRenderable* pRenderable, bool bRenderWithViewModels, int nType, int nModelType, int nSplitscreenEnabled) {
	auto renderable_addr = (uintptr_t)pRenderable;
	if (!renderable_addr || renderable_addr == 0x4)
		return oAddRenderableToList(ecx, edx, pRenderable, bRenderWithViewModels, nType, nModelType, nSplitscreenEnabled);

	auto entity = (CBaseEntity*)(renderable_addr - 0x4);
	int index = entity->EntIndex();

	if (index < 1 || index > 64)
		return oAddRenderableToList(ecx, edx, pRenderable, bRenderWithViewModels, nType, nModelType, nSplitscreenEnabled);

	if (index == EngineClient->GetLocalPlayer())
		nType = 1;
	else
		nType = 2;

	oAddRenderableToList(ecx, edx, pRenderable, bRenderWithViewModels, nType, nModelType, nSplitscreenEnabled);
}

void __fastcall hkCL_DispatchSound(const SoundInfo_t& snd, void* edx) {
	WorldESP->ProcessSound(snd);
	oCL_DispatchSound(snd, edx);
}

bool __fastcall hkInterpolateViewModel(CBaseViewModel* vm, void* edx, float curTime) {
	if (EntityList->GetClientEntityFromHandle(vm->m_hOwner()) != Cheat.LocalPlayer)
		return oInterpolateViewModel(vm, edx, curTime);

	auto& pred_tick = Cheat.LocalPlayer->m_nFinalPredictedTick();
	auto backup_pred_tick = pred_tick;
	auto backup_lerp_amt = GlobalVars->interpolation_amount;

	pred_tick = Exploits->GetInterpolateTick();

	if (Exploits->ShouldCharge())
		GlobalVars->interpolation_amount = 1.f;

	auto result = oInterpolateViewModel(vm, edx, curTime);

	pred_tick = backup_pred_tick;

	GlobalVars->interpolation_amount = backup_lerp_amt;

	return result;
}

bool __fastcall hkInterpolatePlayer(CBasePlayer* pl, void* edx, float curTime) {
	if (pl != Cheat.LocalPlayer || !pl)
		return oInterpolatePlayer(pl, edx, curTime);

	curTime = GlobalVars->curtime;
	auto result = oInterpolatePlayer(pl, edx, curTime);
	curTime = EnginePrediction->curtime();

	return oInterpolatePlayer(pl, edx, curTime);
}

void __fastcall hkCalcViewModel(CBaseViewModel* vm, void* edx, CBasePlayer* player, const Vector& eyePosition, QAngle& eyeAngles) {
	if (!Input->m_fCameraInThirdPerson && Cheat.LocalPlayer->IsAlive() && config.visuals.effects.removals->get(7)) {
		eyeAngles -= Cheat.LocalPlayer->m_aimPunchAngle() * 0.9f;
		eyeAngles -= Cheat.LocalPlayer->m_viewPunchAngle();
	}

	if (ctx.fake_duck && Cheat.LocalPlayer == player)
		return oCalcViewModel(vm, edx, player, player->GetAbsOrigin() + Vector(0, 0, 64), eyeAngles);

	oCalcViewModel(vm, edx, player, eyePosition, eyeAngles);
}

void __fastcall hkResetLatched(CBasePlayer* thisptr, void* edx) {
	if (!Cheat.LocalPlayer || thisptr != Cheat.LocalPlayer || EnginePrediction->HasPredictionErrors())
		return oResetLatched(thisptr, edx);
}

//https://www.unknowncheats.me/forum/counterstrike-global-offensive/501277-fix-white-chams-office-dust.html
void __fastcall hkGetExposureRange(float* min, float* max) {
	*min = 1.f;
	*max = 1.f;

	oGetExposureRange(min, max);
}

void __fastcall hkEstimateAbsVelocity(CBaseEntity* ent, void* edx, Vector& vel) {
	if (ent && ent->IsPlayer()) {
		auto pl = reinterpret_cast<CBasePlayer*>(ent);
		if (pl->IsTeammate())
			return oEstimateAbsVelocity(ent, edx, vel);

		//https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/client/c_baseentity.cpp#L6356
		//vel = pl->m_vecVelocity(); // uwukson: we must use this if want rebuild animation
		vel = pl->m_vecAbsVelocity();

		return;
	}

	oEstimateAbsVelocity(ent, edx, vel);
}

float __fastcall hkGetFOV(CBasePlayer* thisptr, void* edx) {
	if (Cheat.LocalPlayer != thisptr)
		return oGetFOV(thisptr, edx);

	float backup_lerp = GlobalVars->interpolation_amount;

	if (Exploits->ShouldCharge())
		GlobalVars->interpolation_amount = 0.f;

	float result = oGetFOV(thisptr, edx);

	GlobalVars->interpolation_amount = backup_lerp;

	return result;
}

bool __fastcall hkIsConnected(IVEngineClient* thisptr, void* edx) {
	static auto inventory_access = Utils::PatternScan("client.dll", "84 C0 75 05 B0");

	if (_ReturnAddress() == inventory_access)
		return false;

	return oIsConnected(thisptr, edx);
}

void __fastcall hkReadPackets(bool final_tick) {
	if (!PingReducer->AllowReadPackets())
		return;

	oReadPackets(final_tick);
	PingSpike->OnPacketStart();
}

void __fastcall hkClientCmd_Unrestricted(IVEngineClient* engineClient, void* edx, const char* cmd, bool a2) {
	LUA_CALL_HOOK(LUA_CONSOLE_INPUT, std::string(cmd));

	oClientCmd_Unrestricted(engineClient, edx, cmd, a2);
}

void hkUpdateBeam(Beam_t* beam, void* pcbeam) { // TODO: make asm proxy
	float frametime;
	__asm movss frametime, xmm2;
	Vector attachments[10];
	if (beam->type == TE_BEAMSPLINE)
		memcpy(attachments, beam->attachemnt, beam->numAttachment * sizeof(Vector));

	__asm {
		movss xmm2, frametime
		push pcbeam
		push beam
		call oUpdateBeam
	}

	if (beam->type == TE_BEAMSPLINE)
		memcpy(beam->attachemnt, attachments, beam->numAttachment * sizeof(Vector));
}

bool __fastcall hkNETMsg_Tick(CClientState* state, void* edx, const CNETMsg_Tick& msg) {
	Exploits->UpdateServerClock(msg.tick);

	return oNETMsg_Tick(state, edx, msg);
}

void __fastcall hkShutDown(uintptr_t ecx, uintptr_t edx) {
	Interfaces::Panic = true;
	oShutDown(ecx, edx);

	TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
}

int __fastcall hkSendDatargam(INetChannel* nc, void* edx, void* datagram) {
	int backup_seq = nc->m_nInSequenceNr;
	int backup_reliable = nc->m_iInReliableState;

	PingSpike->OnSendDatagram();

	int result = oSendDatagram(nc, edx, datagram);

	nc->m_nInSequenceNr = backup_seq;
	nc->m_iInReliableState = backup_reliable;

	return result;
}

void __fastcall hkProcessPacket(INetChannel* ecx, void* edx, void* packet, bool header) {
	if (!ClientState->m_NetChannel || ClientState->m_nSignonState != SIGNONSTATE_FULL)
		return oProcessPacket(ecx, edx, packet, header);

	oProcessPacket(ecx, edx, packet, header);

	for (auto it = ClientState->m_pEvents; it; it = it->next) { // uwukson: EventInfo
		if (!it->class_id)
			continue;

		// uwukson: set all delays to instant
		it->fire_delay = 0.f;
	}

	EngineClient->FireEvents();
}

void __fastcall hkEmitSound(IEngineSound* thisptr, uint32_t edx, void* filter, int ent_index, int channel, const char* sound_entry, unsigned int sound_entry_hash,
	const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const Vector* origin, const Vector* direction,
	void* vec_origins, bool update_positions, float sound_time, int speaker_entity, int test) {
	static auto original = (void(__thiscall*)(IEngineSound*, uint32_t, void*, int, int, const char*, unsigned int, const char*, float, float, int, int, int, const Vector*, const Vector*, void*, bool, float, int, int))Hooks::EngineVMT->GetOriginal(5);

	if (std::strstr(sound_entry, "null"))
		flags |= ((1 << 2) | (1 << 5));

	if (Prediction->bInPrediction)
	{
		flags |= 1 << 2;
		return;
	}

	original(thisptr, edx, filter, ent_index, channel, sound_entry, sound_entry_hash, sample, volume, attenuation, seed, flags,
		pitch, origin, direction, vec_origins, update_positions, sound_time, speaker_entity, test);
}

int __fastcall hkProcessInterpolatedList() {
	static auto allow_extrapolation = *(bool**)(Utils::PatternScan("client.dll", "A2 ? ? ? ? 8B 45 E8", 0x1));

	if (allow_extrapolation)
		*allow_extrapolation = false;

	return oProcessInterpolatedList();
}

void Hooks::Initialize() {
	oWndProc = (WNDPROC)(SetWindowLongPtr(FindWindowA("Valve001", nullptr), GWL_WNDPROC, (LONG_PTR)hkWndProc));

	WorldESP->RegisterCallback();
	Chams->LoadChams();
	SkinChanger->LoadKnifeModels();
	Ragebot->CreateThreads();
	GrenadeWarning::Setup();

	DirectXDeviceVMT = new VMT(DirectXDevice);
	SurfaceVMT = new VMT(Surface);
	ClientModeVMT = new VMT(ClientMode);
	PanelVMT = new VMT(VPanel);
	EngineVMT = new VMT(EngineClient);
	ModelRenderVMT = new VMT(ModelRender);
	ClientVMT = new VMT(Client);
	PredictionVMT = new VMT(Prediction);
	ModelCacheVMT = new VMT(MDLCache);
	KeyValuesVMT = new VMT(KeyValuesSystem);

	// vmt hooking for directx doesnt work for some reason
	oPresent = HookFunction<tPresent>(Utils::PatternScan("gameoverlayrenderer.dll", "55 8B EC 83 EC 4C 53"), hkPresent);
	//oReset = HookFunction<tReset>(Utils::PatternScan("d3d9.dll", "8B FF 55 8B EC 83 E4 F8 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 53 8B 5D 08 8B CB"), hkReset);

	while (!Menu->IsInitialized())
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	DirectXDeviceVMT->Hook(16, hkReset);
	SurfaceVMT->Hook(67, hkLockCursor);
	ClientModeVMT->Hook(18, hkOverrideView);
	ClientModeVMT->Hook(44, hkDoPostScreenEffects);
	PanelVMT->Hook(41, hkPaintTraverse);
	EngineVMT->Hook(5, hkEmitSound);
	EngineVMT->Hook(90, hkIsPaused);
	EngineVMT->Hook(93, hkIsHLTV);
	ModelRenderVMT->Hook(21, hkDrawModelExecute);
	ClientVMT->Hook(37, hkFrameStageNotify);
	ClientVMT->Hook(11, hkHudUpdate);
	ClientVMT->Hook(22, hkCHLCCreateMove);
	//ModelCacheVMT->Hook(10 ,hkFindMdl);
	ClientVMT->Hook(6, hkLevelInitPostEntity);
	ClientVMT->Hook(7, hkLevelShutdown);
	PredictionVMT->Hook(19, hkRunCommand);
	KeyValuesVMT->Hook(2, hkAllocKeyValuesMemory);
	ClientVMT->Hook(4, hkShutDown);

	oUpdateClientSideAnimation = HookFunction<tUpdateClientSideAnimation>(Utils::PatternScan("client.dll", "55 8B EC 51 56 8B F1 80 BE ? ? ? ? ? 74"), hkUpdateClientSideAnimation);
	oDoExtraBoneProcessing = HookFunction<tDoExtraBoneProcessing>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 57 89 74 24 1C"), hkDoExtraBoneProcessing);
	oShouldSkipAnimationFrame = HookFunction<tShouldSkipAnimationFrame>(Utils::PatternScan("client.dll", "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02"), hkShouldSkipAnimationFrame);
	oBuildTransformations = HookFunction<tBuildTransformations>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 F0 81 ? ? ? ? ? 56 57 8B F9 8B"), hkBuildTransformations);
	oSetupBones = HookFunction<tSetupBones>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57"), hkSetupBones);
	oCL_Move = HookFunction<tCL_Move>(Utils::PatternScan("engine.dll", "55 8B EC 81 EC ? ? ? ? 53 56 8A F9 F3 0F 11 45 ? 8B 4D 04"), hkCL_Move);
	oPhysicsSimulate = HookFunction<tPhysicsSimulate>(Utils::PatternScan("client.dll", "56 8B F1 8B 8E ? ? ? ? 83 F9 FF 74 23 0F B7 C1 C1 E0 04 05 ? ? ? ?"), hkPhysicsSimulate);
	oClampBonesInBBox = HookFunction<tClampBonesInBBox>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 83 EC 70 56 57 8B F9 89 7C 24 38"), hkClampBonesInBBox);
	oCalculateView = HookFunction<tCalculateView>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 14 53 56 57 FF 75 18"), hkCalculateView);
	oSendNetMsg = HookFunction<tSendNetMsg>(Utils::PatternScan("engine.dll", "F1 8B 4D 04 E8 ? ? ? ? 8B 86 ? ? ? ? 85 C0 74 24 48 83 F8 02 77 2C 83 BE ? ? ? ? ? 8D 8E ? ? ? ? 74 06 32 C0 84 C0", -0x8), hkSendNetMsg);
	oRenderSmokeOverlay = HookFunction<tRenderSmokeOverlay>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 30 80 7D 08 00"), hkRenderSmokeOverlay);
	oShouldInterpolate = HookFunction<tShouldInterpolate>(Utils::PatternScan("client.dll", "56 8B F1 E8 ? ? ? ? 3B F0"), hkShouldInterpolate);
	oPacketStart = HookFunction<tPacketStart>(Utils::PatternScan("engine.dll", "55 8B EC 8B 45 08 89 81 ? ? ? ? 8B 45 0C 89 81 ? ? ? ? 5D C2 08 00 CC CC CC CC CC CC CC 56"), hkPacketStart);
	oPacketEnd = HookFunction<tPacketEnd>(Utils::PatternScan("engine.dll", "56 8B F1 E8 ? ? ? ? 8B 8E ? ? ? ? 3B 8E ? ? ? ? 75 34"), hkPacketEnd);
	//oFX_FireBullets = HookFunction<tFX_FireBullets>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 C0 F3 0F 10 45"), hkFX_FireBullets);
	oProcessMovement = HookFunction<tProcessMovement>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 C0 83 EC 38 A1 ? ? ? ?"), hkProcessMovement);
	oLogDirect = HookFunction<tLogDirect>(Utils::PatternScan("tier0.dll", "55 8B EC 83 E4 F8 8B 45 08 83 EC 14 53 56 8B F1 57 85 C0 0F 88 ? ? ? ?"), hkLogDirect);
	oSetSignonState = HookFunction<tSetSignonState>(Utils::PatternScan("engine.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 57 FF 75 10"), hkSetSignonState);
	oCreateNewParticleEffect = HookFunction<tCreateNewParticleEffect>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 0C 53 56 8B F2 89 75 F8 57"), hkCreateNewParticleEffect_proxy);
	oSVCMsg_VoiceData = HookFunction<tSVCMsg_VoiceData>(Utils::PatternScan("engine.dll", "55 8B EC 83 E4 F8 A1 ? ? ? ? 81 EC ? ? ? ? 53 56 8B F1 B9 ? ? ? ? 57 FF 50 34 8B 7D 08 85 C0 74 13 8B 47 08 40 50"), hkSVCMsg_VoiceData);
	oDrawStaticProps = HookFunction<tDrawStaticProps>(Utils::PatternScan("engine.dll", "55 8B EC 56 57 8B F9 8B 0D ? ? ? ? 8B B1 ? ? ? ? 85 F6 74 16 6A 04 6A 00 68"), hkDrawStaticProps);
	oShouldDrawViewModel = HookFunction<tShouldDrawViewModel>(Utils::PatternScan("client.dll", "55 8B EC 51 57 E8"), hkShouldDrawViewModel);
	oPerformScreenOverlay = HookFunction<tPerformScreenOverlay>(Utils::PatternScan("client.dll", "55 8B EC 51 A1 ? ? ? ? 53 56 8B D9 B9 ? ? ? ? 57 89 5D FC FF 50 34 85 C0 75 36"), hkPerformScreenOverlay);
	oListLeavesInBox = HookFunction<tListLeavesInBox>(Utils::PatternScan("engine.dll", "55 8B EC 83 EC 18 8B 4D 0C"), hkListLeavesInBox);
	oCL_DispatchSound = HookFunction<tCL_DispatchSound>(Utils::PatternScan("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 8B F1 8D 4D 98 E8"), hkCL_DispatchSound);
	oInterpolateViewModel = HookFunction<tInterpolateViewModel>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 57 83 BE"), hkInterpolateViewModel);
	oCalcViewModel = HookFunction<tCalcViewModel>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 58 56 57"), hkCalcViewModel);
	oResetLatched = HookFunction<tResetLatched>(Utils::PatternScan("client.dll", "56 8B F1 57 8B BE ? ? ? ? 85 FF 74 ? 8B CF E8 ? ? ? ? 68"), hkResetLatched);
	oGetExposureRange = HookFunction<tGetExposureRange>(Utils::PatternScan("client.dll", "55 8B EC 51 80 3D ? ? ? ? ? 0F 57"), hkGetExposureRange);
	oEstimateAbsVelocity = HookFunction<tEstimateAbsVelocity>(Utils::PatternScan("client.dll", "55 8B EC 83 E4 ? 83 EC ? 56 8B F1 85 F6 74 ? 8B 06 8B 80 ? ? ? ? FF D0 84 C0 74 ? 8A 86"), hkEstimateAbsVelocity);
	//oInterpolatePlayer = HookFunction<tInterpolatePlayer>(Utils::PatternScan("client.dll", "55 8B EC 83 EC ? 56 8B F1 83 BE ? ? ? ? ? 0F 85"), hkInterpolatePlayer);
	oGetFOV = HookFunction<tGetFOV>(Utils::PatternScan("client.dll", "55 8B EC 83 EC ? 56 8B F1 57 8B 06 FF 90 ? ? ? ? 83 F8"), hkGetFOV);
	oIsConnected = HookFunction<tIsConnected>(Utils::PatternScan("engine.dll", "A1 ? ? ? ? 83 B8 ? ? ? ? ? 0F 9D C0 C3 55"), hkIsConnected);
	oReadPackets = HookFunction<tReadPackets>(Utils::PatternScan("engine.dll", "53 8A D9 8B 0D ? ? ? ? 56 57 8B B9"), hkReadPackets);
	oAddRenderableToList = HookFunction<tAddRenderableToList>(Utils::PatternScan("client.dll", "55 8B EC 56 8B 75 08 57 FF 75 18"), hkAddRenderableToList);
	oClientCmd_Unrestricted = HookFunction<tClientCmd_Unrestricted>(Utils::PatternScan("engine.dll", "55 8B EC 8B 0D ? ? ? ? 81 F9 ? ? ? ? 75 ? F3 0F 10 05 ? ? ? ? 0F 2E 05 ? ? ? ? 8B 0D ? ? ? ? 9F F6 C4 ? 7A ? 39 0D ? ? ? ? 75 ? A1 ? ? ? ? 33 05 ? ? ? ? A9 ? ? ? ? 74 ? 8B 15 ? ? ? ? 85 D2 74 ? 8B 02 8B CA 68 ? ? ? ? FF 90 ? ? ? ? 8B 0D ? ? ? ? 81 F1 ? ? ? ? EB ? 8B 01 FF 50 ? 8B C8 A1"), hkClientCmd_Unrestricted);
	oUpdateBeam = HookFunction<tUpdateBeam>(Utils::PatternScan("client.dll", "53 8B DC 83 EC ? 83 E4 ? 83 C4 ? 55 8B 6B ? 89 6C 24 ? 8B EC 83 EC ? 56 8B 73 ? 0F 28 C2"), hkUpdateBeam);
	oNETMsg_Tick = HookFunction<tNETMsg_Tick>(Utils::PatternScan("engine.dll", "55 8B EC 53 56 8B F1 8B 0D ? ? ? ? 57"), hkNETMsg_Tick);
	oSendDatagram = HookFunction<tSendDatagram>(Utils::PatternScan("engine.dll", "55 8B EC 83 E4 ? B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 ? 83 BF"), hkSendDatargam);
	oProcessPacket = HookFunction<tProcessPacket>(Utils::PatternScan("engine.dll", "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 56 57 8B 7D 08 8B D9"), hkProcessPacket);

	oProcessInterpolatedList = HookFunction<tProcessInterpolatedList>(Utils::PatternScan("client.dll", "53 0F B7 1D ? ? ? ? 56"), hkProcessInterpolatedList);

	EventListner->Register();

	Memory->BytePatch(Utils::PatternScan("client.dll", "75 30 38 87"), { 0xEB }); // CameraThink sv_cheats check skip
	Memory->BytePatch(Utils::PatternScan("engine.dll", "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D"), { 0xB8, 64 }); // Bypass 15 tick limit (62)

	// TODO: check this
	//Memory->BytePatch(Utils::PatternScan("engine.dll", "C7 45 ? ? ? ? ? 89 55 ? 3B F3"), { 0xC7, 0x45, 0xB4, 0x00 }); // cmdbackup = 0
	//Memory->BytePatch(Utils::PatternScan("engine.dll", "4E C7 45 ? ? ? ? ? 89 55"), { 0x46 }); // fix for cmdbackup
}

void Hooks::End() {
	SetWindowLongPtr(FindWindowA("Valve001", nullptr), GWL_WNDPROC, (LONG_PTR)oWndProc);

	EventListner->Unregister();
	Ragebot->TerminateThreads();
	Lua->UnloadAll();
	Menu->Release();

	DirectXDeviceVMT->UnHook(16);
	SurfaceVMT->UnHook(67);
	ClientModeVMT->UnHook(18);
	ClientModeVMT->UnHook(44);
	PanelVMT->UnHook(41);
	EngineVMT->UnHook(5);
	EngineVMT->UnHook(90);
	EngineVMT->UnHook(93);
	ModelRenderVMT->UnHook(21);
	ClientVMT->UnHook(37);
	ClientVMT->UnHook(11);
	ClientVMT->UnHook(22);
	ClientVMT->UnHook(6);
	ClientVMT->UnHook(7);
	PredictionVMT->UnHook(19);
	//ClientVMT->UnHook(40);
	KeyValuesVMT->UnHook(2);
	ModelCacheVMT->UnHook(10);
	ClientVMT->UnHook(4);

	RemoveHook(oPresent, hkPresent);
	//RemoveHook(oReset, hkReset);
	RemoveHook(oUpdateClientSideAnimation, hkUpdateClientSideAnimation);
	RemoveHook(oDoExtraBoneProcessing, hkDoExtraBoneProcessing);
	RemoveHook(oShouldSkipAnimationFrame, hkShouldSkipAnimationFrame);
	RemoveHook(oBuildTransformations, hkBuildTransformations);
	RemoveHook(oSetupBones, hkSetupBones);
	RemoveHook(oCL_Move, hkCL_Move);
	RemoveHook(oPhysicsSimulate, hkPhysicsSimulate);
	RemoveHook(oClampBonesInBBox, hkClampBonesInBBox);
	RemoveHook(oCalculateView, hkCalculateView);
	RemoveHook(oSendNetMsg, hkSendNetMsg);
	RemoveHook(oRenderSmokeOverlay, hkRenderSmokeOverlay);
	RemoveHook(oShouldInterpolate, hkShouldInterpolate);
	RemoveHook(oProcessPacket, hkProcessPacket);
	RemoveHook(oPacketStart, hkPacketStart);
	RemoveHook(oPacketEnd, hkPacketEnd);
	//RemoveHook(oFX_FireBullets, hkFX_FireBullets);
	RemoveHook(oProcessMovement, hkProcessMovement);
	RemoveHook(oLogDirect, hkLogDirect);
	RemoveHook(oSetSignonState, hkSetSignonState);
	RemoveHook(oCreateNewParticleEffect, hkCreateNewParticleEffect_proxy);
	RemoveHook(oSVCMsg_VoiceData, hkSVCMsg_VoiceData);
	RemoveHook(oDrawStaticProps, hkDrawStaticProps);
	RemoveHook(oShouldDrawViewModel, hkShouldDrawViewModel);
	RemoveHook(oPerformScreenOverlay, hkPerformScreenOverlay);
	RemoveHook(oListLeavesInBox, hkListLeavesInBox);
	RemoveHook(oCL_DispatchSound, hkCL_DispatchSound);
	RemoveHook(oInterpolateViewModel, hkInterpolateViewModel);
	RemoveHook(oCalcViewModel, hkCalcViewModel);
	RemoveHook(oResetLatched, hkResetLatched);
	RemoveHook(oGetExposureRange, hkGetExposureRange);
	RemoveHook(oEstimateAbsVelocity, hkEstimateAbsVelocity);
	//RemoveHook(oInterpolatePlayer, hkInterpolatePlayer);
	RemoveHook(oGetFOV, hkGetFOV);
	RemoveHook(oIsConnected, hkIsConnected);
	RemoveHook(oReadPackets, hkReadPackets);
	RemoveHook(oAddRenderableToList, hkAddRenderableToList);
	RemoveHook(oClientCmd_Unrestricted, hkClientCmd_Unrestricted);
	RemoveHook(oUpdateBeam, hkUpdateBeam);
	RemoveHook(oNETMsg_Tick, hkNETMsg_Tick);
	RemoveHook(oProcessInterpolatedList, hkProcessInterpolatedList);
}