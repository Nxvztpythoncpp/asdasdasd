#pragma once

#include <deque>
#include "../../SDK/Misc/CBasePlayer.h"
#include <array>

struct LagRecord;
struct AnimationLayer;

enum class R_PlayerState {
	STANDING,
	MOVING,
	AIR
};

enum class R_AntiAimType {
	NONE,
	STATIC,
	JITTER,
	SPIN,
	STATIC_DESYNC,
	LBY_BREAK,
	UNKNOWN
};

enum class ResolverType {
	NONE,
	FREESTAND,
	LOGIC,
	ANIM,
	BRUTEFORCE,
	MEMORY,
	DEFAULT,
};

struct ResolverLayer_t {
	float desync = 0.f;
	float move_layer_delta = 0.f;
	AnimationLayer test_layers[13];
};

struct ResolverData_t {
	R_PlayerState player_state = R_PlayerState::STANDING;
	R_AntiAimType antiaim_type = R_AntiAimType::UNKNOWN; 
	ResolverType resolver_type = ResolverType::NONE;
	ResolverLayer_t layers[6];
	float max_desync_delta = 0.f;
	int side = 0;
};

struct ResolverDataStatic_t { 
	int brute_side = 0;
	float brute_time = 0.f;
	float last_resolved = 0.f;
	ResolverType res_type_last = ResolverType::NONE;
	int last_side = 0;
	int missed_shots = 0;
	float last_lby = 0.f;
	float last_lby_update_time = 0.f;
	bool lby_updated_this_tick = false;

	bool jitter_pattern_detected = false;
	float jitter_angle1 = 0.f;
	float jitter_angle2 = 0.f;
	int last_resolved_jitter_angle_idx = 0; 

	void reset() {
		last_resolved = 0.f;
		brute_side = 0;
		brute_time = 0.f;
		missed_shots = 0;
		last_side = 0;
		res_type_last = ResolverType::NONE;
		last_lby = 0.f;
		last_lby_update_time = 0.f;
		lby_updated_this_tick = false;

		
		jitter_pattern_detected = false;
		jitter_angle1 = 0.f;
		jitter_angle2 = 0.f;
		last_resolved_jitter_angle_idx = 0; 
	}
};

class CResolver {
	std::array<ResolverDataStatic_t, 65> resolver_data;
	float GetTime();
	float ValveAngleDiff(float destAngle, float srcAngle); 
public:
	CResolver() {
		for (int i = 0; i < 65; ++i) {
			resolver_data[i].reset();
		}
	}

	void Reset(CBasePlayer* pl = nullptr);
	R_PlayerState DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers);
	R_AntiAimType DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records);
	void SetupLayer(LagRecord* record, int idx, float delta);
	void SetupResolverLayers(CBasePlayer* player, LagRecord* record);
	void DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records);
	void Apply(LagRecord* record);
	void Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records);
	void OnMiss(CBasePlayer* player, LagRecord* record);
	void OnHit(CBasePlayer* player, LagRecord* record);
};

extern CResolver* Resolver;