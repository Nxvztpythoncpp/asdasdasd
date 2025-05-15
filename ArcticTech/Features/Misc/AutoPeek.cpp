#include "AutoPeek.h"
#include "Prediction.h"
#include "../../SDK/Globals.h"
#include "../../SDK/Render.h"

inline std::unique_ptr<CAutoPeek> AutoPeek = std::make_unique<CAutoPeek>();

static float box_time = 0.f;
void CAutoPeek::Draw() {
	static bool prev_state = false;

	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !Cheat.InGame)
		return;

	bool state = config.ragebot.aimbot.peek_assist->get() && config.ragebot.aimbot.peek_assist_keybind->get();

	if (prev_state != state) {
		if (state) {
			if (Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) {
				position = Cheat.LocalPlayer->m_vecOrigin();
			}
			else {
				CGameTrace trace = EngineTrace->TraceHull(Cheat.LocalPlayer->m_vecOrigin(), Cheat.LocalPlayer->m_vecOrigin() - Vector(0, 0, 128), Cheat.LocalPlayer->m_vecMins(), Cheat.LocalPlayer->m_vecMaxs(), CONTENTS_SOLID, Cheat.LocalPlayer);

				position = trace.endpos;
			}
		}

		prev_state = state;
	}

	const auto track_glowies = [this](const Vector visual_pos)
		{
			for (auto n = GlowObjectManager->m_GlowBoxDefinitions.Count() - 1; n >= 0; n--)
				if (GlowObjectManager->m_GlowBoxDefinitions[n].m_flBirthTimeIndex == box_time)
					GlowObjectManager->m_GlowBoxDefinitions.RemoveAll();

			constexpr auto radius = 36;
			const auto clr = config.ragebot.aimbot.peek_assist_color->get();
			auto last_seg = visual_pos;
			for (auto i = 0; i < radius + 1; i++)
			{
				const auto x = visual_pos.x + .33f * radius * cosf(i * 2.f * M_PI / radius);
				const auto y = visual_pos.y - .33f * radius * sinf(i * 2.f * M_PI / radius);
				const auto seg = Vector{ x, y, visual_pos.z };
				Vector delta_pos(last_seg - seg);
				if (delta_pos.Length2D() < .25f * .33f * radius)
				{
					const auto deg = RAD2DEG(atan2f(delta_pos.y, delta_pos.x));
					QAngle trajectory{ 0.f, deg - floorf(deg / 360.0f + .5f) * 360.0f, 0.f };
					GlowObjectManager->AddGlowBox(seg, trajectory, { 0.f, -.2f, -.2f }, { delta_pos.Length(), .2f, .2f }, clr, .5f);
				}
				last_seg = seg;
			}

			box_time = GlobalVars->curtime - .25f;
		};

	if (state)
	{
		if (Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND)
		{
			track_glowies(position + Vector{ 0.f, 0.f, 1.f });
		}
	}
}

void CAutoPeek::CreateMove() {
	if (!(config.ragebot.aimbot.peek_assist_keybind->get()) || !ctx.active_weapon) {
		returning = false;
		return;
	}

	float distance_sqr = (Cheat.LocalPlayer->m_vecOrigin() - position).LengthSqr();

	if (ctx.cmd->buttons & IN_ATTACK && ctx.active_weapon->ShootingWeapon() && ctx.active_weapon->CanShoot()) {
		Return();
	}
	else if (distance_sqr < 16.f) {
		if (ctx.active_weapon->m_flNextPrimaryAttack() - 0.15f < TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()))
			returning = false;
	}

	if (!returning && (!config.ragebot.aimbot.peek_assist_on_release->get() || distance_sqr < 16.f || ctx.cmd->buttons & (IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK)))
		return;

	if (returning) {
		block_buttons &= ctx.cmd->buttons; // if player will release buttons, they will not be blocked
		ctx.cmd->buttons &= ~block_buttons;

		if (block_buttons & IN_FORWARD && ctx.cmd->forwardmove > 0.f)
			ctx.cmd->forwardmove = 0.f;

		if (block_buttons & IN_BACK && ctx.cmd->forwardmove < 0.f)
			ctx.cmd->forwardmove = 0.f;

		if (block_buttons & IN_MOVELEFT && ctx.cmd->sidemove < 0.f)
			ctx.cmd->sidemove = 0.f;

		if (block_buttons & IN_MOVERIGHT && ctx.cmd->sidemove > 0.f)
			ctx.cmd->sidemove = 0.f;

		if (distance_sqr < 16.f) {
			if (block_buttons == 0)
				returning = false;
			return;
		}
	}

	QAngle ang = Utils::VectorToAngle(Cheat.LocalPlayer->m_vecOrigin(), position);
	QAngle vang;
	EngineClient->GetViewAngles(vang);
	ang.yaw = vang.yaw - ang.yaw;
	ang.Normalize();

	Vector dir;
	Utils::AngleVectors(ang, dir);
	dir *= 450.f;
	ctx.cmd->forwardmove = dir.x;
	ctx.cmd->sidemove = dir.y;
}

void CAutoPeek::Return() {
	block_buttons = ctx.cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
	returning = true;
}