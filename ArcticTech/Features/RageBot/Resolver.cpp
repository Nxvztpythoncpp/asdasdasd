#include "Resolver.h"
#include <algorithm>
#include <cmath>     
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "LagCompensation.h"
#include "../../SDK/Globals.h"
#include "AnimationSystem.h"
#include "../../Utils/Console.h"
#include "../../SDK/Interfaces/IEngineTrace.h" 
CResolver* Resolver = new CResolver;

float CResolver::GetTime() {
	if (!Cheat.LocalPlayer)
		return GlobalVars->curtime;
	return TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
}

float FindAvgYaw(const std::deque<LagRecord>& records) {
	if (records.empty()) return 0.f;

	size_t num_records_to_average = (std::min)(static_cast<size_t>(4), records.size());
	if (num_records_to_average == 0) return 0.f;
	if (num_records_to_average == 1) return records.back().m_angEyeAngles.yaw;

	float sin_sum = 0.f;
	float cos_sum = 0.f;
	int count = 0;

	size_t records_to_consider = (std::min)(records.size(), static_cast<size_t>(5));
	if (records_to_consider <= 1) return records.empty() ? 0.f : records.back().m_angEyeAngles.yaw;

	for (size_t i = 0; i < records_to_consider - 1; ++i) {
		if (records.size() <= (i + 1)) break;
		const LagRecord& record = records[records.size() - 2 - i];
		float eyeYaw = record.m_angEyeAngles.yaw;
		sin_sum += std::sinf(DEG2RAD(eyeYaw));
		cos_sum += std::cosf(DEG2RAD(eyeYaw));
		count++;
		if (count >= 4) break;
	}

	if (count == 0) return records.back().m_angEyeAngles.yaw;
	return RAD2DEG(std::atan2f(sin_sum / static_cast<float>(count), cos_sum / static_cast<float>(count)));
}

void CResolver::Reset(CBasePlayer* pl) {
	if (pl && pl->EntIndex() >= 0 && pl->EntIndex() < 65) {
		resolver_data[pl->EntIndex()].reset();
		return;
	}
	if (!pl) {
		for (int i = 0; i < 65; ++i) {
			resolver_data[i].reset();
		}
	}
}

R_PlayerState CResolver::DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers) {
	if (!player || !animlayers) return R_PlayerState::STANDING;
	if (!(player->m_fFlags() & FL_ONGROUND))
		return R_PlayerState::AIR;
	CCSGOPlayerAnimationState* animstate = player->GetAnimstate();
	if (!animstate) return R_PlayerState::STANDING;

	if (player->m_vecVelocity().LengthSqr() > (1.0f * 1.0f) && animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight > 0.01f) {
		if (animstate->flWalkToRunTransition > 0.2f || animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate > 0.1f) {
			return R_PlayerState::MOVING;
		}
	}
	return R_PlayerState::STANDING;
}

R_AntiAimType CResolver::DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records) {
	if (records.size() < 8)
		return R_AntiAimType::NONE;

	int jittered_records_count = 0;
	float total_delta_sum = 0.f;
	int considered_records = 0;

	if (records.empty()) return R_AntiAimType::NONE;
	if (records.size() <= 1) return R_AntiAimType::NONE;

	float last_yaw = records.front().m_angEyeAngles.yaw;

	size_t num_to_check = (std::min)(static_cast<size_t>(7), records.size() - 1);

	for (size_t i = 0; i < num_to_check; ++i) {
		if (records.size() <= (i + 1)) break;
		const LagRecord& current_record = records[i];
		const LagRecord& prev_record = records[i + 1];

		float delta = std::fabs(Math::AngleDiff(current_record.m_angEyeAngles.yaw, prev_record.m_angEyeAngles.yaw));
		total_delta_sum += delta;
		considered_records++;

		if (delta > 30.f)
			jittered_records_count++;
	}

	if (considered_records == 0) return R_AntiAimType::NONE;

	float average_delta = total_delta_sum / static_cast<float>(considered_records);

	if (jittered_records_count >= (considered_records / 2) && average_delta > 20.f)
		return R_AntiAimType::JITTER;
	if (average_delta < 15.f)
		return R_AntiAimType::STATIC;

	return R_AntiAimType::UNKNOWN;
}

float ValveAngleDiff(float destAngle, float srcAngle) {
	float delta;
	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle) {
		if (delta >= 180)
			delta -= 360;
	}
	else {
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

void CResolver::SetupLayer(LagRecord* record, int idx, float delta) {
	if (!record || !record->player) return;
	CCSGOPlayerAnimationState* animstate = record->player->GetAnimstate();
	if (!animstate) return;

	QAngle angles = record->m_angEyeAngles;
	animstate->flFootYaw = Math::AngleNormalize(angles.yaw + delta);

	if (record->prev_record) {
		memcpy(record->player->GetAnimlayers(), record->animlayers, sizeof(AnimationLayer) * 13);
	}

	animstate->ForceUpdate();
	animstate->Update(angles);

	auto& resolver_layer_entry = record->resolver_data.layers[idx];
	resolver_layer_entry.desync = delta;
	if (record->player->GetAnimlayers() && record->animlayers) {
		resolver_layer_entry.move_layer_delta = std::fabs(record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate - record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate) * 1000.f;
		memcpy(resolver_layer_entry.test_layers, record->player->GetAnimlayers(), sizeof(AnimationLayer) * 13);
	}
	else {
		resolver_layer_entry.move_layer_delta = 999.f;
	}

	*animstate = *AnimationSystem->GetUnupdatedAnimstate(record->player->EntIndex());
}

void CResolver::SetupResolverLayers(CBasePlayer* player, LagRecord* record) {
	if (!player || !record) return;
	SetupLayer(record, 0, 0.f);
	SetupLayer(record, 1, record->resolver_data.max_desync_delta);
	SetupLayer(record, 2, -record->resolver_data.max_desync_delta);
	SetupLayer(record, 3, record->resolver_data.max_desync_delta * 0.5f);
	SetupLayer(record, 4, -record->resolver_data.max_desync_delta * 0.5f);
}

void CResolver::DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records) {
	if (!player || !record || !Cheat.LocalPlayer || records.size() < 2) {
		if (record) {
			record->resolver_data.resolver_type = ResolverType::NONE;
			record->resolver_data.side = 0;
		}
		return;
	}

	Vector eyePos = player->GetEyePosition(); // Poprawka
	float notModifiedYaw = record->m_angEyeAngles.yaw;

	if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC && records.size() >= 4) {
		notModifiedYaw = FindAvgYaw(records);
	}

	Vector right_dir, left_dir, dummy_forward, dummy_up; // Poprawka
	Math::AngleVectors(QAngle(0.f, notModifiedYaw + 90.f, 0.f), dummy_forward, right_dir, dummy_up); // Poprawka
	Math::AngleVectors(QAngle(0.f, notModifiedYaw - 90.f, 0.f), dummy_forward, left_dir, dummy_up);  // Poprawka

	float wall_dist = 28.f;
	CTraceFilter filter;
	filter.pSkip = player;

	Ray_t ray_left(eyePos, eyePos + left_dir * wall_dist);
	Ray_t ray_right(eyePos, eyePos + right_dir * wall_dist);
	CGameTrace trace_left, trace_right;

	EngineTrace->TraceRay(ray_left, MASK_SOLID_BRUSHONLY, &filter, &trace_left);
	EngineTrace->TraceRay(ray_right, MASK_SOLID_BRUSHONLY, &filter, &trace_right);

	bool left_solid = trace_left.DidHit() && trace_left.fraction < 1.0f;
	bool right_solid = trace_right.DidHit() && trace_right.fraction < 1.0f;

	if (left_solid && !right_solid) {
		record->resolver_data.side = 1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
	}
	else if (!left_solid && right_solid) {
		record->resolver_data.side = -1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
	}
	else if (left_solid && right_solid) {
		record->resolver_data.side = (trace_left.fraction > trace_right.fraction) ? 1 : -1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
	}
	else {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
	}
}

void CResolver::Apply(LagRecord* record) {
	if (!record || !record->player) return;
	if (record->resolver_data.side != 0) {
		float body_yaw_offset = record->resolver_data.max_desync_delta * static_cast<float>(record->resolver_data.side);
		CCSGOPlayerAnimationState* state = record->player->GetAnimstate();
		if (!state) return;
		state->flFootYaw = Math::AngleNormalize(state->flEyeYaw + body_yaw_offset);
	}
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records) {
	if (!player || !record || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive()) return;
	if (GameRules()->IsFreezePeriod() || player->m_fFlags() & FL_FROZEN || record->shooting) {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}

	record->resolver_data.max_desync_delta = player->GetMaxDesyncDelta();
	if (record->resolver_data.max_desync_delta == 0.f) record->resolver_data.max_desync_delta = 58.f;

	if (record->m_nChokedTicks == 0 || player->m_bIsDefusing()) {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}

	record->resolver_data.player_state = DetectPlayerState(player, record->animlayers);
	record->resolver_data.antiaim_type = DetectAntiAim(player, records);

	SetupResolverLayers(player, record);
	record->resolver_data.resolver_type = ResolverType::NONE;
	record->resolver_data.side = 0;

	float min_move_layer_delta = 1000.f;
	int initial_anim_side = 0;
	for (const auto& layer_data : record->resolver_data.layers) {
		if (layer_data.move_layer_delta < min_move_layer_delta) {
			min_move_layer_delta = layer_data.move_layer_delta;
			if (layer_data.desync != 0.f) {
				initial_anim_side = layer_data.desync < 0.f ? -1 : 1;
			}
		}
	}

	if (min_move_layer_delta <= 10.f && initial_anim_side != 0) {
		record->resolver_data.side = initial_anim_side;
		record->resolver_data.resolver_type = ResolverType::ANIM;
	}
	else {
		record->resolver_data.resolver_type = ResolverType::NONE;
		record->resolver_data.side = 0;
	}

	if (record->resolver_data.resolver_type == ResolverType::ANIM || record->resolver_data.resolver_type == ResolverType::NONE) {
		int best_side_from_logic = 0;
		float highest_confidence = -1.f;
		const float WEIGHT_STABLE_ADJUST = 1.0f;
		const float WEIGHT_MOVE_LAYER_CONSISTENCY_MOVING = 0.7f;
		const float PENALTY_MOVE_LAYER_INCONSISTENCY_MOVING = -0.7f;
		const float WEIGHT_MOVE_LAYER_CONSISTENCY_STANDING = 0.6f;
		const float WEIGHT_LEAN_CONSISTENCY = 0.3f;
		const float CONFIDENCE_THRESHOLD_FOR_OVERRIDE = 0.5f;
		const float MIN_DELTA_FOR_ANIM_WEAKNESS = 8.f;

		for (const auto& current_tested_state : record->resolver_data.layers) {
			float current_state_confidence = 0.f;
			const AnimationLayer& adj_layer = current_tested_state.test_layers[ANIMATION_LAYER_ADJUST];
			if (adj_layer.m_flWeight > 0.65f && adj_layer.m_flPlaybackRate < 0.05f && adj_layer.m_flCycle > 0.90f) {
				current_state_confidence += WEIGHT_STABLE_ADJUST;
			}

			const AnimationLayer& lean_layer = current_tested_state.test_layers[ANIMATION_LAYER_LEAN];
			if (lean_layer.m_flWeight > 0.01f && lean_layer.m_flCycle > 0.01f && lean_layer.m_flCycle < 0.99f) {
				if ((current_tested_state.desync > 0 && lean_layer.m_flCycle > 0.5f) || (current_tested_state.desync < 0 && lean_layer.m_flCycle < 0.5f)) {
					current_state_confidence += WEIGHT_LEAN_CONSISTENCY;
				}
				else if (current_tested_state.desync == 0.f && (lean_layer.m_flCycle > 0.45f && lean_layer.m_flCycle < 0.55f)) {
					current_state_confidence += WEIGHT_LEAN_CONSISTENCY * 0.5f;
				}
			}

			const AnimationLayer& move_layer = current_tested_state.test_layers[ANIMATION_LAYER_MOVEMENT_MOVE];
			float player_velocity_norm = record->m_vecVelocity.Length2D();

			if (player_velocity_norm > 1.0f) {
				if (move_layer.m_flWeight > 0.05f && move_layer.m_flPlaybackRate > 0.05f) {
					current_state_confidence += WEIGHT_MOVE_LAYER_CONSISTENCY_MOVING;
				}
				else {
					current_state_confidence += PENALTY_MOVE_LAYER_INCONSISTENCY_MOVING;
				}
			}
			else {
				if (move_layer.m_flWeight < 0.1f || move_layer.m_flPlaybackRate < 0.05f) {
					current_state_confidence += WEIGHT_MOVE_LAYER_CONSISTENCY_STANDING;
				}
			}

			if (current_state_confidence > highest_confidence) {
				highest_confidence = current_state_confidence;
				best_side_from_logic = (current_tested_state.desync == 0.f) ? 0 : (current_tested_state.desync < 0.f ? -1 : 1);
			}
		}

		bool anim_resolver_was_weak = (record->resolver_data.resolver_type == ResolverType::ANIM && min_move_layer_delta > MIN_DELTA_FOR_ANIM_WEAKNESS);
		bool anim_resolver_completely_failed = (record->resolver_data.resolver_type == ResolverType::NONE);

		if (highest_confidence >= CONFIDENCE_THRESHOLD_FOR_OVERRIDE) {
			if (anim_resolver_completely_failed || anim_resolver_was_weak || best_side_from_logic != record->resolver_data.side) {
				record->resolver_data.side = best_side_from_logic;
				record->resolver_data.resolver_type = ResolverType::LOGIC;
			}
		}
		else if (anim_resolver_completely_failed) {
			record->resolver_data.resolver_type = ResolverType::NONE;
			record->resolver_data.side = 0;
		}
	}

	if (record->resolver_data.resolver_type == ResolverType::NONE ||
		(record->resolver_data.resolver_type == ResolverType::ANIM && min_move_layer_delta > 8.f)) {
		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && records.size() >= 5) {
			float eyeYaw = player->m_angEyeAngles().yaw;
			float avgPrevEyeYaw = FindAvgYaw(records);
			float delta_from_avg = Math::AngleDiff(eyeYaw, avgPrevEyeYaw);
			if (std::abs(delta_from_avg) > 5.f) {
				record->resolver_data.side = (delta_from_avg < 0.f) ? 1 : -1;
				record->resolver_data.resolver_type = ResolverType::LOGIC;
			}
			else {
				DetectFreestand(player, record, records);
			}
		}
		else {
			DetectFreestand(player, record, records);
		}
	}

	int player_idx = player->EntIndex();
	if (player_idx < 0 || player_idx >= 65) return;
	auto res_data_ptr = &resolver_data[player_idx];
	float curtime = GetTime();

	if (record->resolver_data.resolver_type == ResolverType::NONE) {
		if (curtime - res_data_ptr->last_resolved < 1.5f && res_data_ptr->last_side != 0) {
			record->resolver_data.resolver_type = ResolverType::MEMORY;
			record->resolver_data.side = res_data_ptr->last_side;
		}
	}

	if (record->resolver_data.resolver_type != ResolverType::NONE &&
		record->resolver_data.resolver_type != ResolverType::MEMORY &&
		record->resolver_data.resolver_type != ResolverType::BRUTEFORCE &&
		record->resolver_data.resolver_type != ResolverType::DEFAULT) {
		res_data_ptr->last_resolved = curtime;
		res_data_ptr->last_side = record->resolver_data.side;
		res_data_ptr->res_type_last = record->resolver_data.resolver_type;
	}

	if (curtime - res_data_ptr->brute_time < 2.0f && res_data_ptr->brute_side != 0) {
		if (record->resolver_data.resolver_type == ResolverType::NONE ||
			record->resolver_data.resolver_type == ResolverType::MEMORY ||
			record->resolver_data.resolver_type == ResolverType::DEFAULT ||
			(record->resolver_data.resolver_type == ResolverType::ANIM && min_move_layer_delta > 9.f)
			) {
			record->resolver_data.side = res_data_ptr->brute_side;
			record->resolver_data.resolver_type = ResolverType::BRUTEFORCE;
		}
	}

	if (record->resolver_data.side == 0 && record->resolver_data.resolver_type == ResolverType::NONE) {
		record->resolver_data.side = (res_data_ptr->missed_shots % 2 == 0) ? -1 : 1;
		record->resolver_data.resolver_type = ResolverType::DEFAULT;
	}
	Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record) {
	if (!player || !record) return;
	int player_idx = player->EntIndex();
	if (player_idx < 0 || player_idx >= 65) return;
	auto bf_data = &resolver_data[player_idx];
	bf_data->missed_shots++;

	int current_side_on_miss = record->resolver_data.side;

	if (bf_data->missed_shots % 2 == 0 || record->resolver_data.resolver_type == ResolverType::MEMORY || current_side_on_miss == 0) {
		bf_data->brute_side = (current_side_on_miss == 0) ? ((bf_data->missed_shots % 4 < 2) ? -1 : 1) : -current_side_on_miss;
	}
	else {
		bf_data->brute_side = -current_side_on_miss;
	}
	bf_data->brute_time = GetTime();
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record) {
	if (!player || !record) return;
	int player_idx = player->EntIndex();
	if (player_idx < 0 || player_idx >= 65) return;
	auto hit_data = &resolver_data[player_idx];
	hit_data->last_resolved = GetTime();
	hit_data->last_side = record->resolver_data.side;
	hit_data->res_type_last = record->resolver_data.resolver_type;
	hit_data->missed_shots = 0;
	hit_data->brute_time = 0.f;
}