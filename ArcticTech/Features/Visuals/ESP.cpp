#include "ESP.h"

#include <vector>
#include <string>
#include <algorithm>

#include "../../SDK/NetMessages.h"
#include "../../SDK/Globals.h"
#include "../../SDK/Misc/CBaseCombatWeapon.h"
#include "../../Utils/Utils.h"
#include "GrenadePrediction.h"
#include "../RageBot/LagCompensation.h"
#include "../RageBot/AnimationSystem.h"
#include "WeaponIcons.h"
#include "../../Utils/Console.h"

inline std::unique_ptr<CWorldESP> WorldESP = std::make_unique<CWorldESP>();

void CWorldESP::Draw() {
	if (!config.visuals.esp.enable->get() || !Cheat.InGame || !Cheat.LocalPlayer) return;

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		UpdatePlayer(i);
		DrawPlayer(i);
	}
}


void ProcessSharedESP(const SharedVoiceData_t* data) {
	SharedESP_t esp = *(SharedESP_t*)data;

	int ent_index = EngineClient->GetPlayerForUserID(esp.m_iPlayer);
	auto& esp_info = WorldESP->GetESPInfo(ent_index);

	CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(ent_index));

	if (!player || player == Cheat.LocalPlayer || !player->m_bDormant())
		return;

	esp_info.m_iActiveWeapon = esp.m_ActiveWeapon;
	esp_info.m_flLastUpdateTime = GlobalVars->curtime;
	esp_info.m_nHealth = esp.m_iHealth;
	esp_info.m_bValid = true;
	player->m_vecOrigin() = Vector(esp.m_vecOrigin.x, esp.m_vecOrigin.y, esp.m_vecOrigin.z);
	player->m_iHealth() = esp.m_iHealth;

	esp_info.m_flLastSharedData = GlobalVars->realtime;
}

void ParseOtherShared(const VoiceDataOther* data) {
	auto fatal_data = reinterpret_cast<const SharedEsp_Fatality*>(data);

	if (fatal_data->identifier != 0x7FFA && fatal_data->identifier != 0x7FFB)
		return;

	if (fatal_data->server_tick == 0xDEADC0DE)
		return;

	int ent_index = EngineClient->GetPlayerForUserID(fatal_data->user_id);
	auto& esp_info = WorldESP->GetESPInfo(ent_index);
	const auto player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(ent_index));

	if (!player || !player->m_bDormant() || !player->IsEnemy())
		return;

	player->m_vecOrigin() = fatal_data->pos;
	esp_info.m_iActiveWeapon = fatal_data->weapon_id + (fatal_data->identifier == 0x7FFB ? 400 : 0);
	esp_info.m_flLastUpdateTime = GlobalVars->curtime;
	esp_info.m_flLastSharedData = GlobalVars->realtime;
}

void CWorldESP::RegisterCallback() {
	NetMessages->AddArcticDataCallback(ProcessSharedESP);
	NetMessages->AddVoiceDataCallback(ParseOtherShared);
}

void CWorldESP::ProcessSound(const SoundInfo_t& sound) {
	if (!config.visuals.esp.dormant->get())
		return;

	if (sound.nEntityIndex == 0 || sound.vOrigin.Zero())
		return;

	CBaseEntity* sound_source = EntityList->GetClientEntity(sound.nEntityIndex);

	if (!sound_source)
		return;

	CBasePlayer* player = nullptr;
	CBasePlayer* moveparent = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(sound_source->moveparent()));
	CBasePlayer* shadow_parent = reinterpret_cast<CBasePlayer*>(sound_source->GetShadowParent());
	CBasePlayer* owner = nullptr;
		
	if (sound_source->IsWeapon()) {
		owner = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(reinterpret_cast<CBaseCombatWeapon*>(sound_source)->m_hOwner()));
		if (!owner)
			owner = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(reinterpret_cast<CBaseCombatWeapon*>(sound_source)->m_hOwnerEntity()));
	}

	if (sound_source->IsPlayer())
		player = reinterpret_cast<CBasePlayer*>(sound_source);
	else if (moveparent && moveparent->IsPlayer())
		player = moveparent;
	else if (owner && owner->IsPlayer())
		player = owner;
	else if (shadow_parent && shadow_parent->IsPlayer())
		player = shadow_parent;
	else
		return;

	if (!player || !player->m_bDormant() || player->IsTeammate() || !player->IsAlive())
		return;

	auto& info = esp_info[player->EntIndex()];

	if (abs(GlobalVars->realtime - info.m_flLastSharedData) < 0.5f)
		return;

	Vector origin = sound.vOrigin;

	auto trace = EngineTrace->TraceHull(origin, origin - Vector(0, 0, 128), Vector(-16, -16, 0), Vector(16, 16, 72), MASK_SOLID, player);
	player->m_vecOrigin() = trace.fraction == 1.f ? trace.startpos : trace.endpos;

	info.m_flLastUpdateTime = GlobalVars->curtime;
	info.m_bValid = true;
}

void CWorldESP::AntiFatality() {
	SharedEsp_Fatality data;
	data.identifier = 0x7FFA;
	data.server_tick = 0xDEADC0DE;
	data.weapon_id = Ssg08;
	data.pos = Vector(12, 12, 13);

	for (int i = 0; i < GlobalVars->max_clients; i++) {
		CBasePlayer* pl = (CBasePlayer*)EntityList->GetClientEntity(i);
		if (!pl)
			continue;
		player_info_t info;
		EngineClient->GetPlayerInfo(i, &info);
		data.user_id = info.userId;
		NetMessages->SendDataRaw((VoiceDataOther*)&data);
	}
}

void CWorldESP::UpdatePlayer(int id) {
	CBasePlayer* player = (CBasePlayer*)EntityList->GetClientEntity(id);
	auto& info = esp_info[id];

	info.player = player;
	if (!player) {
		info.m_bValid = false;
		return;
	}

	if (player->m_iTeamNum() == 1) {
		info.m_bValid = false;
		info.m_nHealth = 0;

		if (player->m_iObserverMode() != OBS_MODE_IN_EYE && player->m_iObserverMode() != OBS_MODE_CHASE)
			return;

		CBasePlayer* spec_pl = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(player->m_hObserverTarget()));
		if (!spec_pl)
			return;

		if (spec_pl->IsTeammate())
			return;

		ESPInfo_t& spec_info = esp_info[spec_pl->EntIndex()];
		spec_info.m_vecOrigin = player->GetAbsOrigin();
		spec_info.m_flLastUpdateTime = GlobalVars->curtime;
		return;
	}

	if (player->IsTeammate() || !player->IsAlive()) {
		info.m_flLastUpdateTime = GlobalVars->curtime - 12.f;
		info.m_bValid = false;
		return;
	}

	info.m_bDormant = player->m_bDormant();

	auto& records = LagCompensation->records(id);

	if (records.empty())
		info.m_bDormant = true;

	if (!info.m_bDormant)
		info.m_nHealth = player->m_iHealth();

	if (!info.m_bDormant) {
		LagRecord* latestRecord = &records.front();

		if (latestRecord->m_flDuckAmout > 0.01f && latestRecord->m_flDuckAmout < 0.9 && player->m_flDuckSpeed() == 8)
			info.m_nFakeDuckTicks++;
		else
			info.m_nFakeDuckTicks = 0;

		info.m_vecOrigin = AnimationSystem->GetInterpolated(player);
		info.m_bFakeDuck = info.m_nFakeDuckTicks > 14;
		info.m_bExploiting = latestRecord->shifting_tickbase;
		info.m_bBreakingLagComp = latestRecord->breaking_lag_comp;
	}
	else {
		if (info.m_nHealth == 0)
			info.m_bValid = false;

		info.m_nFakeDuckTicks = 0;
		info.m_bFakeDuck = false;
		info.m_bExploiting = false;
		info.m_bBreakingLagComp = false;

		if (config.misc.miscellaneous.gamesense_mode->get())
			info.m_vecOrigin = player->m_vecOrigin();
		else
			info.m_vecOrigin.Interpolate(player->m_vecOrigin(), GlobalVars->frametime * 32);
	}

	float playerHeight = player->m_vecMaxs().z;
	if (playerHeight == 0) {
		info.m_bValid = false;
		return;
	}

	Vector2 head = Render->WorldToScreen(info.m_vecOrigin + Vector(0, 0, playerHeight));
	Vector2 feet = Render->WorldToScreen(info.m_vecOrigin);
	int h = feet.y - head.y;
	int w = (h / playerHeight) * 32;
	int bboxCenter = (feet.x + head.x) * 0.5f;
	info.m_BoundingBox[0] = Vector2(bboxCenter - w * 0.5f, head.y);
	info.m_BoundingBox[1] = Vector2(bboxCenter + w * 0.5f, feet.y);

	info.m_bValid = info.m_BoundingBox[0].x > 0 && info.m_BoundingBox[0].y > 0 && info.m_BoundingBox[1].x < Cheat.ScreenSize.x && info.m_BoundingBox[1].y < Cheat.ScreenSize.y;

	if (info.m_bDormant) {
		float unupdatedTime = GlobalVars->curtime - info.m_flLastUpdateTime;

		if (unupdatedTime > 3)
			info.m_flAlpha = max((6 - unupdatedTime) * 0.33f, 0.f);
		else
			info.m_flAlpha = 1.f;

		if (unupdatedTime > 6)
			info.m_bValid = false;
	}
	else {
		info.m_flAlpha = 1.f;
		info.m_flLastUpdateTime = GlobalVars->curtime;
		
		if (config.visuals.other_esp.radar->get())
			player->m_bSpotted() = true;

		CBaseCombatWeapon* weapon = player->GetActiveWeapon();

		if (weapon)
			info.m_iActiveWeapon = weapon->m_iItemDefinitionIndex();
	}
}

void CWorldESP::DrawPlayer(int id) {
	auto& info = esp_info[id];

	if (!info.m_bValid)
		return;

	CBasePlayer* player = info.player;

	if (info.m_bDormant && !config.visuals.esp.dormant->get())
		return;

	if (info.m_BoundingBox[0].Invalid() || info.m_BoundingBox[1].Invalid())
		return;

	DrawBox(info);
	DrawHealth(info);
	DrawName(info);
	DrawFlags(info);
	DrawWeapon(info);
	//SpinningStar(info);
}

void CWorldESP::DrawBox(const ESPInfo_t& info) {
	if (!config.visuals.esp.bounding_box->get())
		return;

	Color clr = config.visuals.esp.box_color->get();
	if (info.m_bDormant)
		clr = config.visuals.esp.dormant_color->get();

	clr.a *= info.m_flAlpha;

	Render->Box(info.m_BoundingBox[0], info.m_BoundingBox[1], clr);
	Render->Box(info.m_BoundingBox[0] - Vector2(1, 1), info.m_BoundingBox[1] + Vector2(1, 1), Color(0, 0, 0, 150 * clr.a / 255));
	Render->Box(info.m_BoundingBox[0] + Vector2(1, 1), info.m_BoundingBox[1] - Vector2(1, 1), Color(0, 0, 0, 150 * clr.a / 255));
}

void CWorldESP::DrawHealth(const ESPInfo_t& info) {
	if (!config.visuals.esp.health_bar->get())
		return;

	static std::unordered_map<int, float> animated_health;

	int player_idx = info.player->EntIndex();

	float& current_health = animated_health[player_idx];
	float target_health = static_cast<float>(info.m_nHealth);

	constexpr float animation_speed = 4.0f;
	current_health = std::lerp(current_health, target_health, GlobalVars->frametime * animation_speed);

	Color clr = Color(0, 255, 0);

	float health = current_health;
	clr.r = std::clamp(health < 50 ? 250 : (100 - health) / 50.f * 250.f, 120.f, 230.f);
	clr.g = std::clamp(health > 50 ? 250 : health / 50.f * 250.f, 120.f, 230.f);
	clr.b = 120;

	if (config.visuals.esp.custom_health->get())
		clr = config.visuals.esp.custom_health_color->get();

	if (info.m_bDormant)
		clr = config.visuals.esp.dormant_color->get();

	clr.alpha_modulatef(info.m_flAlpha);
	float clr_a = clr.a / 255.f;

	int h = info.m_BoundingBox[1].y - info.m_BoundingBox[0].y;
	float health_fraction = std::clamp(1 - current_health / 100.f, 0.f, 1.f);

	float pulse_intensity = 0.0f;
	if (current_health < 25.0f) {
		pulse_intensity = (std::sin(GlobalVars->curtime * 5.0f) + 1.0f) * 0.5f;
	}

	Vector2 health_box_start = info.m_BoundingBox[0] - Vector2(7, 1);
	Vector2 health_box_end(info.m_BoundingBox[0].x - 3, info.m_BoundingBox[1].y + 1);

	// background
	Render->BoxFilled(health_box_start, health_box_end, Color(8, 220 * info.m_flAlpha * clr_a));

	// border
	Render->BoxFilled(health_box_start + Vector2(0, 1), Vector2(health_box_start.x + 1, health_box_end.y - 1), Color(8, 245 * info.m_flAlpha * clr_a));
	Render->BoxFilled(Vector2(health_box_end.x - 1, health_box_start.y + 1), Vector2(health_box_end.x, health_box_end.y - 1), Color(8, 245 * info.m_flAlpha * clr_a));
	Render->BoxFilled(health_box_start, Vector2(health_box_end.x, health_box_start.y + 1), Color(8, 245 * info.m_flAlpha * clr_a));
	Render->BoxFilled(Vector2(health_box_start.x, health_box_end.y - 1), health_box_end, Color(8, 245 * info.m_flAlpha * clr_a));

	Color final_clr = clr;
	if (pulse_intensity > 0.0f) {
		final_clr.a = static_cast<byte>(clr.a * (1.0f + pulse_intensity * 0.3f));
	}

	Render->BoxFilled(health_box_start + Vector2(1, 1 + h * health_fraction), health_box_end - Vector2(1, 1), final_clr);

	float base_dmg = 100;

	if (ctx.weapon_info && (ctx.active_weapon->m_iItemDefinitionIndex() == Ssg08 || ctx.active_weapon->m_iItemDefinitionIndex() == Revolver)) {
		float distance = (Cheat.LocalPlayer->GetAbsOrigin() - info.m_vecOrigin).Q_Length();
		base_dmg = ctx.weapon_info->iDamage * std::powf(ctx.weapon_info->flRangeModifier, (distance * 0.002f));
		info.player->ScaleDamage(HITGROUP_STOMACH, ctx.weapon_info, base_dmg);
	}

	if (current_health < base_dmg) {
		Render->Text(std::to_string(static_cast<int>(current_health)),
			info.m_BoundingBox[0] - Vector2(6.f, -health_fraction * h + 5),
			Color(240, 240, 240, 255 * info.m_flAlpha * clr_a),
			SmallFont,
			TEXT_CENTERED | TEXT_OUTLINED);
	}
}

void CWorldESP::DrawName(const ESPInfo_t& info) {
	if (!config.visuals.esp.name->get())
		return;

	static std::unordered_map<int, float> name_animations;
	int player_index = info.player->EntIndex();
	float& animation = name_animations[player_index];

	animation = min(animation + GlobalVars->frametime * 4.0f, 1.0f);

	Color clr = config.visuals.esp.name_color->get();
	if (info.m_bDormant)
		clr = config.visuals.esp.dormant_color->get();

	clr.a *= info.m_flAlpha * animation;

	float offset = std::sin(GlobalVars->curtime * 2.0f) * 2.0f;
	Vector2 name_pos((info.m_BoundingBox[0].x + info.m_BoundingBox[1].x) * 0.5f,
		info.m_BoundingBox[0].y - 13 + offset);

	Render->Text(info.player->GetName(), name_pos, clr, Verdana, TEXT_CENTERED | TEXT_DROPSHADOW);
}

void CWorldESP::DrawFlags(const ESPInfo_t& info) {
	struct ESPFlag_t {
		std::string flag;
		Color color;

		ESPFlag_t(const std::string& f, Color c) : flag(f), color(c) {}
		ESPFlag_t(const char* f, Color c) : flag(f), color(c) {}
	};

	bool dormant = info.m_bDormant;

	auto& records = LagCompensation->records(info.player->EntIndex());

	LagRecord* record = nullptr;
	if (!records.empty())
		record = &records.front();

	std::vector<ESPFlag_t> flags;

	if (config.visuals.esp.flags->get(0)) {
		std::string str = "";
		if (info.player->m_ArmorValue() > 0)
			str = "K";
		if (info.player->m_bHasHelmet())
			str = "HK";
		if (!str.empty())
			flags.emplace_back(str, Color(215, 255 * info.m_flAlpha));
	}

	bool shifting = record && record->shifting_tickbase;

	if (config.visuals.esp.flags->get(4) && info.player->EntIndex() == PlayerResource->m_iPlayerC4())
		flags.emplace_back("BOMB", Color(210, 0, 40, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(1) && info.player->m_bIsScoped() && !dormant)
		flags.emplace_back("ZOOM", Color(120, 160, 200, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(3) && record && (shifting || (record->m_flSimulationTime < record->m_flServerTime - TICKS_TO_TIME(7.f))) && !dormant)
		flags.emplace_back("X", shifting ? Color(210, 0, 40, 255 * info.m_flAlpha) : Color(215, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(2) && info.m_bFakeDuck && !dormant)
		flags.emplace_back("FD", Color(215, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(5) && info.m_bBreakingLagComp && !dormant)
		flags.emplace_back("LC", Color(210, 0, 40, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(7) && info.m_bHit && Cheat.LocalPlayer->IsAlive())
		flags.emplace_back("HIT", Color(215, 255 * info.m_flAlpha));

	if (config.visuals.esp.flags->get(8) && !records.empty()) {
		bool onshot = false;
		for (auto it = records.rbegin(); it != records.rend(); it++) {
			auto record = &*it;
			if (!LagCompensation->ValidRecord(record)) {
				if (record->breaking_lag_comp || record->invalid)
					break;

				continue;
			}

			if (record->shooting) {
				onshot = true;
				break;
			}
		}

		if (onshot)
			flags.emplace_back("O", Color(215, 255 * info.m_flAlpha));
	}

	if (config.visuals.esp.flags->get(6) && record && !dormant && Cheat.LocalPlayer->IsAlive()) {
		std::string rtype;
		switch (record->resolver_data.resolver_type)
		{
		case ResolverType::NONE:
			rtype = "N";
			break;
		case ResolverType::FREESTAND:
			rtype = "F";
			break;
		case ResolverType::LOGIC:
			rtype = "L";
			break;
		case ResolverType::ANIM:
			rtype = "A";
			break;
		case ResolverType::BRUTEFORCE:
			rtype = "B";
			break;
		case ResolverType::MEMORY:
			rtype = "M";
			break;
		case ResolverType::DEFAULT:
			rtype = "D";
			break;
		default:
			break;
		}
		flags.emplace_back(std::format("{}[{}]", rtype, (int)(record->resolver_data.side * info.player->GetMaxDesyncDelta())), Color(210, 255 * info.m_flAlpha));
	}

	int line_offset = 0;
	const Color dormant_color = config.visuals.esp.dormant_color->get().alpha_modulatef(info.m_flAlpha);
	for (const auto& flag : flags) {
		Render->Text(flag.flag, Vector2(info.m_BoundingBox[1].x + 3, info.m_BoundingBox[0].y + line_offset), dormant ? dormant_color : flag.color, SmallFont, TEXT_OUTLINED);
		line_offset += 10;
	}

	//if (record) {
	//	Render->Line(Render->WorldToScreen(record->m_vecOrigin), Render->WorldToScreen(record->m_vecOrigin + record->m_vecVelocity), Color());
	//	Render->Text(std::to_string(record->m_vecVelocity.Q_Length()), Render->WorldToScreen(record->m_vecOrigin + record->m_vecVelocity), Color(), SmallFont, TEXT_OUTLINED);
	//}
}

void CWorldESP::DrawWeapon(const ESPInfo_t& info) {
	if (!config.visuals.esp.weapon_text->get() && !config.visuals.esp.weapon_icon->get())
		return;

	if (!info.m_iActiveWeapon)
		return;

	auto weapon_data = WeaponSystem->GetWeaponData(info.m_iActiveWeapon);

	if (!weapon_data)
		return;

	std::string weap = info.player->GetActiveWeapon()->GetName(weapon_data);
	int current_offset = 0;
	if (config.visuals.esp.weapon_text->get()) {
		Render->Text(weap, Vector2((info.m_BoundingBox[0].x + info.m_BoundingBox[1].x) / 2, info.m_BoundingBox[1].y + 2), info.m_bDormant ? config.visuals.esp.dormant_color->get().alpha_modulatef(info.m_flAlpha) : config.visuals.esp.weapon_text_color->get().alpha_modulatef(info.m_flAlpha), SmallFont, TEXT_OUTLINED | TEXT_CENTERED);
		current_offset += 11;
	}

	if (config.visuals.esp.weapon_icon->get()) {
		auto& wicon = WeaponIcons->GetIcon(info.m_iActiveWeapon);

		if (wicon.texture) {
			Render->Image(wicon, Vector2((info.m_BoundingBox[0].x + info.m_BoundingBox[1].x) / 2 - wicon.width * 0.5f + 1, info.m_BoundingBox[1].y + 2 + current_offset + 1), Color(8, (int)(info.m_flAlpha * 125)));
			Render->Image(wicon, Vector2((info.m_BoundingBox[0].x + info.m_BoundingBox[1].x) / 2 - wicon.width * 0.5f, info.m_BoundingBox[1].y + 2 + current_offset), info.m_bDormant ? config.visuals.esp.dormant_color->get().alpha_modulatef(info.m_flAlpha) : config.visuals.esp.weapon_icon_color->get().alpha_modulatef(info.m_flAlpha));
		}
	}
}

void CWorldESP::OtherESP() {
	if (!Cheat.InGame) 
		return;

	//RenderDebugMessages();
	
	for (int i = 0; i < EntityList->GetHighestEntityIndex(); i++) {
		CBaseEntity* ent = EntityList->GetClientEntity(i);

		if (!ent || ent->m_bDormant())
			continue;

		auto classId = ent->GetClientClass();

		if (!classId)
			continue;

		if (classId->m_ClassID == C_BASE_CS_GRENADE_PROJECTILE || classId->m_ClassID == C_MOLOTOV_PROJECTILE || classId->m_ClassID == C_SMOKE_GRENADE_PROJECTILE || classId->m_ClassID == C_DECOY_PROJECTILE || classId->m_ClassID == C_INFERNO)
			DrawGrenade(reinterpret_cast<CBaseGrenade*>(ent), classId);

		if (classId->m_ClassID == C_C4 || classId->m_ClassID == C_PLANTED_C4)
			DrawBomb(ent, classId);
	}
}

void CWorldESP::DrawGrenade(CBaseGrenade* grenade, ClientClass* cl_class) {
	if (!config.visuals.other_esp.grenades->get())
		return;

	if (cl_class->m_ClassID == C_INFERNO) {
		CBasePlayer* owner = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(grenade->m_hOwnerEntity()));
		CBasePlayer* local = EntityList->GetLocalOrSpec();

		if (owner != local && owner->m_iTeamNum() == local->m_iTeamNum() && (!cvars.mp_friendlyfire->GetInt() || cvars.ff_damage_reduction_grenade->GetFloat() == 0.f))
			return;

		Vector2 pos = Render->WorldToScreen(grenade->GetAbsOrigin());

		if (pos.x <= 0.f || pos.y <= 0.f || pos.x >= Cheat.ScreenSize.x || pos.y >= Cheat.ScreenSize.y)
			pos = Render->GetOOF(grenade->GetAbsOrigin()) * (Cheat.ScreenSize * 0.5f - Vector2(50, 50)) + Cheat.ScreenSize * 0.5f;

		Vector camera_pos = local->GetAbsOrigin();

		if (Cheat.LocalPlayer->m_iObserverMode() == OBS_MODE_ROAMING)
			camera_pos = ctx.camera_postion;

		float distance = (camera_pos - grenade->GetAbsOrigin()).Q_Length();

		if (distance > 550)
			return;

		float distance_alpha = std::clamp(1.f - (distance - 500.f) / 50.f, 0.f, 1.f);
		float circle_radius = 29.f - std::clamp((distance - 180.f) / 100.f, 0.f, 6.f);

		float alpha = distance_alpha;
		float end_time = grenade->m_flInfernoSpawnTime() + 7.03125f;

		if (end_time - 0.25f < GlobalVars->curtime)
			alpha *= (end_time - GlobalVars->curtime) * 4.f;
		else if (GlobalVars->curtime - grenade->m_flInfernoSpawnTime() < 0.1667f)
			alpha *= (GlobalVars->curtime - grenade->m_flInfernoSpawnTime()) * 6.f;

		Render->CircleFilled(pos, circle_radius, Color(16, 16, 16, 190 * alpha));
		Render->GlowCircle2(pos, circle_radius - 2, Color(40, 40, 40, 230 * alpha), Color(20, 20, 20, 230 * alpha));
		Render->GlowCircle(pos, circle_radius - 4, Color(255, 50, 50, std::clamp((380 - distance) / 200.f, 0.f, 1.f) * 215 * alpha));

		Render->Image(Resources::Inferno, pos - Vector2(15, 15), Color(255, 255, 255, 230 * alpha));

		return;
	}

	CBasePlayer* thrower = grenade->GetThrower();

	bool thrower_teammate = thrower && thrower->IsTeammate() && (cvars.mp_friendlyfire->GetInt() == 0 || cvars.ff_damage_reduction_grenade->GetFloat() == 0.f) && thrower != Cheat.LocalPlayer;

	int weapId = -1;
	std::string name;
	float alpha = 1.f;

	switch (grenade->GetGrenadeType()) {
	case GRENADE_TYPE_DECOY:
		name = "DECOY";
		break;
	case GRENADE_TYPE_FIRE:
		name = "MOLLY";
		weapId = Molotov;
		break;
	case GRENADE_TYPE_SMOKE:
		name = "SMOKE";
		if (grenade->m_nSmokeEffectTickBegin() > 0)
			alpha = std::clamp((17.5f - TICKS_TO_TIME(GlobalVars->tickcount - grenade->m_nSmokeEffectTickBegin())) / 1.5f, 0.f, 1.f);
		break;
	case GRENADE_TYPE_EXPLOSIVE:
		const model_t* model = grenade->GetModel();
		if (!model)
			break;
		studiohdr_t* studioModel = ModelInfoClient->GetStudioModel(model);
		if (!studioModel)
			break;

		if (!strstr(studioModel->szName, "fraggrenade")) {
			name = "FLASH";
			break;
		}

		if (grenade->m_nExplodeEffectTickBegin() > 0)
			break;

		name = "FRAG";
		weapId = HeGrenade;
		break;
	}

	if (!name.empty()) {
		Vector2 pos = Render->WorldToScreen(grenade->GetAbsOrigin());
		if (!pos.Invalid())
			Render->Text(name, pos + Vector2(0, 8), Color(230, 255 * alpha), SmallFont, TEXT_OUTLINED | TEXT_CENTERED);
	}

	if (weapId == -1)
		return;

	if (config.visuals.other_esp.grenade_proximity_warning->get() && !thrower_teammate)
		NadeWarning->Warning(grenade, weapId);
}

void CWorldESP::DrawBomb(CBaseEntity* ent, ClientClass* cl_class) {
	if (!config.visuals.other_esp.bomb->get())
		return;

	Vector2 pos = Render->WorldToScreen(ent->GetAbsOrigin());

	if (pos.Invalid())
		return;

	pos += Vector2(0, 8);

	if (cl_class->m_ClassID == C_PLANTED_C4) {
		Render->Text("BOMB", pos, config.visuals.other_esp.bomb_color->get(), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);

		CPlantedC4* planted_c4 = reinterpret_cast<CPlantedC4*>(ent);
		float blow_time = planted_c4->m_flC4Blow();
		if (blow_time > GlobalVars->curtime && !planted_c4->m_bBombDefused()) {
			CBasePlayer* bomb_defuser =	reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(planted_c4->m_hBombDefuser()));

			if (bomb_defuser != nullptr) {
				Render->Text(std::format("DEF {:.1f}S", planted_c4->m_flDefuseCountDown() - GlobalVars->curtime), pos + Vector2(0, 10), planted_c4->m_flDefuseCountDown() > planted_c4->m_flC4Blow() ? Color(255, 0, 0) : Color(255), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);
			} else {
				Render->Text(std::format("{:.1f}S", planted_c4->m_flC4Blow() - GlobalVars->curtime), pos + Vector2(0, 10), Color(255), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);
			}
		}
	}
	else {
		CC4* c4 = reinterpret_cast<CC4*>(ent);

		CBasePlayer* owner = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(c4->m_hOwner()));

		if (!GameRules())
			return;

		if (!owner || c4->m_bStartedArming())
			Render->Text("BOMB", pos, config.visuals.other_esp.bomb_color->get(), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);

		if (!c4->m_bStartedArming())
			return;

		float round_end_time = GameRules()->m_fRoundStartTime() + GameRules()->m_iRoundTime();

		Render->Text(std::format("PLANT: {:.1f}S", c4->m_fArmedTime() - GlobalVars->curtime), pos + Vector2(0, 10), c4->m_fArmedTime() > round_end_time ? Color(255, 0, 0) : Color(255), SmallFont, TEXT_CENTERED | TEXT_OUTLINED);
	}
}

void CWorldESP::AddHitmarker(const Vector& position) {
	hit_markers.emplace_back(Hitmarker_t{ position, GlobalVars->realtime });
}

void CWorldESP::AddDamageMarker(const Vector& position, int damage) {
	damage_markers.emplace_back(DamageMarker_t{ position, GlobalVars->realtime, damage });
}

void CWorldESP::RenderMarkers() {
	if (!Cheat.InGame)
		return;

	for (auto it = damage_markers.begin(); it != damage_markers.end();) {
		if (GlobalVars->realtime - it->time > 1.5f) {
			it = damage_markers.erase(it);
			continue;
		}

		float timer = GlobalVars->realtime - it->time;
		float alpha = std::clamp((1.5f - timer) * 2.f, 0.f, 1.f);

		Vector world_pos = it->position + Vector(0, 0, timer * 30.f);
		Vector2 pos = Render->WorldToScreen(world_pos);

		if (pos.Invalid()) {
			it++;
			continue;
		}

		Render->Text(std::to_string(it->damage), pos, config.visuals.esp.damage_marker_color->get().alpha_modulatef(alpha), Verdana, TEXT_CENTERED | TEXT_DROPSHADOW);

		it++;
	}

	for (auto it = hit_markers.begin(); it != hit_markers.end();) {
		if (GlobalVars->realtime - it->time > 3.f) {
			it = hit_markers.erase(it);
			continue;
		}

		float timer = GlobalVars->realtime - it->time;
		float alpha = std::clamp(3.f - timer, 0.f, 1.f);

		Vector2 pos = Render->WorldToScreen(it->position);

		if (pos.Invalid()) {
			it++;
			continue;
		}

		Render->Line(pos + Vector2(2, 2), pos + Vector2(5, 5), config.visuals.esp.hitmarker_color->get().alpha_modulatef(alpha));
		Render->Line(pos - Vector2(2, 2), pos - Vector2(5, 5), config.visuals.esp.hitmarker_color->get().alpha_modulatef(alpha));
		Render->Line(pos + Vector2(-2, 2), pos + Vector2(-5, 5), config.visuals.esp.hitmarker_color->get().alpha_modulatef(alpha));
		Render->Line(pos + Vector2(2, -2), pos + Vector2(5, -5), config.visuals.esp.hitmarker_color->get().alpha_modulatef(alpha));

		it++;
	}
}

void CWorldESP::SpinningStar(const ESPInfo_t& info) {
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	static std::unordered_map<int, float> rotations;
	int player_index = info.player->EntIndex();
	float& current_rotation = rotations[player_index];

	QAngle view_angles;
	EngineClient->GetViewAngles(view_angles);

	const Vector local_origin = Cheat.LocalPlayer->GetEyePosition();
	const Vector& target_origin = info.m_vecOrigin;

	const float arrow_size = 10.0f;
	const float arrow_distance = 250.0f;

	const float target_rotation = DEG2RAD(view_angles.yaw - Math::AngleFromVectors(local_origin, target_origin).y - 90.f);

	current_rotation = Math::AngleNormalize(std::lerp(current_rotation, target_rotation, GlobalVars->frametime * 8.0f));

	const Vector2 screen_center = Vector2(Cheat.ScreenSize.x / 2, Cheat.ScreenSize.y / 2);

	const Vector2 position = {
		screen_center.x + arrow_distance * std::cos(current_rotation),
		screen_center.y + arrow_distance * std::sin(current_rotation)
	};

	Vector2 points[3] = {
		Vector2(position.x - arrow_size, position.y - arrow_size),
		Vector2(position.x + arrow_size, position.y),
		Vector2(position.x - arrow_size, position.y + arrow_size)
	};

	Math::RotateTrianglePoints(points, current_rotation);

	const float arc_angle = view_angles.yaw - Math::AngleFromVectors(local_origin, target_origin).y - 90.f;
	const float arc_width = 5.0f;

	Color clr = info.m_bDormant ? config.visuals.esp.dormant_color->get() : config.visuals.esp.box_color->get();

	float distance = (local_origin - target_origin).Q_Length();
	float distance_alpha = std::clamp(1.0f - (distance - 1000.0f) / 500.0f, 0.2f, 1.0f);
	clr.a *= info.m_flAlpha * distance_alpha;

	float pulse = (std::sin(GlobalVars->curtime * 4.0f) * 0.5f + 0.5f) * 0.3f + 0.7f;

	Render->Arc(screen_center, arrow_distance + 6, arc_angle - arc_width, arc_angle + arc_width,
		Color(clr.r, clr.g, clr.b, static_cast<int>(150 * clr.a / 255 * pulse)), 4.0f);

	Render->Arc(screen_center, arrow_distance, arc_angle - arc_width, arc_angle + arc_width,
		Color(clr.r, clr.g, clr.b, static_cast<int>(150 * clr.a / 255 * pulse)), 1.5f);
}

void CWorldESP::AddDebugMessage(std::string msg) {
	bool dont = false;

	if (DebugMessages.size()) {
		for (int i = 0; i < DebugMessages.size(); ++i) {
			auto msgs = DebugMessages[i];

			if (msgs.find(msg) != std::string::npos)
				dont = true;
		}
	}

	if (dont) {
		return;
	}

	DebugMessages.push_back(msg);
}

void CWorldESP::RenderDebugMessages() {
	for (size_t i{ }; i < DebugMessagesSane.size(); ++i) {
		auto& indicator = DebugMessagesSane[i];
		Vector2 size = Render->CalcTextSize(indicator, Verdana);
		Render->Text(indicator, Vector2(Cheat.ScreenSize.x - static_cast<float>(size.x) - 30, 12.0f + (12.0f * i)), Color(220, 220, 220, 255), Verdana);
	}
}