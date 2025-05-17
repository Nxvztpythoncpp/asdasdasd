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
#include "../../Utils/Math.h"

CResolver* Resolver = new CResolver;

float CResolver::GetTime() {
	if (!Cheat.LocalPlayer)
		return GlobalVars->curtime;
	return TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
}

float FindAvgYaw(const std::deque<LagRecord>& records,
	size_t max_records_to_use = 7,
	float recency_bias_factor = 0.65f)
{
	if (records.empty()) {
		return 0.f;
	}
	size_t num_records_available = records.size();
	size_t records_to_process = (std::min)(max_records_to_use, num_records_available);
	if (records_to_process == 0) {
		return 0.f;
	}
	if (records_to_process == 1 || recency_bias_factor >= 0.999f) {
		return records.front().m_angEyeAngles.yaw;
	}
	float sin_sum = 0.f;
	float cos_sum = 0.f;
	float total_weight = 0.f;
	float current_record_weight = 1.0f;
	for (size_t i = 0; i < records_to_process; ++i) {
		const LagRecord& record = records[i];
		float eyeYaw = record.m_angEyeAngles.yaw;
		sin_sum += current_record_weight * std::sinf(DEG2RAD(eyeYaw));
		cos_sum += current_record_weight * std::cosf(DEG2RAD(eyeYaw));
		total_weight += current_record_weight;
		if (i < records_to_process - 1) {
			current_record_weight *= (1.0f - recency_bias_factor);
			if (current_record_weight < 0.01f && recency_bias_factor < 0.95f) {
				current_record_weight = 0.01f;
			}
			else if (recency_bias_factor >= 0.95f && current_record_weight < 0.0001f) {
				current_record_weight = 0.f;
			}
		}
	}
	if (total_weight < 0.0001f) {
		return records.front().m_angEyeAngles.yaw;
	}
	return RAD2DEG(std::atan2f(sin_sum / total_weight, cos_sum / total_weight));
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

	auto& p_data = resolver_data[player->EntIndex()];
	p_data.jitter_pattern_detected = false; 

	const size_t min_records_for_analysis = 3;
	if (records.size() < min_records_for_analysis) {
		return R_AntiAimType::NONE;
	}

	const LagRecord& current_record = records.front();
	if (p_data.lby_updated_this_tick) {
		return R_AntiAimType::LBY_BREAK;
	}

	
	if (records.size() >= 3) {
		float yaw_diff1 = Math::AngleDiff(records[0].m_angEyeAngles.yaw, records[1].m_angEyeAngles.yaw);
		float yaw_diff2 = Math::AngleDiff(records[1].m_angEyeAngles.yaw, records[2].m_angEyeAngles.yaw);
		const float spin_threshold = 45.f;

		bool is_spinning = false;
		if (std::fabs(yaw_diff1) > spin_threshold && std::fabs(yaw_diff2) > spin_threshold) {
			if ((yaw_diff1 > 0 && yaw_diff2 > 0) || (yaw_diff1 < 0 && yaw_diff2 < 0)) {
				if (records.size() >= 4) {
					float yaw_diff3 = Math::AngleDiff(records[2].m_angEyeAngles.yaw, records[3].m_angEyeAngles.yaw);
					if (std::fabs(yaw_diff3) > spin_threshold && ((yaw_diff2 > 0 && yaw_diff3 > 0) || (yaw_diff2 < 0 && yaw_diff3 < 0))) {
						is_spinning = true;
					}
				}
				else {
					is_spinning = true;
				}
			}
		}
		if (is_spinning) {
			return R_AntiAimType::SPIN;
		}
	}

	
	bool lby_is_considered_stable = (current_record.m_flSimulationTime - p_data.last_lby_update_time > 0.22f) &&
		(current_record.m_flSimulationTime - p_data.last_lby_update_time < 1.0f) &&
		!p_data.lby_updated_this_tick;

	if (lby_is_considered_stable && records.size() >= 2) {
		if (std::fabs(Math::AngleDiff(records.front().player->m_flLowerBodyYawTarget(), records[1].player->m_flLowerBodyYawTarget())) > 1.f) {
			lby_is_considered_stable = false;
		}
	}

	
	if (lby_is_considered_stable) {
		float desync_delta = Math::AngleDiff(current_record.m_angEyeAngles.yaw, p_data.last_lby);
		const float min_static_desync_threshold = 25.f;
		const float max_static_desync_threshold = 65.f;

		if (std::fabs(desync_delta) >= min_static_desync_threshold && std::fabs(desync_delta) <= max_static_desync_threshold) {
			return R_AntiAimType::STATIC_DESYNC;
		}
	}

	
	const float jitter_angle_equality_tolerance = 8.f;
	const float min_jitter_separation_dist = 20.f;

	
	if (records.size() >= 3) {
		float y0 = records[0].m_angEyeAngles.yaw;
		float y1 = records[1].m_angEyeAngles.yaw; 
		float y2 = records[2].m_angEyeAngles.yaw;

		bool y0_matches_y2 = std::fabs(Math::AngleDiff(y0, y2)) < jitter_angle_equality_tolerance;
		bool y0_distinct_from_y1 = std::fabs(Math::AngleDiff(y0, y1)) > min_jitter_separation_dist;

		if (y0_matches_y2 && y0_distinct_from_y1) {
			p_data.jitter_pattern_detected = true;
			p_data.jitter_angle1 = Math::AngleNormalize(y0); 
			p_data.jitter_angle2 = Math::AngleNormalize(y1); 
			
			return R_AntiAimType::JITTER;
		}
	}

	
	if (!p_data.jitter_pattern_detected && records.size() >= 4) {
		float y0 = records[0].m_angEyeAngles.yaw; // A 
		float y1 = records[1].m_angEyeAngles.yaw; // B
		float y2 = records[2].m_angEyeAngles.yaw; // A'
		float y3 = records[3].m_angEyeAngles.yaw; // B'

		bool y0_matches_y2 = std::fabs(Math::AngleDiff(y0, y2)) < jitter_angle_equality_tolerance;
		bool y1_matches_y3 = std::fabs(Math::AngleDiff(y1, y3)) < jitter_angle_equality_tolerance;
		bool y0_distinct_from_y1 = std::fabs(Math::AngleDiff(y0, y1)) > min_jitter_separation_dist;

		if (y0_matches_y2 && y1_matches_y3 && y0_distinct_from_y1) {
			p_data.jitter_pattern_detected = true;
			p_data.jitter_angle1 = Math::AngleNormalize(y0); 
			p_data.jitter_angle2 = Math::AngleNormalize(y1); 
			
			return R_AntiAimType::JITTER;
		}
	}

	if (records.size() >= 3) {
		int jittered_records_count_fallback = 0;
		float total_delta_sum_fallback = 0.f;
		int considered_records_fallback = 0;
		
		size_t max_deltas_generic = (std::min)(records.size() - 1, static_cast<size_t>(2));

		for (size_t i = 0; i < max_deltas_generic; ++i) {
			const LagRecord& rec1 = records[i];       
			const LagRecord& rec2 = records[i + 1];   
			float delta = std::fabs(Math::AngleDiff(rec1.m_angEyeAngles.yaw, rec2.m_angEyeAngles.yaw));
			total_delta_sum_fallback += delta;
			considered_records_fallback++;
			if (delta > 30.f) {
				jittered_records_count_fallback++;
			}
		}

		if (considered_records_fallback > 0) {
			float average_delta_fallback = total_delta_sum_fallback / static_cast<float>(considered_records_fallback);
			
			if (!p_data.jitter_pattern_detected && jittered_records_count_fallback >= considered_records_fallback / 2 && average_delta_fallback > 20.f) {
				
				return R_AntiAimType::JITTER;
			}

			
			if (average_delta_fallback < 15.f && jittered_records_count_fallback == 0) {
				if (lby_is_considered_stable) {
					
					if (std::fabs(Math::AngleDiff(current_record.m_angEyeAngles.yaw, p_data.last_lby)) <= 20.f) {
						
						return R_AntiAimType::STATIC;
					}
					
				}
				else { 
					return R_AntiAimType::STATIC;
				}
			}
		}
	}
	

	return R_AntiAimType::UNKNOWN;
}

float CResolver::ValveAngleDiff(float destAngle, float srcAngle) {
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

	Vector eyePos = player->GetEyePosition();
	float notModifiedYaw = record->m_angEyeAngles.yaw;

	if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC && record->resolver_data.antiaim_type != R_AntiAimType::LBY_BREAK && records.size() >= 4) {
		notModifiedYaw = FindAvgYaw(records, 4, 0.6f);
	}

	Vector right_dir, left_dir, dummy_forward, dummy_up;
	Math::AngleVectors(QAngle(0.f, notModifiedYaw + 90.f, 0.f), dummy_forward, right_dir, dummy_up);
	Math::AngleVectors(QAngle(0.f, notModifiedYaw - 90.f, 0.f), dummy_forward, left_dir, dummy_up);

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
	if (!record || !record->player) {
		return;
	}
	CCSGOPlayerAnimationState* animstate = record->player->GetAnimstate();
	if (!animstate) {
		return;
	}

	
	if (record->resolver_data.side != 0 &&
		record->resolver_data.resolver_type != ResolverType::NONE &&
		record->resolver_data.resolver_type != ResolverType::DEFAULT &&
		record->resolver_data.resolver_type != ResolverType::MEMORY &&
		record->resolver_data.resolver_type != ResolverType::BRUTEFORCE) {

		float resolved_desync_angle = static_cast<float>(record->resolver_data.side) * record->resolver_data.max_desync_delta;
		
		auto& p_data = resolver_data[record->player->EntIndex()];
		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && p_data.jitter_pattern_detected) {
	
			if (p_data.last_resolved_jitter_angle_idx == 0) {
				resolved_desync_angle = Math::AngleDiff(p_data.jitter_angle1, animstate->flEyeYaw); 
				p_data.last_resolved_jitter_angle_idx = 1;
			}
			else {
				resolved_desync_angle = Math::AngleDiff(p_data.jitter_angle2, animstate->flEyeYaw);
				p_data.last_resolved_jitter_angle_idx = 0;
			}
			
			resolved_desync_angle = Math::AngleNormalize(resolved_desync_angle);
		}


		float new_foot_yaw = Math::AngleNormalize(animstate->flEyeYaw + resolved_desync_angle);
		animstate->flFootYaw = new_foot_yaw;
		
	}
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records) {

	auto& p_data = resolver_data[player->EntIndex()];

	
	p_data.lby_updated_this_tick = false;
	if (record->player->m_flLowerBodyYawTarget() != p_data.last_lby) { 
		if (record->m_flSimulationTime > p_data.last_lby_update_time) {
			p_data.lby_updated_this_tick = true;
			p_data.last_lby = record->player->m_flLowerBodyYawTarget();
			p_data.last_lby_update_time = record->m_flSimulationTime;
		}
	}

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
		
		const float CONFIDENCE_THRESHOLD_FOR_OVERRIDE = 0.5f;
		const float MIN_DELTA_FOR_ANIM_WEAKNESS = 8.f;
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
		(record->resolver_data.resolver_type == ResolverType::ANIM && min_move_layer_delta > 8.f)) { // Próg 8.f

		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER) {
			if (p_data.jitter_pattern_detected) {
				
				record->resolver_data.side = (p_data.missed_shots % 2 == 0) ? 1 : -1; 
				
				record->resolver_data.resolver_type = ResolverType::LOGIC;
				
			}
			else {
				if (records.size() >= 5) {
					float eyeYaw = player->m_angEyeAngles().yaw; 
					float avgPrevEyeYaw = FindAvgYaw(records, 5, 0.5f);
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
		}
		else { 
			DetectFreestand(player, record, records);
		}
	}

	
	int player_idx = player->EntIndex();
	
	float curtime = GetTime();

	if (record->resolver_data.resolver_type == ResolverType::NONE) { 
		if (curtime - p_data.last_resolved < 1.5f && p_data.last_side != 0) { 
			record->resolver_data.resolver_type = ResolverType::MEMORY;
			record->resolver_data.side = p_data.last_side;
			
		}
	}

	
	if (record->resolver_data.resolver_type != ResolverType::NONE &&
		record->resolver_data.resolver_type != ResolverType::MEMORY &&
		record->resolver_data.resolver_type != ResolverType::BRUTEFORCE &&
		record->resolver_data.resolver_type != ResolverType::DEFAULT) {
		p_data.last_resolved = curtime;
		p_data.last_side = record->resolver_data.side;
		p_data.res_type_last = record->resolver_data.resolver_type;
	}

	if (curtime - p_data.brute_time < 2.0f && p_data.brute_side != 0) {
		
		if (record->resolver_data.resolver_type == ResolverType::NONE ||
			record->resolver_data.resolver_type == ResolverType::MEMORY ||
			record->resolver_data.resolver_type == ResolverType::DEFAULT ||
			(record->resolver_data.resolver_type == ResolverType::ANIM && min_move_layer_delta > 9.f)) {
			record->resolver_data.side = p_data.brute_side;
			record->resolver_data.resolver_type = ResolverType::BRUTEFORCE;
			
		}
	}

	
	if (record->resolver_data.side == 0 && record->resolver_data.resolver_type == ResolverType::NONE) {
		record->resolver_data.side = (p_data.missed_shots % 2 == 0) ? -1 : 1; 
		record->resolver_data.resolver_type = ResolverType::DEFAULT;
		
	}

	
	Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record) {
	if (!player || !record) return;
	int player_idx = player->EntIndex();
	if (player_idx < 0 || player_idx >= 65) return; 
	auto& bf_data = resolver_data[player_idx];
	bf_data.missed_shots++;

	int current_side_on_miss = record->resolver_data.side; 

	
	if (bf_data.missed_shots % 2 == 0 || record->resolver_data.resolver_type == ResolverType::MEMORY || current_side_on_miss == 0) {
		bf_data.brute_side = (current_side_on_miss == 0) ? ((bf_data.missed_shots % 4 < 2) ? -1 : 1) : -current_side_on_miss;
	}
	else {
		bf_data.brute_side = -current_side_on_miss;
	}
	bf_data.brute_time = GetTime();
	
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record) {
	if (!player || !record) return;
	int player_idx = player->EntIndex();
	if (player_idx < 0 || player_idx >= 65) return;
	auto& hit_data = resolver_data[player_idx]; 

	
	hit_data.last_resolved = GetTime();
	hit_data.last_side = record->resolver_data.side;
	hit_data.res_type_last = record->resolver_data.resolver_type;
	hit_data.missed_shots = 0; // Resetuj licznik missów!
	hit_data.brute_time = 0.f; // Zresetuj czas bruteforce'u, bo trafiliœmy
	// Console::Log("[Resolver] Player %d: ON HIT! Side: %d, Type: %d\n", player_idx, hit_data.last_side, static_cast<int>(hit_data.res_type_last));
}