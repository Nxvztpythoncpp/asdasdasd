#define TIME_TO_TICKS( dt ) ( (int)( 0.5f + (float)(dt) / GlobalVars->interval_per_tick ) )
#include "LagCompensation.h"
#include "AnimationSystem.h"
#include "../Misc/Prediction.h"
#include "../../SDK/Interfaces.h" // Zawiera deklaracjê globalnego ICvar* CVar;
#include "../../SDK/Globals.h"    // Zawiera definicjê extern CVars cvars; i extern Ctx_t ctx;
#include "../AntiAim/AntiAim.h"
#include "../../Utils/Utils.h"
#include <algorithm> // MASZ TO!
#include "../../SDK/NetMessages.h"
#include "../Visuals/ESP.h"
#include "Exploits.h"

LagRecord* CLagCompensation::BackupData(CBasePlayer* player) {
	LagRecord* record = new LagRecord;

	record->player = player;
	record->m_vecAbsOrigin = player->GetAbsOrigin();
	RecordDataIntoTrack(player, record);

	return record;
}

void CLagCompensation::RecordDataIntoTrack(CBasePlayer* player, LagRecord* record) {
	record->player = player;

	record->m_angEyeAngles = player->m_angEyeAngles();
	record->m_flSimulationTime = player->m_flSimulationTime();
	record->m_vecOrigin = player->m_vecOrigin();
	record->m_fFlags = player->m_fFlags();
	record->m_flCycle = player->m_flCycle();
	record->m_nSequence = player->m_nSequence();
	record->m_flDuckAmout = player->m_flDuckAmount();
	record->m_flDuckSpeed = player->m_flDuckSpeed();
	record->m_vecMaxs = player->m_vecMaxs();
	record->m_vecMins = player->m_vecMins();
	record->m_vecVelocity = player->m_vecVelocity();
	record->m_vecAbsAngles = player->GetAbsAngles();

	if (!record->bone_matrix_filled) {
		memcpy(record->bone_matrix, player->GetCachedBoneData().Base(), sizeof(matrix3x4_t) * player->GetCachedBoneData().Count());
		record->bone_matrix_filled = true;
	}

	memcpy(record->animlayers, player->GetAnimlayers(), sizeof(AnimationLayer) * 13);
}

void CLagCompensation::BacktrackEntity(LagRecord* record, bool copy_matrix, bool use_aim_matrix) {
	CBasePlayer* player = record->player;

	player->m_vecOrigin() = record->m_vecOrigin;
	player->SetAbsOrigin(record->m_vecAbsOrigin);
	player->m_fFlags() = record->m_fFlags;
	player->m_flCycle() = record->m_flCycle;
	player->m_nSequence() = record->m_nSequence;
	player->m_flDuckAmount() = record->m_flDuckAmout;
	player->m_flDuckSpeed() = record->m_flDuckSpeed;
	player->m_vecVelocity() = record->m_vecVelocity;
	player->SetAbsAngles(record->m_vecAbsAngles);
	player->ForceBoneCache();

	player->SetCollisionBounds(record->m_vecMins, record->m_vecMaxs);

	if (copy_matrix) {
		if (use_aim_matrix) {
			memcpy(player->GetCachedBoneData().Base(), record->clamped_matrix, player->GetCachedBoneData().Count() * sizeof(matrix3x4_t));
		}
		else {
			memcpy(player->GetCachedBoneData().Base(), record->bone_matrix, player->GetCachedBoneData().Count() * sizeof(matrix3x4_t));
		}
	}
}

void LagRecord::BuildMatrix() {
	memcpy(clamped_matrix, aim_matrix, 128 * sizeof(matrix3x4_t));
	memcpy(safe_matrix, opposite_matrix, 128 * sizeof(matrix3x4_t));

	if (config.antiaim.angles.legacy_desync->get())
		return;

	auto backup_eye_angle = player->m_angEyeAngles();

	player->ClampBonesInBBox(safe_matrix, BONE_USED_BY_HITBOX);

	if (config.ragebot.aimbot.roll_resolver->get())
		player->m_angEyeAngles().roll = config.ragebot.aimbot.roll_angle->get() * (resolver_data.side != 0 ? resolver_data.side : 1);
	
	player->ClampBonesInBBox(clamped_matrix, BONE_USED_BY_ANYTHING);

	player->m_angEyeAngles() = backup_eye_angle;
}

void CLagCompensation::OnNetUpdate() {
	if (!Cheat.InGame)
		return;

	INetChannel* nc = ClientState->m_NetChannel;
	auto nci = EngineClient->GetNetChannelInfo();

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* pl = (CBasePlayer*)EntityList->GetClientEntity(i);

		if (!pl || !pl->IsAlive() || pl == Cheat.LocalPlayer || pl->m_bDormant())
			continue;

		auto& records = lag_records[i];

		if (!records.empty() && pl->m_flSimulationTime() == pl->m_flOldSimulationTime())
			continue;

		LagRecord* prev_record = !records.empty() ? &records.front() : nullptr;

		if (prev_record && prev_record->player != pl) {
			records.clear();
			prev_record = nullptr;
		}

		if (prev_record && prev_record->animlayers[ANIMATION_LAYER_ALIVELOOP].m_flCycle == pl->GetAnimlayers()[ANIMATION_LAYER_ALIVELOOP].m_flCycle) {
			pl->m_flOldSimulationTime() = pl->m_flSimulationTime();
			continue;
		}

		LagRecord* new_record = &records.emplace_front();

		new_record->prev_record = prev_record;
		new_record->update_tick = GlobalVars->tickcount;			
		new_record->m_flSimulationTime = pl->m_flSimulationTime();
		new_record->m_flServerTime = EngineClient->GetLastTimeStamp();

		new_record->shifting_tickbase = max_simulation_time[i] >= new_record->m_flSimulationTime;

		if (new_record->m_flSimulationTime > max_simulation_time[i] || abs(max_simulation_time[i] - new_record->m_flSimulationTime) > 3.f)
			max_simulation_time[i] = new_record->m_flSimulationTime;

		last_update_tick[i] = GlobalVars->tickcount;

		AnimationSystem->UpdateAnimations(pl, new_record, records);
		RecordDataIntoTrack(pl, new_record);

		if (prev_record)
			new_record->breaking_lag_comp = (prev_record->m_vecOrigin - new_record->m_vecOrigin).LengthSqr() > 4096.f;

		if (config.visuals.esp.shared_esp->get() && !EngineClient->IsVoiceRecording() && nc) {
			if (config.visuals.esp.share_with_enemies->get() || !pl->IsTeammate()) {
				SharedESP_t msg;

				player_info_t pinfo;
				EngineClient->GetPlayerInfo(i, &pinfo);

				msg.m_iPlayer = pinfo.userId;
				msg.m_ActiveWeapon = pl->GetActiveWeapon() ? pl->GetActiveWeapon()->m_iItemDefinitionIndex() : 0;
				msg.m_iHealth = pl->m_iHealth();
				msg.m_vecOrigin = new_record->m_vecOrigin;

				NetMessages->SendNetMessage((SharedVoiceData_t*)&msg);
			}
		}

		while (records.size() > 32)
			records.pop_back();

		if (config.visuals.esp.show_server_hitboxes->get() && nci && nci->IsLoopback())
			pl->DrawServerHitboxes(GlobalVars->interval_per_tick, true);
	}
}


LagRecord* CLagCompensation::ExtrapolateRecord(LagRecord* current_record, int ticks_to_extrapolate) {
    if (!current_record || !current_record->player || !current_record->player->IsAlive() || ticks_to_extrapolate <= 0) {
        return nullptr;
    }

    CBasePlayer* player = current_record->player;
    const int player_idx = player->EntIndex();
    if (player_idx < 0 || player_idx >= 64) {
        return nullptr;
    }

    const float time_delta_per_tick = GlobalVars->interval_per_tick;

    const int max_reasonable_extrapolation_ticks_calculated = TIME_TO_TICKS(0.22f); // Zmieniona nazwa dla pewnoœci

    // Zamiast std::min, u¿yjmy zwyk³ego if-a, ¿eby zobaczyæ, czy to przejdzie
    if (ticks_to_extrapolate > max_reasonable_extrapolation_ticks_calculated) {
        ticks_to_extrapolate = max_reasonable_extrapolation_ticks_calculated;
    }

    // Teraz linia, która by³a oryginalnie 183 (if poni¿ej)
    if (ticks_to_extrapolate <= 0) {
        return nullptr;
    }

    const float total_extrapolation_time = static_cast<float>(ticks_to_extrapolate) * time_delta_per_tick;

    auto& extrapolated_history = extrapolated_records[player_idx];
    if (extrapolated_history.size() >= 32) {
        extrapolated_history.pop_back();
    }
    LagRecord* extrapolated_rec = &extrapolated_history.emplace_front();
    *extrapolated_rec = *current_record;

    Vector predicted_velocity = current_record->m_vecVelocity;
    if (!(current_record->m_fFlags & FL_ONGROUND)) {
        static ConVar* sv_gravity_cv = nullptr;
        if (!sv_gravity_cv) sv_gravity_cv = CVar->FindVar("sv_gravity");
        float gravity = sv_gravity_cv ? sv_gravity_cv->GetFloat() : 800.0f;
        predicted_velocity.z -= gravity * total_extrapolation_time;
    }

    Vector predicted_origin = current_record->m_vecOrigin + (predicted_velocity * total_extrapolation_time);

    int predicted_flags = current_record->m_fFlags;


    extrapolated_rec->m_flSimulationTime = current_record->m_flSimulationTime + total_extrapolation_time;
    extrapolated_rec->update_tick = TIME_TO_TICKS(extrapolated_rec->m_flSimulationTime);
    extrapolated_rec->m_vecOrigin = predicted_origin;
    extrapolated_rec->m_vecVelocity = predicted_velocity;
    extrapolated_rec->m_fFlags = predicted_flags;
    extrapolated_rec->m_vecAbsOrigin = predicted_origin;
    extrapolated_rec->m_angEyeAngles = current_record->m_angEyeAngles;
    extrapolated_rec->m_flDuckAmout = current_record->m_flDuckAmout;
    extrapolated_rec->resolver_data = current_record->resolver_data;


    float g_backup_curtime = GlobalVars->curtime;
    float g_backup_frametime = GlobalVars->frametime;
    int g_backup_tickcount = GlobalVars->tickcount;

    CBasePlayer* real_player_object = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(player_idx));
    if (!real_player_object || real_player_object != player) {
        return nullptr;
    }

    Vector backup_player_origin = player->m_vecOrigin();
    Vector backup_player_velocity = player->m_vecVelocity();
    int backup_player_flags = player->m_fFlags();
    QAngle backup_player_abs_angles = player->GetAbsAngles();
    QAngle backup_player_eye_angles = player->m_angEyeAngles();
    float backup_player_duck_amount = player->m_flDuckAmount();
    float backup_player_lby = player->m_flLowerBodyYawTarget();
    std::array<float, 24> backup_player_pose_params = player->m_flPoseParameter();
    AnimationLayer backup_player_layers[13];
    memcpy(backup_player_layers, player->GetAnimlayers(), sizeof(AnimationLayer) * 13);

    CCSGOPlayerAnimationState backup_player_animstate_obj;
    CCSGOPlayerAnimationState* player_animstate_ptr = player->GetAnimstate();
    if (player_animstate_ptr) {
        backup_player_animstate_obj = *player_animstate_ptr;
    }
    else {
        return nullptr; 
    }

    player->m_vecOrigin() = extrapolated_rec->m_vecOrigin;
    player->m_vecVelocity() = extrapolated_rec->m_vecVelocity;
    player->m_fFlags() = extrapolated_rec->m_fFlags;
    player->SetAbsOrigin(extrapolated_rec->m_vecAbsOrigin);
    player->m_angEyeAngles() = extrapolated_rec->m_angEyeAngles;
    player->m_flDuckAmount() = extrapolated_rec->m_flDuckAmout;
    memcpy(player->GetAnimlayers(), current_record->animlayers, sizeof(AnimationLayer) * 13);
    *player_animstate_ptr = backup_player_animstate_obj; 

    for (int i = 0; i < ticks_to_extrapolate; ++i) {
        float iter_sim_time = current_record->m_flSimulationTime + (static_cast<float>(i + 1) * time_delta_per_tick);

        GlobalVars->curtime = iter_sim_time;
        GlobalVars->frametime = time_delta_per_tick;
        GlobalVars->tickcount = TIME_TO_TICKS(iter_sim_time);

        player_animstate_ptr->bOnGround = (player->m_fFlags() & FL_ONGROUND) != 0;

        if (player_animstate_ptr->nLastUpdateFrame >= GlobalVars->tickcount) {
            player_animstate_ptr->nLastUpdateFrame = GlobalVars->tickcount - 1;
        }

        player_animstate_ptr->Update(current_record->m_angEyeAngles);
    }

    memcpy(extrapolated_rec->animlayers, player->GetAnimlayers(), sizeof(AnimationLayer) * 13);

    GlobalVars->curtime = extrapolated_rec->m_flSimulationTime;
    GlobalVars->tickcount = TIME_TO_TICKS(extrapolated_rec->m_flSimulationTime);

    AnimationSystem->BuildMatrix(player, extrapolated_rec->bone_matrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, extrapolated_rec->animlayers);
    extrapolated_rec->bone_matrix_filled = true;

    memcpy(extrapolated_rec->aim_matrix, extrapolated_rec->bone_matrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);


    GlobalVars->curtime = g_backup_curtime;
    GlobalVars->frametime = g_backup_frametime;
    GlobalVars->tickcount = g_backup_tickcount;

    player->m_vecOrigin() = backup_player_origin;
    player->m_vecVelocity() = backup_player_velocity;
    player->m_fFlags() = backup_player_flags;
    player->SetAbsAngles(backup_player_abs_angles);
    player->m_angEyeAngles() = backup_player_eye_angles;
    player->m_flDuckAmount() = backup_player_duck_amount;
    player->m_flLowerBodyYawTarget() = backup_player_lby;
    player->m_flPoseParameter() = backup_player_pose_params;
    memcpy(player->GetAnimlayers(), backup_player_layers, sizeof(AnimationLayer) * 13);
    if (player->GetAnimstate()) {
        *player->GetAnimstate() = backup_player_animstate_obj;
    }
    player->InvalidateBoneCache();

    extrapolated_rec->invalid = false;
    return extrapolated_rec;
}


float CLagCompensation::GetLerpTime() {
	static ConVar* cl_interp_cv = nullptr; // Zmieñ na wskaŸniki static
    static ConVar* cl_updaterate_cv = nullptr;
    static ConVar* sv_minupdaterate_cv = nullptr;
    static ConVar* sv_maxupdaterate_cv = nullptr;
    static ConVar* cl_interp_ratio_cv = nullptr;
    static ConVar* sv_min_interp_ratio_cv = nullptr;
    static ConVar* sv_max_interp_ratio_cv = nullptr;

    if (!cl_interp_cv) cl_interp_cv = CVar->FindVar("cl_interp"); 
    if (!cl_updaterate_cv) cl_updaterate_cv = CVar->FindVar("cl_updaterate");
    if (!sv_minupdaterate_cv) sv_minupdaterate_cv = CVar->FindVar("sv_minupdaterate");
    if (!sv_maxupdaterate_cv) sv_maxupdaterate_cv = CVar->FindVar("sv_maxupdaterate");
    if (!cl_interp_ratio_cv) cl_interp_ratio_cv = CVar->FindVar("cl_interp_ratio");
    if (!sv_min_interp_ratio_cv) sv_min_interp_ratio_cv = CVar->FindVar("sv_client_min_interp_ratio");
    if (!sv_max_interp_ratio_cv) sv_max_interp_ratio_cv = CVar->FindVar("sv_client_max_interp_ratio");

    
    float fl_cl_updaterate = cl_updaterate_cv ? cl_updaterate_cv->GetFloat() : 64.0f;
    float fl_sv_minupdaterate = sv_minupdaterate_cv ? sv_minupdaterate_cv->GetFloat() : 10.0f;
    float fl_sv_maxupdaterate = sv_maxupdaterate_cv ? sv_maxupdaterate_cv->GetFloat() : 64.0f;
    float fl_cl_interp_ratio = cl_interp_ratio_cv ? cl_interp_ratio_cv->GetFloat() : 2.0f;
    float fl_sv_min_interp_ratio = sv_min_interp_ratio_cv ? sv_min_interp_ratio_cv->GetFloat() : 1.0f;
    float fl_sv_max_interp_ratio = sv_max_interp_ratio_cv ? sv_max_interp_ratio_cv->GetFloat() : 2.0f;
    float fl_cl_interp = cl_interp_cv ? cl_interp_cv->GetFloat() : 0.03125f;


	const float update_rate = std::clamp<float>(fl_cl_updaterate, fl_sv_minupdaterate, fl_sv_maxupdaterate);
	const float interp_ratio = std::clamp<float>(fl_cl_interp_ratio, fl_sv_min_interp_ratio, fl_sv_max_interp_ratio);

	return std::clamp<float>(interp_ratio / update_rate, fl_cl_interp, 1.f);
}

int GetServerTick(int latency) 
{							  
	int extra_choke = 0;

	if (ctx.fake_duck)
		extra_choke = 14 - ClientState->m_nChokedCommands;

	return GlobalVars->tickcount + TIME_TO_TICKS(static_cast<float>(latency)) + extra_choke; 
}

bool CLagCompensation::ValidRecord(LagRecord* record) {
    if (!record || !record->player || record->invalid) {
        return false;
    }

    INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
    if (!nci) {
        return false;
    }

    float sv_maxunlag = cvars.sv_maxunlag ? cvars.sv_maxunlag->GetFloat() : 0.2f;
   

    float net_latency = nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING);
    float lerp_time = GetLerpTime(); 

    float total_correction_seconds = net_latency + lerp_time;
    total_correction_seconds = std::clamp(total_correction_seconds, 0.0f, sv_maxunlag);

    float estimated_current_server_simtime = TICKS_TO_TIME(ctx.corrected_tickbase);
    float target_record_simtime = estimated_current_server_simtime - total_correction_seconds;

    
    float oldest_acceptable_simtime_fudge_factor = 2.0f; 
    
    float max_acceptable_simtime_difference_value = 0.05f; 
    

    float oldest_acceptable_simtime = estimated_current_server_simtime - sv_maxunlag - (oldest_acceptable_simtime_fudge_factor * GlobalVars->interval_per_tick);

    if (record->m_flSimulationTime < oldest_acceptable_simtime) {
        return false;
    }

    float simtime_difference = fabsf(target_record_simtime - record->m_flSimulationTime);

    if (simtime_difference > max_acceptable_simtime_difference_value) {
        return false;
    }

    if (record->shifting_tickbase || record->breaking_lag_comp) {
      
        return false;
    }

    return true;
}


LagRecord* CLagCompensation::GetLastRecord(int idx) {
	LagRecord* record = nullptr;
	auto& records = lag_records[idx];
	for (auto it = records.rbegin(); it != records.rend(); it++) {
		if (!ValidRecord(&*it)) { 
			if (it->breaking_lag_comp || it->invalid) 
				break;

			continue;
		}
		record = &*it;
	}

	return record;
}

void CLagCompensation::Reset(int index) {
	if (index != -1) {
		lag_records[index].clear();
		max_simulation_time[index] = 0.f;
		last_update_tick[index] = 0;
	}
	else {
		// Poprawiony warunek pêtli, size() zwraca size_t, i powinno byæ <
		for (size_t i = 0; i < lag_records.size(); i++) {
			lag_records[i].clear();
			max_simulation_time[i] = 0.f;
			last_update_tick[i] = 0;
		}
	}
}

void CLagCompensation::Invalidate(int index) {
	for (auto& record : lag_records[index])
		record.invalid = true;
}

inline std::unique_ptr<CLagCompensation> LagCompensation = std::make_unique<CLagCompensation>();