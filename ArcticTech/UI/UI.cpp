#include "UI.h"
#include "../SDK/Config.h"

#include "../Features/Visuals/SkinChanger.h"
#include "../Features/Visuals/World.h"

void CMenu::SetupUI() {
	AddTab("Aimbot", pic::tab::aimbot);
	AddTab("Anti aim", pic::tab::antiaim);
	AddTab("Player", pic::tab::players);
	AddTab("Visuals", pic::tab::visuals);
	AddTab("Misc", pic::tab::misc);
	AddTab("Skins", pic::tab::skins);
	AddTab("Config", pic::tab::configs);
	//AddTab("Scripts", pic::tab::scripts);

	auto aimbot = AddGroupBox("Aimbot", "Aimbot");
	auto aim_settings = AddGroupBox("Aimbot", "Settings");

	auto aa_angles = AddGroupBox("Anti aim", "Angles");
	auto fake_lag = AddGroupBox("Anti aim", "Fake lag", 0.8f, 1);
	auto aa_other = AddGroupBox("Anti aim", "Other", 1.f, 1);

	auto player_esp = AddGroupBox("Player", "ESP");
	auto chams = AddGroupBox("Player", "Chams");

	auto other_esp = AddGroupBox("Visuals", "Other ESP");
	auto effects = AddGroupBox("Visuals", "Effects");

	auto misc = AddGroupBox("Misc", "Miscellaneous");
	auto movement = AddGroupBox("Misc", "Movement");
	auto ui_group = AddGroupBox("Misc", "UI");

	auto skins = AddGroupBox("Skins", "Skins");
	auto models = AddGroupBox("Skins", "Models");

	auto configs = AddGroupBox("Config", "Config");

	auto scripts = AddGroupBox("Config", "Scripts");

	config.ragebot.aimbot.enabled = aimbot->AddCheckBox("Enabled");
	config.ragebot.aimbot.extrapolation = aimbot->AddComboBox("Extrapolation", { "Disable", "Enable", "Force" });
	config.ragebot.aimbot.dormant_aim = aimbot->AddCheckBox("Dormant aim");
	config.ragebot.aimbot.dormant_aim_key = aimbot->AddKeyBind("Dormant aim");
	config.ragebot.aimbot.extended_backtrack = aimbot->AddCheckBox("Extended backtrack");
	config.ragebot.aimbot.doubletap = aimbot->AddCheckBox("Double Tap");
	config.ragebot.aimbot.doubletap_key = aimbot->AddKeyBind("Double Tap");
	config.ragebot.aimbot.doubletap_options = aimbot->AddMultiCombo("Double Tap options", { "Break LC", "Lag Peek", "Immediate teleport", "Fast throw", "Recharge on error"});
	config.ragebot.aimbot.hide_shots = aimbot->AddCheckBox("Hide Shots");
	config.ragebot.aimbot.hide_shots_key = aimbot->AddKeyBind("Hide Shots");
	config.ragebot.aimbot.hideshots_options = aimbot->AddComboBox("Hide Shots options", {"Favor Fire Rate", "Favor Fake Lag"});
	config.ragebot.aimbot.force_teleport = aimbot->AddKeyBind("Force Teleport");
	config.ragebot.aimbot.force_body_aim = aimbot->AddKeyBind("Force Body Aim");
	config.ragebot.aimbot.force_safepoint = aimbot->AddKeyBind("Force safepoint");
	config.ragebot.aimbot.minimum_damage_override_key = aimbot->AddKeyBind("Min. damage override");
	config.ragebot.aimbot.peek_assist = aimbot->AddCheckBox("Peek Assist");
	config.ragebot.aimbot.peek_assist_color = aimbot->AddColorPicker("Peek Assist");
	config.ragebot.aimbot.peek_assist_keybind = aimbot->AddKeyBind("Peek Assist key");
	config.ragebot.aimbot.peek_assist_on_release = aimbot->AddCheckBox("Retreat on key release");
	config.ragebot.aimbot.show_aimpoints = aimbot->AddCheckBox("Show aim points");
	config.ragebot.aimbot.roll_resolver = aimbot->AddCheckBox("Roll Resolver");
	config.ragebot.aimbot.roll_resolver_key = aimbot->AddKeyBind("Roll Resolver");
	config.ragebot.aimbot.roll_angle = aimbot->AddSliderInt("Roll Angle", -90, 90, 0);

	config.ragebot.aimbot.extrapolation->SetVisible(false);
	//config.ragebot.aimbot.show_aimpoints->SetVisible(false);

	config.ragebot.selected_weapon = aim_settings->AddComboBox("Current Weapon", { "Global", "AWP", "Autosniper", "Scout", "Deagle", "Revolver", "Pistol" });

	auto setup_weapon_config = [this, aim_settings](weapon_settings_t& settings) {
		settings.hitboxes = aim_settings->AddMultiCombo(std::format("[{}] {}", settings.weapon_name, "Hitbox"), { "Head", "Chest", "Stomach", "Arms", "Legs", "Feet" });
		settings.multipoints = aim_settings->AddMultiCombo(std::format("[{}] {}", settings.weapon_name, "Multipoints"), { "Head", "Chest", "Stomach" });
		settings.head_point_scale = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Head scale"), 0, 100, 50, "%d%%");
		settings.body_point_scale = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Body scale"), 0, 100, 50, "%d%%");
		settings.hitchance = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Hitchance"), 0, 100, 50, "%d%%");
		settings.minimum_damage = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Minimum damage"), 1, 130, 30);
		settings.minimum_damage_override = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Minimum damage override"), 1, 130, 10);
		settings.auto_stop = aim_settings->AddMultiCombo(std::format("[{}] {}", settings.weapon_name, "Auto stop"), { "Early", "Move between shots", "In Air" });
		settings.auto_scope = aim_settings->AddCheckBox(std::format("[{}] {}", settings.weapon_name, "Auto scope"));
		settings.delay_shot = aim_settings->AddSliderInt(std::format("[{}] {}", settings.weapon_name, "Delay shot"), 0, 32, 0);
		settings.strict_hitchance = aim_settings->AddCheckBox(std::format("[{}] {}", settings.weapon_name, "Strict hitchance"));
		settings.aim_head_if_safe = aim_settings->AddCheckBox(std::format("[{}] {}", settings.weapon_name, "Aim head if safe"));
	};

	setup_weapon_config(config.ragebot.weapons.global);
	setup_weapon_config(config.ragebot.weapons.awp);
	setup_weapon_config(config.ragebot.weapons.autosniper);
	setup_weapon_config(config.ragebot.weapons.scout);
	setup_weapon_config(config.ragebot.weapons.deagle);
	setup_weapon_config(config.ragebot.weapons.revolver);
	setup_weapon_config(config.ragebot.weapons.pistol);

	config.antiaim.angles.pitch = aa_angles->AddComboBox("Pitch", { "Disabled", "Down" , "Experimental"});
	config.antiaim.angles.yaw = aa_angles->AddComboBox("Yaw", { "Forward", "At target", "Spin" });
	config.antiaim.angles.yaw_jitter = aa_angles->AddCheckBox("Yaw jitter");
	config.antiaim.angles.modifier_value = aa_angles->AddSliderInt("Modifier value", -180, 180, 0);
	config.antiaim.angles.defensive = aa_angles->AddCheckBox("Defensive");
	config.antiaim.angles.manual_left = aa_angles->AddKeyBind("Manual left");
	config.antiaim.angles.manual_right = aa_angles->AddKeyBind("Manual right");
	config.antiaim.angles.manual_options = aa_angles->AddMultiCombo("Manual options", {"Disable jitters", "Freestand"});
	config.antiaim.angles.body_yaw = aa_angles->AddCheckBox("Body yaw");
	config.antiaim.angles.body_yaw_options = aa_angles->AddMultiCombo("Body yaw options", { "Jitter", "Random jitter", "Extended", "Freestand" });
	config.antiaim.angles.body_yaw_limit = aa_angles->AddSliderInt("Limit", 0, 60, 60);
	config.antiaim.angles.inverter = aa_angles->AddKeyBind("Inverter");
	config.antiaim.angles.legacy_desync = aa_angles->AddCheckBox("Legacy Desync");

	config.antiaim.fakelag.enabled = fake_lag->AddCheckBox("Enabled");
	config.antiaim.fakelag.limit = fake_lag->AddSliderInt("Limit", 1, 15, 13);
	config.antiaim.fakelag.variability = fake_lag->AddSliderInt("Variabaility", 0, 14, 1);

	config.antiaim.misc.fake_duck = aa_other->AddKeyBind("Fake duck");
	config.antiaim.misc.slow_walk = aa_other->AddKeyBind("Slow walk");
	config.antiaim.misc.leg_movement = aa_other->AddComboBox("Leg movement", { "Default", "Sliding","Inverse slide" });
	config.antiaim.misc.jitter_move = aa_other->AddCheckBox("Jitter move");

	config.visuals.esp.enable = player_esp->AddCheckBox("Enable");
	config.visuals.esp.dormant = player_esp->AddCheckBox("Dormant");
	config.visuals.esp.dormant_color = player_esp->AddColorPicker("Dormant", Color(255, 255, 255, 200));
	config.visuals.esp.bounding_box = player_esp->AddCheckBox("Bounding box");
	config.visuals.esp.box_color = player_esp->AddColorPicker("Bounding box");
	config.visuals.esp.health_bar = player_esp->AddCheckBox("Health bar");
	config.visuals.esp.custom_health = player_esp->AddCheckBox("Custom health");
	config.visuals.esp.custom_health_color = player_esp->AddColorPicker("Custom health");
	config.visuals.esp.name = player_esp->AddCheckBox("Name");
	config.visuals.esp.name_color = player_esp->AddColorPicker("Name");
	config.visuals.esp.flags = player_esp->AddMultiCombo("Flags", { "Armor", "Zoom", "Fake duck", "Exploit", "Break LC", "Bomb", "Resolver", "Hit", "Onshot"});
	config.visuals.esp.weapon_text = player_esp->AddCheckBox("Weapon text");
	config.visuals.esp.weapon_text_color = player_esp->AddColorPicker("Weapon text");
	config.visuals.esp.weapon_icon = player_esp->AddCheckBox("Weapon icon");
	config.visuals.esp.weapon_icon_color = player_esp->AddColorPicker("Weapon icon");
	config.visuals.esp.ammo = player_esp->AddCheckBox("Ammo");
	config.visuals.esp.ammo_color = player_esp->AddColorPicker("Ammo", Color(80, 140, 200));
	config.visuals.esp.glow = player_esp->AddCheckBox("Glow");
	config.visuals.esp.glow_color = player_esp->AddColorPicker("Glow", Color(180, 60, 120));
	config.visuals.esp.hitsound = player_esp->AddCheckBox("Hit sound");
	config.visuals.esp.show_server_hitboxes = player_esp->AddCheckBox("Show sever hitboxes");
	config.visuals.esp.shared_esp = player_esp->AddCheckBox("Shared ESP");
	config.visuals.esp.share_with_enemies = player_esp->AddCheckBox("Share with enemies");
	config.visuals.esp.anti_fatality = player_esp->AddCheckBox("Anti fatality");
	config.visuals.esp.hitmarker = player_esp->AddCheckBox("Hitmarker");
	config.visuals.esp.hitmarker_color = player_esp->AddColorPicker("Hitmarker");
	config.visuals.esp.damage_marker = player_esp->AddCheckBox("Damage marker");
	config.visuals.esp.damage_marker_color = player_esp->AddColorPicker("Damage marker");

	config.visuals.esp.anti_fatality->SetVisible(false);
	config.visuals.esp.show_server_hitboxes->SetVisible(false);
	config.visuals.esp.bounding_box->SetVisible(false);
	config.visuals.esp.box_color->SetVisible(false);
	config.visuals.esp.ammo->SetVisible(false);
	config.visuals.esp.ammo_color->SetVisible(false);

	config.visuals.chams.chams_selector = chams->AddComboBox("Class", { "Player", "Shot", "Backtrack", "Local", "Attachments", "Viewmodel" });
	config.visuals.chams.enemy = chams->AddCheckBox("Player");
	config.visuals.chams.enemy_invisible = chams->AddCheckBox("Player behind wall");
	config.visuals.chams.enemy_color = chams->AddColorPicker("Player", Color(150, 190, 70));
	config.visuals.chams.enemy_invisible_color = chams->AddColorPicker("Player behind wall", Color(60, 120, 160));
	config.visuals.chams.enemy_second_color = chams->AddColorPicker("Enemy second color");
	config.visuals.chams.enemy_type = chams->AddComboBox("Enemy type", { "Default", "Solid", "Glow", "Glow outlined", "Glow overlay" });
	config.visuals.chams.enemy_glow_thickness = chams->AddSliderFloat("Enemy glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.shot_chams = chams->AddCheckBox("Shot");
	config.visuals.chams.shot_chams_color = chams->AddColorPicker("Shot", Color(100, 100, 100));
	config.visuals.chams.shot_chams_second_color = chams->AddColorPicker("Shot second color");
	config.visuals.chams.shot_chams_type = chams->AddComboBox("Shot type", { "Default", "Solid", "Glow", "Glow outlined" });
	config.visuals.chams.shot_chams_thickness = chams->AddSliderFloat("Shot glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.shot_chams_duration = chams->AddSliderInt("Shot chams duration", 1, 10, 4, "s");
	config.visuals.chams.shot_chams_options = chams->AddMultiCombo("Shot chams options", { "Behind walls", "Last shot only" });
	config.visuals.chams.backtrack_chams = chams->AddCheckBox("Backtrack");
	config.visuals.chams.backtrack_chams_color = chams->AddColorPicker("Backtrack", Color(100, 100, 100));
	config.visuals.chams.backtrack_chams_second_color = chams->AddColorPicker("Backtrack second color");
	config.visuals.chams.backtrack_chams_type = chams->AddComboBox("Backtrack type", { "Default", "Solid", "Glow", "Glow outlined" });
	config.visuals.chams.backtrack_chams_thickness = chams->AddSliderFloat("Backtrack glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.backtrack_chams_interpolate = chams->AddCheckBox("Interpolate");
	config.visuals.chams.local_player = chams->AddCheckBox("Local player");
	config.visuals.chams.local_player_color = chams->AddColorPicker("Local player", Color(100, 100, 100));
	config.visuals.chams.local_player_second_color = chams->AddColorPicker("Local second color");
	config.visuals.chams.local_player_type = chams->AddComboBox("Local type", { "Default", "Solid", "Glow", "Glow outlined", "Glow overlay" });
	config.visuals.chams.local_glow_thickness = chams->AddSliderFloat("Local glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.attachments = chams->AddCheckBox("Attachments");
	config.visuals.chams.attachments_color = chams->AddColorPicker("Attachments");
	config.visuals.chams.attachments_second_color = chams->AddColorPicker("Attachments second color");
	config.visuals.chams.attachments_type = chams->AddComboBox("Attachments type", { "Default", "Solid", "Glow", "Glow outlined", "Glow overlay" });
	config.visuals.chams.attachments_glow_thickness = chams->AddSliderFloat("Attachments glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.viewmodel = chams->AddCheckBox("Viewmodel");
	config.visuals.chams.viewmodel_color = chams->AddColorPicker("Viewmodel");
	config.visuals.chams.viewmodel_second_color = chams->AddColorPicker("Viewmodel second color");
	config.visuals.chams.viewmodel_type = chams->AddComboBox("Viewmodel type", { "Default", "Solid", "Glow", "Glow outlined", "Glow overlay" });
	config.visuals.chams.viewmodel_glow_thickness = chams->AddSliderFloat("Viewmodel glow thickness", 0.1f, 9.f, 1.f, "%.1f");
	config.visuals.chams.disable_model_occlusion = chams->AddCheckBox("Disable model occlusion");

	config.visuals.other_esp.radar = other_esp->AddCheckBox("Radar");
	config.visuals.other_esp.dropped_weapons = other_esp->AddMultiCombo("Dropped weapons", { "Text", "Icon", "Glow" });
	config.visuals.other_esp.dropped_weapons_color = other_esp->AddColorPicker("Dropped weapons");
	config.visuals.other_esp.sniper_crosshair = other_esp->AddCheckBox("Sniper crosshair");
	config.visuals.other_esp.penetration_crosshair = other_esp->AddCheckBox("Penetration crosshair");
	config.visuals.other_esp.bomb = other_esp->AddCheckBox("Bomb");
	config.visuals.other_esp.bomb_color = other_esp->AddColorPicker("Bomb", Color(150, 200, 60));
	config.visuals.other_esp.grenades = other_esp->AddCheckBox("Grenades");
	config.visuals.other_esp.molotov_radius = other_esp->AddCheckBox("Molotov radius");
	config.visuals.other_esp.molotov_radius_color = other_esp->AddColorPicker("Molotov radius", Color(255, 0, 0));
	config.visuals.other_esp.grenade_trajecotry = other_esp->AddCheckBox("Grenade trajectory");
	config.visuals.other_esp.grenade_trajectory_color = other_esp->AddColorPicker("Grenade trajectory", Color(250, 60, 60));
	config.visuals.other_esp.grenade_trajectory_hit_color = other_esp->AddColorPicker("Grenade trajectory (hit)", Color(150, 200, 60));
	config.visuals.other_esp.grenade_proximity_warning = other_esp->AddCheckBox("Grenade proximity warning");
	config.visuals.other_esp.grenade_predict_color = other_esp->AddColorPicker("Grenade predict color");
	config.visuals.other_esp.particles = other_esp->AddMultiCombo("Particles", { "Molotov" });

	config.visuals.other_esp.molotov_radius->SetVisible(false);
	config.visuals.other_esp.molotov_radius_color->SetVisible(false);
	config.visuals.other_esp.dropped_weapons->SetVisible(false);
	config.visuals.other_esp.dropped_weapons_color->SetVisible(false);

	config.visuals.effects.fov = effects->AddSliderInt("Field of view", 80, 130, 90);
	config.visuals.effects.fov_zoom = effects->AddSliderInt("FOV - Zoom", 0, 100, 10, "%d%%");
	config.visuals.effects.fov_second_zoom = effects->AddSliderInt("FOV - Second zoom", 0, 100, 0, "%d%%");
	config.visuals.effects.removals = effects->AddMultiCombo("Removals", { "Post effects", "Fog", "Shadows", "Smoke", "Flashbang", "Blood", "Sprites", "Recoil", "Bloom", "Particles", "Map details", "Dynamic lights" });
	config.visuals.effects.remove_scope = effects->AddComboBox("Remove scope", { "Disabled", "Remove overlay", "Remove all" });
	config.visuals.effects.world_color_enable = effects->AddCheckBox("World color");
	config.visuals.effects.world_color = effects->AddColorPicker("World color");
	config.visuals.effects.props_color_enable = effects->AddCheckBox("Props color");
	config.visuals.effects.props_color = effects->AddColorPicker("Props color");
	config.visuals.effects.thirdperson = effects->AddCheckBox("Force thirdperson");
	config.visuals.effects.thirdperson_dead = effects->AddCheckBox("Force thirdperson while dead");
	config.visuals.effects.thirdperson_bind = effects->AddKeyBind("Force thirdperson");
	config.visuals.effects.thirdperson_distance = effects->AddSliderInt("Thirdperson distance", 25, 200, 100);
	config.visuals.effects.aspect_ratio = effects->AddSliderFloat("Aspect ratio", 0, 2, 0, "%.2f");
	config.visuals.effects.client_impacts = effects->AddCheckBox("Client impacts");
	config.visuals.effects.client_impacts_color = effects->AddColorPicker("Client impacts", Color(255, 0, 0, 125));
	config.visuals.effects.server_impacts = effects->AddCheckBox("Server impacts");
	config.visuals.effects.server_impacts_color = effects->AddColorPicker("Server impacts", Color(0, 0, 255, 125));
	config.visuals.effects.impacts_duration = effects->AddSliderInt("Duration", 1, 10, 4, "%ds");
	config.visuals.effects.override_skybox = effects->AddComboBox("Override skybox", { "Disabled", "Tibet", "Baggage", "Italy", "Aztec", "Vertigo", "Daylight", "Daylight 2", "Clouds", "Clouds 2", "Gray", "Clear", "Canals", "Cobblestone", "Assault", "Clouds dark", "Night", "Night 2", "Night flat", "Dusty", "Rainy" });
	config.visuals.effects.override_fog = effects->AddCheckBox("Override fog");
	config.visuals.effects.fog_color = effects->AddColorPicker("Override fog");
	config.visuals.effects.fog_start = effects->AddSliderInt("Fog start", 0, 1000, 200);
	config.visuals.effects.fog_end = effects->AddSliderInt("Fog end", 0, 1000, 500);
	config.visuals.effects.fog_density = effects->AddSliderInt("Fog density", 0, 100, 50);
	config.visuals.effects.preserve_killfeed = effects->AddCheckBox("Preserve killfeed");
	config.visuals.effects.custom_sun_direction = effects->AddCheckBox("Custom sun direction");
	config.visuals.effects.sun_pitch = effects->AddSliderInt("Sun pitch", 0, 90, 0);
	config.visuals.effects.sun_yaw = effects->AddSliderInt("Sun yaw", -180, 180, 0);
	config.visuals.effects.sun_distance = effects->AddSliderInt("Sun distance", 0, 2000, 400);
	config.visuals.effects.scope_blend = effects->AddSliderInt("Scope blend", 0, 100, 30, "%d%%");
	config.visuals.effects.viewmodel_scope_alpha = effects->AddSliderInt("Viewmodel scope alpha", 0, 100, 0, "%d%%");

	config.misc.miscellaneous.anti_untrusted = misc->AddCheckBox("Anti untrusted");
	config.misc.miscellaneous.ping_reducer = misc->AddCheckBox("Ping reducer");
	config.misc.miscellaneous.automatic_grenade_release = misc->AddSliderInt("Automatic grenade release", 0, 60, 0, "%ddmg");
	config.misc.miscellaneous.logs = misc->AddMultiCombo("Logs", { "Damage", "Aimbot", "Purchuases" });
	config.misc.miscellaneous.auto_buy = misc->AddMultiCombo("Auto buy", { "AWP", "Scout", "Autosniper", "Deagle / R8", "Five-Seven / Tec-9", "Taser", "Armor", "Smoke", "Molotov", "HeGrenade", "Flashbang", "Defuse kit" });
	config.misc.miscellaneous.filter_console = misc->AddCheckBox("Filter console");
	config.misc.miscellaneous.clantag = misc->AddCheckBox("Clantag");
	config.misc.miscellaneous.ad_block = misc->AddCheckBox("Ad block");
	config.misc.miscellaneous.gamesense_mode = misc->AddCheckBox("Gamesense mode");
	config.misc.miscellaneous.ping_spike = misc->AddKeyBind("Ping spike");
	config.misc.miscellaneous.ping_spike_amount = misc->AddSliderInt("Amount", 0, 200, 200, "%dms");

	config.misc.movement.auto_jump = movement->AddCheckBox("Auto jump");
	config.misc.movement.auto_strafe = movement->AddCheckBox("Auto strafe");
	config.misc.movement.auto_strafe_smooth = movement->AddSliderInt("Auto strafe smooth", 0, 100, 50, "%d%%");
	config.misc.movement.easy_strafe = movement->AddCheckBox("Easy strafe");
	config.misc.movement.super_toss = movement->AddComboBox("Super toss", {"Disabled", "Semi", "Full"});
	config.misc.movement.edge_jump = movement->AddCheckBox("Edge jump");
	config.misc.movement.edge_jump_key = movement->AddKeyBind("Edge jump");
	config.misc.movement.infinity_duck = movement->AddCheckBox("Infinity duck");
	config.misc.movement.quick_stop = movement->AddCheckBox("Quick stop");

	config.misc.ui_group.indicators = ui_group->AddMultiCombo("Indicators", { "Watermark", "Performance" });

	config.skins.override_knife = models->AddCheckBox("Override knife");
	config.skins.knife_model = models->AddComboBox("Knife model", SkinChanger->GetUIKnifeModels());

	config.skins.mask_changer = models->AddCheckBox("Enable mask changer");
	config.skins.mask_changer_models = models->AddComboBox("Mask models", {
		"None",
		"Dallas",
		"Battle Mask",
		"Evil Clown",
		"Anaglyph",
		"Boar",
		"Bunny",
		"Bunny Gold",
		"Chains",
		"Chicken",
		"Devil Plastic",
		"Hoxton",
		"Pumpkin",
		"Samurai",
		"Sheep Bloody",
		"Sheep Gold",
		"Sheep Model",
		"Skull",
		"Template",
		"Wolf",
		"Doll",
	});

	config.skins.override_agent = models->AddCheckBox("Override agent");
	config.skins.agent_model_t = models->AddComboBox("Agent model T Side", {
	    "T Model",
		"CT Model",
		"Silent | Sir Bloody Darryl",
		"Vypa Sista of the Revolution | Guerrilla Warfare",
		"Medium Rare' Crasswater | Guerrilla Warfare",
		"Crasswater The Forgotten | Guerrilla Warfare",
		"Skullhead | Sir Bloody Darryl",
		"Chef d'Escadron Rouchard | Gendarmerie Nationale",
		"Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman",
		"Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman",
		"Royale | Sir Bloody Darryl",
		"Loudmouth | Sir Bloody Darryl",
		"Miami | Sir Bloody Darryl",
		"Getaway Sally | Professional",
		"Elite Trapper Solman | Guerrilla Warfare",
		"Bloody Darryl The Strapped | The Professionals",
		"Chem-Haz Capitaine | Gendarmerie Nationale",
		"Lieutenant Rex Krikey | SEAL Frogman",
		"Arno The Overgrown | Guerrilla Warfare",
		"Col. Mangos Dabisi | Guerrilla Warfare",
		"Officer Jacques Beltram | Gendarmerie Nationale",
		"Trapper | Guerrilla Warfare",
		"Lieutenant 'Tree Hugger' Farlow | SWAT",
		"Sous-Lieutenant Medic | Gendarmerie Nationale",
		"Primeiro Tenente | Brazilian 1st Battalion",
		"D Squadron Officer | NZSAS",
		"Trapper Aggressor | Guerrilla Warfare",
		"Aspirant | Gendarmerie Nationale",
		"AGENT Gandon | Professional",
		"Safecracker Voltzmann | Professional",
		"Little Kev | Professional",
		"Blackwolf | Sabre",
		"Rezan the Redshirt | Sabre",
		"Rezan The Ready | Sabre",
		"Maximus | Sabre",
		"Dragomir | Sabre",
		"Dragomir | Sabre Footsoldier",
		"Lt. Commander Ricksaw | NSWC SEAL",
		"Two Times' McCoy | USAF TACP",
		"Two Times' McCoy | USAF Cavalry",
		"Buckshot | NSWC SEAL",
		"Blueberries Buckshot | NSWC SEAL",
		"Seal Team 6 Soldier | NSWC SEAL",
		"3rd Commando Company | KSK",
		"The Doctor' Romanov | Sabre",
		"Michael Syfers| FBI Sniper",
		"Markus Delrow | FBI HRT",
		"Cmdr. Mae | SWAT",
		"1st Lieutenant Farlow | SWAT",
		"John Van Healen Kask | SWAT",
		"Bio-Haz Specialist | SWAT",
		"Chem-Haz Specialist | SWAT",
		"Sergeant Bombson | SWAT",
		"Operator | FBI SWAT",
		"Street Soldier | Phoenix",
		"Slingshot | Phoenix",
		"Enforcer | Phoenix",
		"Soldier | Phoenix",
		"The Elite Mr. Muhlik | Elite Crew",
		"Prof. Shahmat | Elite Crew",
		"Osiris | Elite Crew",
		"Ground Rebel| Elite Crew",
		"Special Agent Ava | FBI",
		"B Squadron Officer | SAS",
		"Jumpsuit A",
		"Jumpsuit B",
		"Jumpsuit C",
		"Anarchist A",
		"Anarchist B",
		"Anarchist C",
		"Separatist A",
		"Separatist B",
		"Separatist C",
		"Separatist D",
		"CTM. FBI",
		"CTM. FBI A",
		"CTM. FBI B",
		"CTM. FBI C",
		"CTM. FBI E",
		"Gign Model A",
		"Gign Model B",
		"Gign Model C",
		"CTM. ST6",
		"CTM. ST6 A",
		"CTM. ST6 B",
		"CTM. ST6 C",
		"CTM. ST6 D",
		"CTM. IDF B",
		"CTM. IDF C",
		"CTM. IDF D",
		"CTM. IDF E",
		"CTM. IDF F",
		"CTM. Swat",
		"TM. Swat A",
		"CTM. Swat B",
		"CTM. Swat C",
		"CTM. Swat D",
		"CTM. Sas",
		"CTM. Gsg9",
		"CTM. Gsg9 A",
		"CTM. Gsg9 B",
		"CTM. Gsg9 C",
		"CTM. Gsg9 D",
		"Professional A",
		"Professional B",
		"Professional C",
		"Professional D",
		"Leet A",
		"Leet B",
		"Leet C",
		"Leet D",
		"Balkan A",
		"Balkan B",
		"Balkan C",
		"Balkan D",
		"Pirate A",
		"Pirate B",
		"Pirate C",
	});

	config.skins.agent_model_ct = models->AddComboBox("Agent model CT Side", {
	 "T Model",
		"CT Model",
		"Silent | Sir Bloody Darryl",
		"Vypa Sista of the Revolution | Guerrilla Warfare",
		"Medium Rare' Crasswater | Guerrilla Warfare",
		"Crasswater The Forgotten | Guerrilla Warfare",
		"Skullhead | Sir Bloody Darryl",
		"Chef d'Escadron Rouchard | Gendarmerie Nationale",
		"Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman",
		"Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman",
		"Royale | Sir Bloody Darryl",
		"Loudmouth | Sir Bloody Darryl",
		"Miami | Sir Bloody Darryl",
		"Getaway Sally | Professional",
		"Elite Trapper Solman | Guerrilla Warfare",
		"Bloody Darryl The Strapped | The Professionals",
		"Chem-Haz Capitaine | Gendarmerie Nationale",
		"Lieutenant Rex Krikey | SEAL Frogman",
		"Arno The Overgrown | Guerrilla Warfare",
		"Col. Mangos Dabisi | Guerrilla Warfare",
		"Officer Jacques Beltram | Gendarmerie Nationale",
		"Trapper | Guerrilla Warfare",
		"Lieutenant 'Tree Hugger' Farlow | SWAT",
		"Sous-Lieutenant Medic | Gendarmerie Nationale",
		"Primeiro Tenente | Brazilian 1st Battalion",
		"D Squadron Officer | NZSAS",
		"Trapper Aggressor | Guerrilla Warfare",
		"Aspirant | Gendarmerie Nationale",
		"AGENT Gandon | Professional",
		"Safecracker Voltzmann | Professional",
		"Little Kev | Professional",
		"Blackwolf | Sabre",
		"Rezan the Redshirt | Sabre",
		"Rezan The Ready | Sabre",
		"Maximus | Sabre",
		"Dragomir | Sabre",
		"Dragomir | Sabre Footsoldier",
		"Lt. Commander Ricksaw | NSWC SEAL",
		"Two Times' McCoy | USAF TACP",
		"Two Times' McCoy | USAF Cavalry",
		"Buckshot | NSWC SEAL",
		"Blueberries Buckshot | NSWC SEAL",
		"Seal Team 6 Soldier | NSWC SEAL",
		"3rd Commando Company | KSK",
		"The Doctor' Romanov | Sabre",
		"Michael Syfers| FBI Sniper",
		"Markus Delrow | FBI HRT",
		"Cmdr. Mae | SWAT",
		"1st Lieutenant Farlow | SWAT",
		"John Van Healen Kask | SWAT",
		"Bio-Haz Specialist | SWAT",
		"Chem-Haz Specialist | SWAT",
		"Sergeant Bombson | SWAT",
		"Operator | FBI SWAT",
		"Street Soldier | Phoenix",
		"Slingshot | Phoenix",
		"Enforcer | Phoenix",
		"Soldier | Phoenix",
		"The Elite Mr. Muhlik | Elite Crew",
		"Prof. Shahmat | Elite Crew",
		"Osiris | Elite Crew",
		"Ground Rebel| Elite Crew",
		"Special Agent Ava | FBI",
		"B Squadron Officer | SAS",
		"Jumpsuit A",
		"Jumpsuit B",
		"Jumpsuit C",
		"Anarchist A",
		"Anarchist B",
		"Anarchist C",
		"Separatist A",
		"Separatist B",
		"Separatist C",
		"Separatist D",
		"CTM. FBI",
		"CTM. FBI A",
		"CTM. FBI B",
		"CTM. FBI C",
		"CTM. FBI E",
		"Gign Model A",
		"Gign Model B",
		"Gign Model C",
		"CTM. ST6",
		"CTM. ST6 A",
		"CTM. ST6 B",
		"CTM. ST6 C",
		"CTM. ST6 D",
		"CTM. IDF B",
		"CTM. IDF C",
		"CTM. IDF D",
		"CTM. IDF E",
		"CTM. IDF F",
		"CTM. Swat",
		"TM. Swat A",
		"CTM. Swat B",
		"CTM. Swat C",
		"CTM. Swat D",
		"CTM. Sas",
		"CTM. Gsg9",
		"CTM. Gsg9 A",
		"CTM. Gsg9 B",
		"CTM. Gsg9 C",
		"CTM. Gsg9 D",
		"Professional A",
		"Professional B",
		"Professional C",
		"Professional D",
		"Leet A",
		"Leet B",
		"Leet C",
		"Leet D",
		"Balkan A",
		"Balkan B",
		"Balkan C",
		"Balkan D",
		"Pirate A",
		"Pirate B",
		"Pirate C",
	});

	Config->config_list = configs->AddComboBox("cfglist", {});
	Config->config_name = configs->AddInput("Config");
	Config->load_button = configs->AddButton("Load");
	Config->save_button = configs->AddButton("Save");
	Config->refresh_button = configs->AddButton("Refresh");

	Config->lua_list = scripts->AddComboBox("lualist", {});
	Config->lua_button = scripts->AddButton("Load");
	Config->lua_button_unload = scripts->AddButton("Unload");
	Config->lua_refresh = scripts->AddButton("Refresh");
	Config->lua_save = scripts->AddButton("Save");

	Config->Init();
	
	config.ragebot.selected_weapon->SetCallback([]() {
		const int selected_weapon = config.ragebot.selected_weapon->get();
		auto& weapon_configs = config.ragebot.weapons;

		weapon_configs.global.SetVisible(selected_weapon == 0);
		weapon_configs.awp.SetVisible(selected_weapon == 1);
		weapon_configs.autosniper.SetVisible(selected_weapon == 2);
		weapon_configs.scout.SetVisible(selected_weapon == 3);
		weapon_configs.deagle.SetVisible(selected_weapon == 4);
		weapon_configs.revolver.SetVisible(selected_weapon == 5);
		weapon_configs.pistol.SetVisible(selected_weapon == 6);
	});

	auto world_modulation_callback = []() {
		ctx.update_nightmode = true;
	};

	config.visuals.effects.world_color_enable->SetCallback(world_modulation_callback);
	config.visuals.effects.world_color->SetCallback(world_modulation_callback);
	config.visuals.effects.props_color_enable->SetCallback(world_modulation_callback);
	config.visuals.effects.props_color->SetCallback(world_modulation_callback);

	config.visuals.effects.override_skybox->SetCallback([]() {
		World->SkyBox();
	});

	auto world_fog_callback = []() {
		World->Fog();

		config.visuals.effects.fog_density->SetVisible(config.visuals.effects.override_fog->get());
		config.visuals.effects.fog_start->SetVisible(config.visuals.effects.override_fog->get());
		config.visuals.effects.fog_end->SetVisible(config.visuals.effects.override_fog->get());
	};

	config.visuals.effects.override_fog->SetCallback(world_fog_callback);
	config.visuals.effects.fog_color->SetCallback(world_fog_callback);
	config.visuals.effects.fog_density->SetCallback(world_fog_callback);
	config.visuals.effects.fog_start->SetCallback(world_fog_callback);
	config.visuals.effects.fog_end->SetCallback(world_fog_callback);

	config.visuals.effects.removals->SetCallback([]() {
		World->Smoke();
		ctx.update_remove_blood = true;
	});

	config.visuals.effects.custom_sun_direction->SetCallback([]() {
		config.visuals.effects.sun_pitch->SetVisible(config.visuals.effects.custom_sun_direction->get());
		config.visuals.effects.sun_yaw->SetVisible(config.visuals.effects.custom_sun_direction->get());
		config.visuals.effects.sun_distance->SetVisible(config.visuals.effects.custom_sun_direction->get());
	});

	config.visuals.esp.shared_esp->SetCallback([]() {
		config.visuals.esp.share_with_enemies->SetVisible(config.visuals.esp.shared_esp->get());
	});

	config.visuals.chams.chams_selector->SetCallback([]() {
		config.visuals.chams.enemy->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_color->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_glow_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_invisible->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_invisible_color->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 0);
		config.visuals.chams.enemy_type->SetVisible(config.visuals.chams.chams_selector->get() == 0);

		config.visuals.chams.shot_chams->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_color->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_type->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_duration->SetVisible(config.visuals.chams.chams_selector->get() == 1);
		config.visuals.chams.shot_chams_options->SetVisible(config.visuals.chams.chams_selector->get() == 1);

		config.visuals.chams.backtrack_chams->SetVisible(config.visuals.chams.chams_selector->get() == 2);
		config.visuals.chams.backtrack_chams_color->SetVisible(config.visuals.chams.chams_selector->get() == 2);
		config.visuals.chams.backtrack_chams_type->SetVisible(config.visuals.chams.chams_selector->get() == 2);
		config.visuals.chams.backtrack_chams_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 2);
		config.visuals.chams.backtrack_chams_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 2);
		config.visuals.chams.backtrack_chams_interpolate->SetVisible(config.visuals.chams.chams_selector->get() == 2);

		config.visuals.chams.local_player->SetVisible(config.visuals.chams.chams_selector->get() == 3);
		config.visuals.chams.local_player_color->SetVisible(config.visuals.chams.chams_selector->get() == 3);
		config.visuals.chams.local_player_type->SetVisible(config.visuals.chams.chams_selector->get() == 3);
		config.visuals.chams.local_player_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 3);
		config.visuals.chams.local_glow_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 3);

		config.visuals.chams.attachments->SetVisible(config.visuals.chams.chams_selector->get() == 4);
		config.visuals.chams.attachments_color->SetVisible(config.visuals.chams.chams_selector->get() == 4);
		config.visuals.chams.attachments_type->SetVisible(config.visuals.chams.chams_selector->get() == 4);
		config.visuals.chams.attachments_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 4);
		config.visuals.chams.attachments_glow_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 4);

		config.visuals.chams.viewmodel->SetVisible(config.visuals.chams.chams_selector->get() == 5);
		config.visuals.chams.viewmodel_color->SetVisible(config.visuals.chams.chams_selector->get() == 5);
		config.visuals.chams.viewmodel_type->SetVisible(config.visuals.chams.chams_selector->get() == 5);
		config.visuals.chams.viewmodel_second_color->SetVisible(config.visuals.chams.chams_selector->get() == 5);
		config.visuals.chams.viewmodel_glow_thickness->SetVisible(config.visuals.chams.chams_selector->get() == 5);
	});

	config.visuals.effects.fov->SetCallback([]() {
		config.visuals.effects.fov_zoom->SetVisible(config.visuals.effects.fov->get() != 90);
		config.visuals.effects.fov_second_zoom->SetVisible(config.visuals.effects.fov->get() != 90);
	});
}

void CMenu::Watermark()
{
	if (!config.misc.ui_group.indicators->get(0))
		return;

	char cur_time[128]{};
	time_t t = time(NULL);
	struct tm* ptm = localtime(&t);
	strftime(cur_time, sizeof(cur_time), "%H:%M", ptm);

	auto calculated_ping = ctx.real_ping == -1.f ? 0 : (int)(ctx.real_ping * 1000.f);
	auto ping = std::format("{}ms", calculated_ping);

	ImVec2 text_size{};
	ImGui::PushFont(ctx.font);
	{
		std::string watermark_string = this->cheat_name + " " + std::string(cur_time) + " " + ping;

		text_size = ImGui::CalcTextSize(watermark_string.c_str());
	}
	ImGui::PopFont();

	auto window_size = ImVec2(50.f + text_size.x, 32.f);
	static auto opened = true;

	ImVec2 fixed_position = ImVec2(Cheat.ScreenSize.x - window_size.x - 10.f, 10.f);

	ImGui::SetNextWindowPos(fixed_position);
	ImGui::SetNextWindowSize(window_size);
	ImGui::PushFont(ctx.font);
	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::Begin("##watermark_window", &opened, ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoFocusOnAppearing);
	{
		auto list = ImGui::GetWindowDrawList();
		list->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;

		auto prefix_size = ImGui::CalcTextSize(this->cheat_name.c_str());

		auto base_window_pos_spec = ImGui::GetWindowPos() + ImVec2(4.f, 1.f);
		auto base_window_pos = ImGui::GetWindowPos() + ImVec2(14.f, 1.f);

		std::string right_text = std::string(cur_time) + " " + ping;

		auto text_test = ImGui::CalcTextSize(right_text.c_str());
		auto menu_color = ImColor(105, 163, 255, 255);

		{
			auto base_offset = ImVec2{ base_window_pos.x + 7.f, base_window_pos.y + 6.5f };

			//left
			list->AddRectFilled(base_window_pos_spec + ImVec2(5, 0), base_window_pos_spec + ImVec2(prefix_size.x + 14, window_size.y - 3.f), menu_color, 5.f);
			list->AddRectFilled(base_window_pos, base_window_pos + ImVec2(prefix_size.x + 14, window_size.y - 3.f), ImColor(25, 25, 30, 255), 6.f);
			list->AddText(base_offset, ImColor(255, 255, 255, 255), this->cheat_name.c_str());
			list->AddRect(base_window_pos, base_window_pos + ImVec2(prefix_size.x + 14, window_size.y - 3.f), ImColor(120, 120, 120, 100), 6.f);

			//right
			list->AddRectFilled(base_window_pos + ImVec2(prefix_size.x + 18, 0), base_window_pos + ImVec2(prefix_size.x + text_test.x + 31, window_size.y - 3.f), ImColor(25, 25, 30, 255), 6.f);
			list->AddText(base_offset + ImVec2(prefix_size.x + 17, 0), ImColor(255, 255, 255, 255), right_text.c_str());
			list->AddRect(base_window_pos + ImVec2(prefix_size.x + 18, 0), base_window_pos + ImVec2(prefix_size.x + text_test.x + 31, window_size.y - 3.f), ImColor(120, 120, 120, 100), 6.f);
		}

		list->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);
	}
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleColor();
}

void DrawGraph(ImDrawList* draw_list, const char* label, const std::vector<float>& data, ImVec2 pos, ImVec2 size, ImColor color, bool unk, float max_value)
{
	ImVec2 graph_pos = pos;
	ImVec2 graph_size = size;

	draw_list->AddRectFilled(graph_pos, graph_pos + graph_size, ImColor(20, 20, 20, 200), 2.0f);

	if (unk) {
		int points_count = data.size();
		float x_step = graph_size.x / (points_count - 1);

		for (int i = 0; i < points_count - 1; i++)
		{
			float x1 = graph_pos.x + i * x_step;
			float x2 = graph_pos.x + (i + 1) * x_step;
			float y1 = graph_pos.y + graph_size.y * (1 - data[i] / max_value);
			float y2 = graph_pos.y + graph_size.y * (1 - data[i + 1] / max_value);
			draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 1.0f);
		}

		draw_list->AddText(ImVec2(graph_pos.x + 5, graph_pos.y + 5), ImColor(200, 200, 200), label);

		char min_text[16], max_text[16];
		snprintf(min_text, sizeof(min_text), "%.1f", *std::min_element(data.begin(), data.end()));
		snprintf(max_text, sizeof(max_text), "%.1f", *std::max_element(data.begin(), data.end()));
		draw_list->AddText(ImVec2(graph_pos.x + graph_size.x - 30, graph_pos.y + 5), ImColor(200, 200, 200), max_text);
		draw_list->AddText(ImVec2(graph_pos.x + graph_size.x - 30, graph_pos.y + graph_size.y - 15), ImColor(200, 200, 200), min_text);
	}
	else {
		int points_count = data.size();
		float x_step = graph_size.x / (points_count - 1);

		for (int i = 0; i < points_count - 1; i++)
		{
			float x1 = graph_pos.x + i * x_step;
			float x2 = graph_pos.x + (i + 1) * x_step;
			float y1 = graph_pos.y + graph_size.y * (1 - data[i]);
			float y2 = graph_pos.y + graph_size.y * (1 - data[i + 1]);

			draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 1.0f);
		}

		draw_list->AddText(ImVec2(graph_pos.x + 5, graph_pos.y + 5), ImColor(200, 200, 200), label);

		char min_text[16], max_text[16];
		snprintf(min_text, sizeof(min_text), "%.1f", *std::min_element(data.begin(), data.end()) * 100);
		snprintf(max_text, sizeof(max_text), "%.1f", *std::max_element(data.begin(), data.end()) * 100);

		draw_list->AddText(ImVec2(graph_pos.x + graph_size.x - 30, graph_pos.y + 5), ImColor(200, 200, 200), max_text);
		draw_list->AddText(ImVec2(graph_pos.x + graph_size.x - 30, graph_pos.y + graph_size.y - 15), ImColor(200, 200, 200), min_text);
	}
}

ULONGLONG SubtractTimes(const FILETIME& t1, const FILETIME& t2) {
	LARGE_INTEGER a, b;
	a.LowPart = t1.dwLowDateTime;
	a.HighPart = t1.dwHighDateTime;
	b.LowPart = t2.dwLowDateTime;
	b.HighPart = t2.dwHighDateTime;
	return a.QuadPart - b.QuadPart;
}

float GetCPUUsage() {
	static FILETIME prevIdleTime, prevKernelTime, prevUserTime;
	static std::chrono::steady_clock::time_point lastUpdateTime;
	static float lastUsage = 0.0f;
	static bool firstCall = true;

	auto now = std::chrono::steady_clock::now();
	if (firstCall || std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count() >= 500) {
		FILETIME idleTime, kernelTime, userTime;
		GetSystemTimes(&idleTime, &kernelTime, &userTime);

		if (!firstCall) {
			ULONGLONG idle = SubtractTimes(idleTime, prevIdleTime);
			ULONGLONG kernel = SubtractTimes(kernelTime, prevKernelTime);
			ULONGLONG user = SubtractTimes(userTime, prevUserTime);

			ULONGLONG system = kernel + user;
			ULONGLONG total = system + idle;

			lastUsage = total > 0 ? (float)(system * 100.0 / total) : 0.0f;
		}

		prevIdleTime = idleTime;
		prevKernelTime = kernelTime;
		prevUserTime = userTime;

		lastUpdateTime = now;
		firstCall = false;
	}

	return lastUsage;
}

int GetRAMUsage() {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return static_cast<int>(statex.dwMemoryLoad);
}

void CMenu::arc_()
{
	if (!config.misc.ui_group.indicators->get(1))
		return;

	ImGui::PushFont(ctx.font);

	static float panel_width = 240.0f;
	static float panel_height = 145.0f;
	static ImVec2 panel_pos(ImGui::GetIO().DisplaySize.x - panel_width - 20.0f,
		ImGui::GetIO().DisplaySize.y - panel_height - 20.0f);

	ImGui::SetNextWindowPos(panel_pos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

	if (Menu->IsOpened())
		window_flags &= ~ImGuiWindowFlags_NoMove;
	else
		window_flags |= ImGuiWindowFlags_NoMove;

	if (ImGui::Begin("Performance Monitor", nullptr, window_flags))
	{
		panel_pos = ImGui::GetWindowPos();
		panel_width = ImGui::GetWindowWidth();
		panel_height = ImGui::GetWindowHeight();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// header
		draw_list->AddRectFilled(panel_pos, panel_pos + ImVec2(panel_width, 20), ImColor(10, 10, 10, 255));
		draw_list->AddText(panel_pos + ImVec2(5, 3), ImColor(255, 255, 255, 200), "Performance");

		// separator
		draw_list->AddRectFilled(panel_pos + ImVec2(0, 20), panel_pos + ImVec2(panel_width, 22), ImColor(0, 120, 215, 255));

		// info bar
		float cpu_usage = GetCPUUsage();
		float ram_usage = GetRAMUsage();
		int fps = static_cast<int>(ImGui::GetIO().Framerate);
		auto calculated_ping = ctx.real_ping == -1.f ? 0 : (int)(ctx.real_ping * 1000.f);

		char info_text[128];
		snprintf(info_text, sizeof(info_text), "FPS %d   CPU %.1f%%   RAM %.1f%%   PING %dms", fps, cpu_usage, ram_usage, calculated_ping);
		draw_list->AddText(panel_pos + ImVec2(5, 25), ImColor(255, 255, 255, 255), info_text);

		// graphs
		static std::vector<float> fps_history(47, 0.0f);
		static std::vector<float> cpu_history(47, 0.0f);

		fps_history.erase(fps_history.begin());
		fps_history.push_back(fps); // assuming max fps is 300

		cpu_history.erase(cpu_history.begin());
		cpu_history.push_back(cpu_usage / 100);

		float max_fps = max(300.0f, *std::max_element(fps_history.begin(), fps_history.end()));

		float max_cpu = max(100.0f, *std::max_element(cpu_history.begin(), cpu_history.end()));

		DrawGraph(draw_list, "FPS", fps_history, panel_pos + ImVec2(5, 45), ImVec2(panel_width - 10, 45), ImColor(0, 120, 215, 255), true, max_fps);
		DrawGraph(draw_list, "CPU", cpu_history, panel_pos + ImVec2(5, 95), ImVec2(panel_width - 10, 45), ImColor(215, 120, 0, 255), false, max_cpu);
	}
	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
	ImGui::PopFont();
}