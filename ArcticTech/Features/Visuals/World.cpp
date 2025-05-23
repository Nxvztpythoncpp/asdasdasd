#include "World.h"

#include <string>
#include <unordered_map>

#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../Utils/Utils.h"
#include "../RageBot/AnimationSystem.h"
#include "../RageBot/AutoWall.h"

void CWorld::Modulation() {
	static std::unordered_map<std::string, Color> original_colors;
	static std::unordered_map<void*, Color> backup_props;

	if (!Cheat.InGame)
		return;

	for (auto i = MaterialSystem->FirstMaterial(); i != MaterialSystem->InvalidMaterial(); i = MaterialSystem->NextMaterial(i)) {
		IMaterial* material = MaterialSystem->GetMaterial(i);
			
		if (!material)
			continue;

		if (material->IsErrorMaterial())
			continue;

		if (strstr(material->GetTextureGroupName(), "World")) {
			if (config.visuals.effects.world_color_enable->get()) {
				float r, g, b;
				material->GetColorModulation(&r, &g, &b);

				auto it = original_colors.find(material->GetName());
				if (it == original_colors.end()) {
					original_colors.insert({ material->GetName(), Color().as_fraction(r, g, b) });
					it = original_colors.find(material->GetName());
				}

				const Color clr = config.visuals.effects.world_color->get();
				material->ColorModulate(it->second * clr);
				material->AlphaModulate(clr.a / 255.f);
			}
			else {
				auto it = original_colors.find(material->GetName());
				if (it != original_colors.end())
					material->ColorModulate(it->second);
				material->AlphaModulate(1);
			}
		}
	}

	for (int i = 0; i < StaticPropMgr->m_StaticProps.Count(); i++) {
		CStaticProp* prop = &StaticPropMgr->m_StaticProps[i];

		if (!prop)
			continue;

		if (config.visuals.effects.props_color_enable->get()) {
			Color clr = config.visuals.effects.props_color->get();

			auto it = backup_props.find(prop);
			if (it == backup_props.end()) {
				backup_props.insert({ prop, Color(prop->m_DiffuseModulation[0] * 255.f, prop->m_DiffuseModulation[1] * 255.f, prop->m_DiffuseModulation[2] * 255.f, prop->m_pClientAlphaProperty->GetAlphaModulation()) });
				it = backup_props.find(prop);
			}

			prop->m_DiffuseModulation[0] = (it->second.r / 255.f) * (clr.r / 255.f);
			prop->m_DiffuseModulation[1] = (it->second.g / 255.f) * (clr.g / 255.f);
			prop->m_DiffuseModulation[2] = (it->second.b / 255.f) * (clr.b / 255.f);
			prop->m_DiffuseModulation[3] = 1.f;

			auto alpha_prop = prop->m_pClientAlphaProperty;
			if (alpha_prop)
				alpha_prop->SetAlphaModulation(clr.a);
		}
		else {
			auto it = backup_props.find(prop);

			Color clr_backup;
			if (it != backup_props.end())
				clr_backup = it->second;

			prop->m_DiffuseModulation[0] = clr_backup.r / 255.f;
			prop->m_DiffuseModulation[1] = clr_backup.g / 255.f;
			prop->m_DiffuseModulation[2] = clr_backup.b / 255.f;
			prop->m_DiffuseModulation[3] = 1.f;

			auto alpha_prop = prop->m_pClientAlphaProperty;
			if (alpha_prop)
				alpha_prop->SetAlphaModulation(clr_backup.a);
		}
	}
}

void CWorld::Fog() {
	if (config.visuals.effects.override_fog->get()) {
		cvars.fog_override->SetInt(1);
		cvars.fog_start->SetInt(config.visuals.effects.fog_start->get() * 8);
		cvars.fog_end->SetInt(config.visuals.effects.fog_end->get() * 8);
		cvars.fog_maxdensity->SetFloat(config.visuals.effects.fog_density->get() * 0.01f);
		cvars.fog_color->SetString((std::to_string(config.visuals.effects.fog_color->get().r) + " " + std::to_string(config.visuals.effects.fog_color->get().g) + " " + std::to_string(config.visuals.effects.fog_color->get().b)).c_str());
	}
	else {
		cvars.fog_override->SetInt(0);
	}
}

void CWorld::SkyBox() {
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(Utils::PatternScan("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));

	if (!Cheat.InGame)
		return;

	switch (config.visuals.effects.override_skybox->get()) {
	case 1:
		load_skybox("cs_tibet");
		break;
	case 2:
		load_skybox("cs_baggage_skybox_");
		break;
	case 3:
		load_skybox("italy");
		break;
	case 4:
		load_skybox("jungle");
		break;
	case 5:
		load_skybox("office");
		break;
	case 6:
		load_skybox("sky_cs15_daylight01_hdr");
		break;
	case 7:
		load_skybox("sky_cs15_daylight02_hdr");
		break;
	case 8:
		load_skybox("vertigoblue_hdr");
		break;
	case 9:
		load_skybox("vertigo");
		break;
	case 10:
		load_skybox("sky_day02_05_hdr");
		break;
	case 11:
		load_skybox("nukeblank");
		break;
	case 12:
		load_skybox("sky_venice");
		break;
	case 13:
		load_skybox("sky_cs15_daylight03_hdr");
		break;
	case 14:
		load_skybox("sky_cs15_daylight04_hdr");
		break;
	case 15:
		load_skybox("sky_csgo_cloudy01");
		break;
	case 16:
		load_skybox("sky_csgo_night02");
		break;
	case 17:
		load_skybox("sky_csgo_night02b");
		break;
	case 18:
		load_skybox("sky_csgo_night_flat");
		break;
	case 19:
		load_skybox("sky_dust");
		break;
	case 20:
		load_skybox("vietnam");
		break;
	}
}

void CWorld::ProcessCamera(CViewSetup* setup) {
	static float current_fov = 90.f;
	static Vector current_camera_offset;
	static float current_distance = 0.f;

	m_flFOVOverriden = 0.f;

	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	float player_fov = config.visuals.effects.fov->get();
	if (Cheat.LocalPlayer->IsAlive() && Cheat.LocalPlayer->m_bIsScoped() && ctx.active_weapon) {
		switch (ctx.active_weapon->m_zoomLevel()) {
		case 1:
			player_fov += (setup->fov - player_fov) * config.visuals.effects.fov_zoom->get() * 0.01f;
			break;
		case 2:
			player_fov += (setup->fov - player_fov) * config.visuals.effects.fov_second_zoom->get() * 0.01f;
			break;
		}
	}

	current_fov = Math::Lerp(current_fov, player_fov, 0.1f);
	setup->fov = current_fov;
	m_flFOVOverriden = current_fov;

	if (config.visuals.effects.thirdperson->get() && Cheat.LocalPlayer->IsAlive()) {
		Input->m_fCameraInThirdPerson = true;
		QAngle angles; EngineClient->GetViewAngles(angles);
		QAngle backAngle = QAngle(angles.yaw - 180, -angles.pitch, 0);
		backAngle.Normalize();
		Vector cameraDirection = Math::AngleVectors(angles);
		if (cameraDirection.z == 0.f) // fuck valve shitcode
			cameraDirection.z = 0.01f;

		CGameTrace trace;
		CTraceFilterWorldOnly filter;
		Ray_t ray;
		Vector eyePos = (ctx.fake_duck ? Vector(0, 0, 64) : Cheat.LocalPlayer->m_vecViewOffset()) + Cheat.LocalPlayer->GetAbsOrigin();
		ray.Init(eyePos, eyePos - cameraDirection * config.visuals.effects.thirdperson_distance->get(), Vector(-16, -16, -16), Vector(16, 16, 16));

		EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

		float distance = trace.fraction * config.visuals.effects.thirdperson_distance->get();

		Input->m_vecCameraOffset = Vector(angles.pitch, angles.yaw, distance);
	}

	else {
		Input->m_fCameraInThirdPerson = false;

		if (config.visuals.effects.thirdperson_dead->get() && Cheat.LocalPlayer->m_iObserverMode() == 4)
		{
			Cheat.LocalPlayer->m_iObserverMode() = 5;
		}

	}

	if (Cheat.LocalPlayer && (!Cheat.LocalPlayer->IsAlive() || Cheat.LocalPlayer->m_iTeamNum() == 1) && Cheat.LocalPlayer->m_iObserverMode() == OBS_MODE_CHASE) { // disable spectators interpolation
		CBasePlayer* observer = (CBasePlayer*)EntityList->GetClientEntityFromHandle(Cheat.LocalPlayer->m_hObserverTarget());

		if (observer) {
			Vector dir = Math::AngleVectors(setup->angles);
			Vector origin = AnimationSystem->GetInterpolated(observer) + Vector(0, 0, 64 - observer->m_flDuckAmount() * 28);
			CGameTrace trace = EngineTrace->TraceHull(origin,
				origin - dir * config.visuals.effects.thirdperson_distance->get(), Vector(-20, -20, -20), Vector(20, 20, 20), CONTENTS_SOLID, observer);
			setup->origin = trace.endpos;
		}
	}
}

void CWorld::OverrideSensitivity() {
	if (m_flFOVOverriden == 0.f || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
		return;
	
	int iDefaultFOV = cvars.default_fov->GetInt();
	int	localFOV = m_flFOVOverriden;

	CSGOHud->m_flFOVSensitivityAdjust = 1.0f;

	if (CSGOHud->m_flMouseSensitivityFactor) {
		CSGOHud->m_flMouseSensitivity = cvars.sensitivity->GetFloat() * CSGOHud->m_flMouseSensitivityFactor;
	} else {
		if (iDefaultFOV == 0)
			return;

		float zoomSensitivity = cvars.zoom_sensitivity_ratio_mouse->GetFloat();

		CSGOHud->m_flFOVSensitivityAdjust =
			((float)localFOV / (float)iDefaultFOV) * // linear fov downscale
			zoomSensitivity; // sensitivity scale factor
		CSGOHud->m_flMouseSensitivity = CSGOHud->m_flFOVSensitivityAdjust * cvars.sensitivity->GetFloat(); // regular sensitivity
	}
}

void CWorld::Smoke() {
	if (!Cheat.InGame)
		return;

	static const char* smokeMaterials[] = {
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_smokegrenade"
	};

	for (const char* mat : smokeMaterials) {
		IMaterial* material = MaterialSystem->FindMaterial(mat);
		material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, config.visuals.effects.removals->get(3));
	}
}

void CWorld::RemoveBlood() {
	if (!Cheat.InGame)
		return;

	static const char* bloodMaterials[] = {
		"decals/blood1_subrect.vmt",
		"decals/blood2_subrect.vmt",
		"decals/blood3_subrect.vmt",
		"decals/blood4_subrect.vmt",
		"decals/blood5_subrect.vmt",
		"decals/blood6_subrect.vmt",
		"decals/blood1.vmt",
		"decals/blood2.vmt",
		"decals/blood3.vmt",
		"decals/blood4.vmt",
		"decals/blood5.vmt",
		"decals/blood6.vmt",
		"decals/blood7.vmt",
		"decals/blood8.vmt",
	};

	for (const char* mat : bloodMaterials) {
		IMaterial* material = MaterialSystem->FindMaterial(mat);

		if (material && !material->IsErrorMaterial()) {
			material->IncrementReferenceCount();
			material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, config.visuals.effects.removals->get(5));
		}
	}
}

void CWorld::Crosshair() {
	cvars.weapon_debug_spread_show->SetInt((config.visuals.other_esp.sniper_crosshair->get() && Cheat.LocalPlayer && Cheat.LocalPlayer->IsAlive() && !Cheat.LocalPlayer->m_bIsScoped()) ? 2 : 0);

	if (Cheat.InGame && Cheat.LocalPlayer->IsAlive() && config.visuals.effects.remove_scope->get() == 1 && Cheat.LocalPlayer->m_bIsScoped()) {
		Render->BoxFilled(Vector2(Cheat.ScreenSize.x / 2, 0), Vector2(Cheat.ScreenSize.x / 2 + 1, Cheat.ScreenSize.y), Color(10, 10, 10, 255));
		Render->BoxFilled(Vector2(0, Cheat.ScreenSize.y / 2), Vector2(Cheat.ScreenSize.x, Cheat.ScreenSize.y / 2 + 1), Color(10, 10, 10, 255));
	}

	if (!config.visuals.other_esp.penetration_crosshair->get())
		return;

	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
		return;

	CBaseCombatWeapon* activeWeapon = Cheat.LocalPlayer->GetActiveWeapon();

	if (!activeWeapon || !activeWeapon->ShootingWeapon() || activeWeapon->m_iItemDefinitionIndex() == Taser)
		return;

	Vector eyePos = (ctx.fake_duck ? Vector(0, 0, 64) : Cheat.LocalPlayer->m_vecViewOffset()) + Cheat.LocalPlayer->GetAbsOrigin();

	QAngle viewAngle;
	EngineClient->GetViewAngles(viewAngle);

	Vector direction = Math::AngleVectors(viewAngle);

	if (direction.z == 0.f)
		direction.z = 0.01f;

	FireBulletData_t fb_data;
	bool hit = AutoWall->FireBullet(Cheat.LocalPlayer, eyePos, eyePos + direction * 8192, fb_data);

	Color crosshairColor(255, 0, 50);

	if (fb_data.num_impacts > 1) {
		crosshairColor = Color(50, 255, 0);
	}

	if (hit)
		crosshairColor = Color(56, 209, 255);

	Render->BoxFilled(Cheat.ScreenSize * 0.5f - Vector2(0, 1), Cheat.ScreenSize * 0.5f + Vector2(1, 2), crosshairColor);
	Render->BoxFilled(Cheat.ScreenSize * 0.5f - Vector2(1, 0), Cheat.ScreenSize * 0.5f + Vector2(2, 1), crosshairColor);
}

void CWorld::SunDirection() {
	static bool backupShouldReset;
	static Vector backupDirection, backupEnvDirection;
	static float backupDistance = -1.f;

	if (!Cheat.InGame) {
		backupShouldReset = false;
		backupDistance = -1.f;
		return;
	}

	static float lerp_sun_pitch = 0;
	static float lerp_sun_yaw = 0;

	static auto cascade_light_ptr = *reinterpret_cast<CCascadeLight***>(Utils::PatternScan("client.dll", "A1 ? ? ? ? B9 ? ? ? ? 56 F3 0F 7E 80", 0x1));
	
	CCascadeLight* cascade_light_entity = *cascade_light_ptr;

	if (!cascade_light_entity) {
		backupShouldReset = false;
		backupDistance = -1.f;
		return;
	}

	if (config.visuals.effects.custom_sun_direction->get()) {
		if (backupDistance == -1.f) {
			backupDirection = cascade_light_entity->m_shadowDirection();
			backupEnvDirection = cascade_light_entity->m_envLightShadowDirection();
			backupDistance = cascade_light_entity->m_flMaxShadowDist();
		}

		lerp_sun_pitch = lerp_sun_pitch + (config.visuals.effects.sun_pitch->get() - lerp_sun_pitch) * GlobalVars->frametime * 8;
		lerp_sun_yaw = lerp_sun_yaw + (config.visuals.effects.sun_yaw->get() - lerp_sun_yaw) * GlobalVars->frametime * 8;

		Vector newDirection = Math::AngleVectors(QAngle(lerp_sun_pitch, lerp_sun_yaw, 0));

		cascade_light_entity->m_envLightShadowDirection() = newDirection;
		cascade_light_entity->m_shadowDirection() = newDirection;
		cascade_light_entity->m_flMaxShadowDist() = config.visuals.effects.sun_distance->get();
		backupShouldReset = true;
	}
	else if (backupShouldReset) { 
		cascade_light_entity->m_envLightShadowDirection() = backupEnvDirection;
		cascade_light_entity->m_shadowDirection() = backupDirection;
		cascade_light_entity->m_flMaxShadowDist() = backupDistance;
	}
}

inline std::unique_ptr<CWorld> World = std::make_unique<CWorld>();