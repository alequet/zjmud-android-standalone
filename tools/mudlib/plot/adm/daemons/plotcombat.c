// Shared adaptive combat profiles for original plot encounters.

inherit F_DBASE;

#define MAX_BOSS_PHASE 2

private mapping martial_backgrounds = ([
	"wanderer_escort_blade" : ([
		"name" : "江湖走镖刀客",
		"faction" : "江湖散人",
		"style" : "blade",
		"weapon" : "/clone/weapon/gangdao",
		"weapon_skill" : "wuhu-duanmendao",
		"force_skill" : "wuzheng-xinfa",
		"dodge_skill" : "feiyan-zoubi",
		"secondary_style" : "unarmed",
		"secondary_skill" : "houquan",
		"skill_map" : ([
			"blade" : "wuhu-duanmendao",
			"parry" : "wuhu-duanmendao",
			"force" : "wuzheng-xinfa",
			"dodge" : "feiyan-zoubi",
			"unarmed" : "houquan",
		]),
		"skill_prepare" : ([ "unarmed" : "houquan" ]),
		"phase_requirements" : ({
			([ "primary" : 120, "force" : 80, "max_neili" : 600,
				"neili" : 200, "core" : "blade.duan" ]),
		}),
		"phase_actions" : ({ ({ "blade.duan" }) }),
	]),
	"shaolin_lay_hand" : ([
		"name" : "少林俗家旁支",
		"faction" : "少林俗家",
		"style" : "hand",
		"weapon" : "",
		"weapon_skill" : "fengyun-shou",
		"force_skill" : "hunyuan-yiqi",
		"dodge_skill" : "shaolin-shenfa",
		"secondary_style" : "strike",
		"secondary_skill" : "sanhua-zhang",
		"skill_map" : ([
			"hand" : "fengyun-shou",
			"parry" : "fengyun-shou",
			"force" : "hunyuan-yiqi",
			"dodge" : "shaolin-shenfa",
			"strike" : "sanhua-zhang",
		]),
		"skill_prepare" : ([
			"hand" : "fengyun-shou",
			"strike" : "sanhua-zhang",
		]),
		"phase_requirements" : ({
			([ "primary" : 120, "force" : 100, "max_neili" : 700,
				"neili" : 150, "core" : "hand.qinna" ]),
		}),
		"phase_actions" : ({ ({ "hand.qinna", "force.powerup" }) }),
	]),
	"baichuan_water_escort" : ([
		"name" : "百川会水路护运",
		"faction" : "百川会",
		"style" : "blade",
		"weapon" : "/clone/weapon/gangdao",
		"weapon_skill" : "wuhu-duanmendao",
		"force_skill" : "lingyuan-xinfa",
		"dodge_skill" : "feiyan-zoubi",
		"secondary_style" : "hand",
		"secondary_skill" : "yunlong-shou",
		"skill_map" : ([
			"blade" : "wuhu-duanmendao",
			"parry" : "wuhu-duanmendao",
			"force" : "lingyuan-xinfa",
			"dodge" : "feiyan-zoubi",
			"hand" : "yunlong-shou",
		]),
		"skill_prepare" : ([ "hand" : "yunlong-shou" ]),
		"phase_requirements" : ({
			([ "primary" : 120, "force" : 100, "max_neili" : 700,
				"neili" : 200, "core" : "blade.duan" ]),
		}),
		"phase_actions" : ({ ({ "blade.duan" }) }),
	]),
	"yunlong_courier" : ([
		"name" : "云龙门外围信使",
		"faction" : "云龙门外围",
		"style" : "whip",
		"weapon" : "/clone/weapon/changbian",
		"weapon_skill" : "yunlong-bian",
		"force_skill" : "yunlong-shengong",
		"dodge_skill" : "yunlong-shenfa",
		"secondary_style" : "hand",
		"secondary_skill" : "yunlong-shou",
		"skill_map" : ([
			"whip" : "yunlong-bian",
			"parry" : "yunlong-bian",
			"force" : "yunlong-shengong",
			"dodge" : "yunlong-shenfa",
			"hand" : "yunlong-shou",
		]),
		"skill_prepare" : ([ "hand" : "yunlong-shou" ]),
		"phase_requirements" : ({
			([ "primary" : 70, "force" : 100, "max_neili" : 700,
				"neili" : 150, "core" : "whip.chan" ]),
		}),
		"phase_actions" : ({ ({ "whip.chan", "force.powerup" }) }),
	]),
	"gaibang_old_branch" : ([
		"name" : "丐帮旧支",
		"faction" : "丐帮旧支",
		"style" : "staff",
		"weapon" : "/clone/plot/where_rivers_end/qimei_staff",
		"weapon_skill" : "dagou-bang",
		"force_skill" : "huntian-qigong",
		"dodge_skill" : "feiyan-zoubi",
		"secondary_style" : "strike",
		"secondary_skill" : "dragon-strike",
		"skill_map" : ([
			"staff" : "dagou-bang",
			"parry" : "dagou-bang",
			"force" : "huntian-qigong",
			"dodge" : "feiyan-zoubi",
			"strike" : "dragon-strike",
		]),
		"skill_prepare" : ([ "strike" : "dragon-strike" ]),
		"phase_requirements" : ({
			([ "primary" : 70, "force" : 100, "max_neili" : 1200,
				"neili" : 200, "core" : "staff.chan" ]),
			([ "primary" : 100, "force" : 100, "max_neili" : 1400,
				"neili" : 300, "core" : "staff.ban" ]),
			([ "primary" : 150, "force" : 220, "max_neili" : 1800,
				"neili" : 500, "core" : "staff.wugou" ]),
		}),
		"phase_actions" : ({
			({ "staff.chan", "force.powerup" }),
			({ "staff.chan", "staff.ban", "force.powerup" }),
			({ "staff.ban", "staff.wugou", "force.powerup" }),
		}),
	]),
]);

private mapping profiles = ([
	"street_blade" : ([
		"martial_background" : "wanderer_escort_blade",
		"exp_ratio" : 70, "skill_ratio" : 75, "base_skill" : 20,
		"skill_cap" : 900, "qi_base" : 360, "max_phase" : 0,
	]),
	"green_hood" : ([
		"martial_background" : "shaolin_lay_hand",
		"exp_ratio" : 72, "skill_ratio" : 78, "base_skill" : 18,
		"skill_cap" : 900, "qi_base" : 340, "max_phase" : 0,
	]),
	"ferry_matron" : ([
		"martial_background" : "baichuan_water_escort",
		"exp_ratio" : 74, "skill_ratio" : 80, "base_skill" : 20,
		"skill_cap" : 1000, "qi_base" : 420, "max_phase" : 0,
	]),
	"courier_scout" : ([
		"martial_background" : "yunlong_courier",
		"exp_ratio" : 73, "skill_ratio" : 79, "base_skill" : 19,
		"skill_cap" : 950, "qi_base" : 380, "max_phase" : 0,
	]),
	"ledger_guard" : ([
		"martial_background" : "baichuan_water_escort",
		"exp_ratio" : 76, "skill_ratio" : 81, "base_skill" : 22,
		"skill_cap" : 1050, "qi_base" : 440, "max_phase" : 0,
	]),
	"ledger_master" : ([
		"martial_background" : "gaibang_old_branch",
		"exp_ratio" : 82, "skill_ratio" : 86, "base_skill" : 25,
		"skill_cap" : 1150, "qi_base" : 520, "max_phase" : 2,
	]),
]);

void create()
{
	seteuid(ROOT_UID);
}

private int larger(int first, int second)
{
	return first > second ? first : second;
}

private mapping copy_flat_mapping(mapping source)
{
	mapping result;
	mixed key;

	result = ([]);
	if (! mapp(source)) return result;
	foreach (key in keys(source)) result[key] = source[key];
	return result;
}

private int caller_is_plot_controller()
{
	object caller;
	string file;

	caller = previous_object();
	if (! objectp(caller)) return 0;
	file = base_name(caller);
	return stringp(file) && strsrch(file, "/adm/daemons/plot/") == 0;
}

private int effective_level(int exp)
{
	int level;

	if (exp < 1000) exp = 1000;
	level = to_int(pow(to_float(exp) * 10.0, 0.333333333));
	return level < 20 ? 20 : level;
}

private int combat_tier(int level)
{
	if (level < 60) return 0;
	if (level < 100) return 1;
	if (level < 150) return 2;
	if (level < 220) return 3;
	if (level < 320) return 4;
	return 5;
}

private int meridian_boost(object me)
{
	int boost;
	int gain;

	if (stringp(me->query("meridian/ap")) && me->query("meridian/ap") != "")
		boost++;
	if (stringp(me->query("meridian/dp")) && me->query("meridian/dp") != "")
		boost++;
	gain = me->query("gain/attack") + me->query("gain/defense");
	if (gain >= 100) boost++;
	if (gain >= 300) boost++;
	return boost > 2 ? 2 : boost;
}

mapping query_player_profile(object me, string profile_id, int phase)
{
	mapping result;
	mapping background;
	mapping requirements;
	int exp;
	int level;
	int tier;
	int boost;
	int skill;
	int force;
	int ratio;
	int max_neili;

	if (! objectp(me) || ! userp(me) || ! mapp(profiles[profile_id]) ||
		phase < 0 || phase > MAX_BOSS_PHASE ||
		phase > profiles[profile_id]["max_phase"])
		return 0;
	background = martial_backgrounds[profiles[profile_id]["martial_background"]];
	if (! mapp(background) || ! arrayp(background["phase_requirements"]) ||
		phase >= sizeof(background["phase_requirements"])) return 0;
	requirements = background["phase_requirements"][phase];
	exp = me->query("combat_exp");
	if (exp < 1000) exp = 1000;
	level = effective_level(exp);
	tier = combat_tier(level);
	boost = meridian_boost(me);
	result = copy_flat_mapping(profiles[profile_id]);
	ratio = result["exp_ratio"] + boost * 5 + phase * 15;
	if (ratio > 110) ratio = 110;
	skill = level * result["skill_ratio"] / 100 + result["base_skill"] + phase * 12;
	skill = larger(skill, requirements["primary"]);
	skill += tier * 2 + boost * 4;
	if (skill > result["skill_cap"]) skill = result["skill_cap"];
	force = larger(skill - 10, requirements["force"]);
	max_neili = larger(skill * 12, requirements["max_neili"]);
	max_neili = larger(max_neili, requirements["neili"]);
	result["profile_id"] = profile_id;
	result["combat_exp"] = exp * ratio / 100;
	if (result["combat_exp"] < 1200) result["combat_exp"] = 1200;
	result["exp_ratio_applied"] = ratio;
	result["effective_level"] = level;
	result["combat_tier"] = tier;
	result["meridian_boost"] = boost;
	result["skill_level"] = skill;
	result["force_level"] = force;
	result["max_neili"] = max_neili;
	result["required_neili"] = requirements["neili"];
	result["core_perform"] = requirements["core"];
	result["phase_actions"] = background["phase_actions"][phase];
	result["phase"] = phase;
	result["max_qi"] = result["qi_base"] + skill * 4 + boost * 80 + phase * 200;
	if (result["max_qi"] > 12000) result["max_qi"] = 12000;
	return result;
}

private void install_combat_callback(object enemy)
{
	enemy->enable_plot_combat_actions();
}

mapping configure_enemy(object enemy, object me, string profile_id, int phase)
{
	mapping profile;
	mapping background;
	mapping skills;
	object weapon;
	int skill;
	int force;
	int dodge;
	int secondary;
	int qi;
	string key;

	if (! caller_is_plot_controller() || ! objectp(enemy) ||
		phase < 0 || phase > MAX_BOSS_PHASE)
		return 0;
	profile = query_player_profile(me, profile_id, phase);
	if (! mapp(profile)) return 0;
	background = martial_backgrounds[profile["martial_background"]];
	if (! mapp(background)) return 0;
	skill = profile["skill_level"];
	force = profile["force_level"];
	dodge = larger(skill - 5, 45);
	secondary = larger(skill - 10, 45);
	qi = profile["max_qi"];

	skills = enemy->query_skills();
	if (mapp(skills)) foreach (key in keys(skills)) enemy->delete_skill(key);
	enemy->set("combat_exp", profile["combat_exp"]);
	enemy->set("level", 8 + profile["combat_tier"] + phase);
	enemy->set("str", 22 + profile["combat_tier"] * 2 + profile["meridian_boost"]);
	enemy->set("con", 22 + profile["combat_tier"] * 2 + profile["meridian_boost"]);
	enemy->set("dex", 24 + profile["combat_tier"] * 2 + profile["meridian_boost"]);
	enemy->set("int", 20 + profile["combat_tier"]);
	enemy->set_skill(background["style"], skill);
	enemy->set_skill(background["weapon_skill"], skill);
	enemy->set_skill("parry", skill);
	enemy->set_skill("dodge", dodge);
	enemy->set_skill(background["dodge_skill"], dodge);
	enemy->set_skill("force", force);
	enemy->set_skill(background["force_skill"], force);
	enemy->set_skill(background["secondary_style"], secondary);
	enemy->set_skill(background["secondary_skill"], secondary);
	foreach (key in keys(background["skill_map"]))
		enemy->map_skill(key, background["skill_map"][key]);
	foreach (key in keys(background["skill_prepare"]))
		enemy->prepare_skill(key, background["skill_prepare"][key]);
	enemy->set("max_qi", qi);
	enemy->set("eff_qi", qi);
	enemy->set("qi", qi);
	enemy->set("max_jing", qi / 2);
	enemy->set("eff_jing", qi / 2);
	enemy->set("jing", qi / 2);
	enemy->set("max_neili", profile["max_neili"]);
	enemy->set("neili", profile["max_neili"]);
	enemy->set("jiali", force / 5);
	enemy->set_temp("apply/attack", skill / 4 + profile["meridian_boost"] * 5);
	enemy->set_temp("apply/defense", skill / 4 + profile["meridian_boost"] * 5);
	enemy->set_temp("apply/damage", 8 + skill / 6);
	enemy->set("plot_combat/profile", profile_id);
	enemy->set("plot_combat/background", profile["martial_background"]);
	enemy->set("plot_combat/background_name", background["name"]);
	enemy->set("plot_combat/faction", background["faction"]);
	enemy->set("plot_combat/effective_level", profile["effective_level"]);
	enemy->set("plot_combat/tier", profile["combat_tier"]);
	enemy->set("plot_combat/meridian_boost", profile["meridian_boost"]);
	enemy->set("plot_combat/phase", phase);
	enemy->set("plot_combat/skill_level", skill);
	enemy->set("plot_combat/force_level", force);
	enemy->set("plot_combat/core_perform", profile["core_perform"]);
	enemy->set("plot_combat/phase_actions", profile["phase_actions"]);
	enemy->set("plot_combat/combat_exp", profile["combat_exp"]);
	enemy->set("plot_combat/max_qi", qi);
	enemy->set("attitude", "peaceful");
	install_combat_callback(enemy);
	if (background["weapon"] != "")
	{
		weapon = enemy->carry_object(background["weapon"]);
		if (! objectp(weapon) || ! weapon->wield()) return 0;
	}
	return copy_flat_mapping(profile);
}

int perform_enemy_action(object enemy, string action)
{
	string *actions;
	string function_name;
	mixed error;
	int result;
	int before_neili;

	if (! objectp(enemy) || ! stringp(enemy->query("plot_combat/profile")) ||
		! stringp(action)) return 0;
	actions = enemy->query("plot_combat/phase_actions");
	if (! arrayp(actions) || member_array(action, actions) < 0) return 0;
	before_neili = enemy->query("neili");
	if (sscanf(action, "force.%s", function_name) == 1)
		error = catch(result = enemy->exert_function(function_name));
	else error = catch(result = enemy->perform_action(action));
	if (error)
	{
		if (enemy->is_busy()) enemy->interrupt_busy(0);
		result = enemy->is_fighting() && enemy->query("neili") < before_neili;
	}
	enemy->set("plot_combat/last_action", action);
	enemy->set("plot_combat/last_action_ok", result ? 1 : 0);
	if (result) enemy->add("plot_combat/action_count", 1);
	return result ? 1 : 0;
}

int perform_enemy(object enemy)
{
	string *actions;
	int start;
	int index;

	if (! objectp(enemy) || ! enemy->is_fighting()) return 0;
	actions = enemy->query("plot_combat/phase_actions");
	if (! arrayp(actions) || ! sizeof(actions)) return 0;
	start = random(sizeof(actions));
	for (index = 0; index < sizeof(actions); index++)
		if (perform_enemy_action(enemy, actions[(start + index) % sizeof(actions)]))
			return 1;
	return 0;
}

int restore_enemy(object enemy)
{
	if (! caller_is_plot_controller() || ! objectp(enemy) ||
		! stringp(enemy->query("plot_combat/profile"))) return 0;
	enemy->remove_all_enemy(1);
	enemy->remove_all_killer();
	enemy->set("eff_qi", enemy->query("max_qi"));
	enemy->set("qi", enemy->query("max_qi"));
	enemy->set("eff_jing", enemy->query("max_jing"));
	enemy->set("jing", enemy->query("max_jing"));
	enemy->set("neili", enemy->query("max_neili"));
	enemy->set("attitude", "peaceful");
	install_combat_callback(enemy);
	return 1;
}

mapping selftest()
{
	mapping result;
	mapping profile;
	mapping background;
	mapping requirement;
	string profile_id;
	string background_id;
	string key;
	int phase;

	result = ([ "ok" : 1, "errors" : ({}) ]);
	foreach (profile_id in keys(profiles))
	{
		profile = profiles[profile_id];
		background_id = profile["martial_background"];
		background = martial_backgrounds[background_id];
		if (! mapp(background))
		{
			result["errors"] += ({ profile_id + ":background" });
			continue;
		}
		foreach (key in ({ "name", "faction", "style", "weapon_skill",
			"force_skill", "dodge_skill", "secondary_style", "secondary_skill" }))
			if (! stringp(background[key]) || background[key] == "")
				result["errors"] += ({ background_id + ":" + key });
		if (! mapp(background["skill_map"]) || ! mapp(background["skill_prepare"]) ||
			! arrayp(background["phase_requirements"]) ||
			! arrayp(background["phase_actions"]) ||
			sizeof(background["phase_requirements"]) <= profile["max_phase"] ||
			sizeof(background["phase_actions"]) <= profile["max_phase"])
			result["errors"] += ({ background_id + ":structure" });
		for (phase = 0; phase <= profile["max_phase"] &&
			phase < sizeof(background["phase_requirements"]); phase++)
		{
			requirement = background["phase_requirements"][phase];
			if (! mapp(requirement) || requirement["primary"] < 50 ||
				requirement["force"] < 50 || requirement["neili"] < 100 ||
				! stringp(requirement["core"]) ||
				member_array(requirement["core"], background["phase_actions"][phase]) < 0)
				result["errors"] += ({ background_id + ":phase" + phase });
		}
		if (profile["max_phase"] == 0 && sizeof(background["phase_requirements"]) != 1)
			result["errors"] += ({ profile_id + ":ordinary_phases" });
		if (profile["exp_ratio"] < 50 || profile["exp_ratio"] > 100 ||
			profile["skill_ratio"] < 50 || profile["skill_cap"] < 200)
			result["errors"] += ({ profile_id + ":scaling" });
	}
	if (profiles["ferry_matron"]["martial_background"] !=
		profiles["ledger_guard"]["martial_background"])
		result["errors"] += ({ "luo_qiniang:continuity" });
	if (profiles["ledger_master"]["martial_background"] != "gaibang_old_branch" ||
		profiles["ledger_master"]["max_phase"] != 2)
		result["errors"] += ({ "wen_shouzhuo:gaibang_old_branch" });
	result["ok"] = sizeof(result["errors"]) == 0;
	result["profiles"] = sizeof(keys(profiles));
	result["backgrounds"] = sizeof(keys(martial_backgrounds));
	return result;
}
