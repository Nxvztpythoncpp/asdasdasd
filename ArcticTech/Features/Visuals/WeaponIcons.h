#pragma once

#include "../../SDK/Render.h"
#include "../../SDK/Misc/CBaseCombatWeapon.h"

class CWeaponIcon {
	DXImage weap_ak47;
	DXImage weap_ammobox;
	DXImage weap_ammobox_threepack;
	DXImage weap_armor;
	DXImage weap_armor_helmet;
	DXImage weap_assaultsuit;
	DXImage weap_aug;
	DXImage weap_awp;
	DXImage weap_axe;
	DXImage weap_bayonet;
	DXImage weap_bizon;
	DXImage weap_breachcharge;
	DXImage weap_breachcharge_projectile;
	DXImage weap_bumpmine;
	DXImage weap_c4;
	DXImage weap_controldrone;
	DXImage weap_cz75a;
	DXImage weap_deagle;
	DXImage weap_decoy;
	DXImage weap_defuser;
	DXImage weap_diversion;
	DXImage weap_dronegun;
	DXImage weap_elite;
	DXImage weap_famas;
	DXImage weap_firebomb;
	DXImage weap_fists;
	DXImage weap_fiveseven;
	DXImage weap_flashbang;
	DXImage weap_flashbang_assist;
	DXImage weap_frag_grenade;
	DXImage weap_g3sg1;
	DXImage weap_galilar;
	DXImage weap_glock;
	DXImage weap_grenadepack;
	DXImage weap_grenadepack2;
	DXImage weap_hammer;
	DXImage weap_healthshot;
	DXImage weap_heavy_armor;
	DXImage weap_hegrenade;
	DXImage weap_helmet;
	DXImage weap_hkp2000;
	DXImage weap_incgrenade;
	DXImage weap_inferno;
	DXImage weap_kevlar;
	DXImage weap_knife;
	DXImage weap_knifegg;
	DXImage weap_knife_bowie;
	DXImage weap_knife_butterfly;
	DXImage weap_knife_canis;
	DXImage weap_knife_cord;
	DXImage weap_knife_css;
	DXImage weap_knife_falchion;
	DXImage weap_knife_flip;
	DXImage weap_knife_gut;
	DXImage weap_knife_gypsy_jackknife;
	DXImage weap_knife_karambit;
	DXImage weap_knife_m9_bayonet;
	DXImage weap_knife_outdoor;
	DXImage weap_knife_push;
	DXImage weap_knife_skeleton;
	DXImage weap_knife_stiletto;
	DXImage weap_knife_survival_bowie;
	DXImage weap_knife_t;
	DXImage weap_knife_tactical;
	DXImage weap_knife_ursus;
	DXImage weap_knife_widowmaker;
	DXImage weap_m249;
	DXImage weap_m4a1;
	DXImage weap_m4a1_silencer;
	DXImage weap_m4a1_silencer_off;
	DXImage weap_mac10;
	DXImage weap_mag7;
	DXImage weap_molotov;
	DXImage weap_mp5sd;
	DXImage weap_mp7;
	DXImage weap_mp9;
	DXImage weap_negev;
	DXImage weap_nova;
	DXImage weap_p2000;
	DXImage weap_p250;
	DXImage weap_p90;
	DXImage weap_planted_c4;
	DXImage weap_planted_c4_survival;
	DXImage weap_prop_exploding_barrel;
	DXImage weap_radarjammer;
	DXImage weap_revolver;
	DXImage weap_sawedoff;
	DXImage weap_scar20;
	DXImage weap_sg556;
	DXImage weap_shield;
	DXImage weap_smokegrenade;
	DXImage weap_snowball;
	DXImage weap_spanner;
	DXImage weap_ssg08;
	DXImage weap_stomp_damage;
	DXImage weap_tablet;
	DXImage weap_tagrenade;
	DXImage weap_taser;
	DXImage weap_tec9;
	DXImage weap_ump45;
	DXImage weap_usp_silencer;
	DXImage weap_usp_silencer_off;
	DXImage weap_xm1014;
	DXImage weap_zone_repulsor;

public:
	void Setup();

	DXImage& GetIcon(int weapon_id);
};

extern std::unique_ptr<CWeaponIcon> WeaponIcons;