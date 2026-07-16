// Administrative controls for standalone AI players.

#include <ansi.h>

#define AI_PLAYER_D "/adm/daemons/ai_playerd"

private string resolve_id(string value)
{
	mapping profiles;

	profiles = AI_PLAYER_D->query_profiles();
	if (mapp(profiles[value]))
		return value;
	if (mapp(profiles["ai_" + value]))
		return "ai_" + value;
	return 0;
}

private void show_status()
{
	mapping profiles;
	mapping status;
	string id;
	string msg;

	profiles = AI_PLAYER_D->query_profiles();
	msg = AI_PLAYER_D->is_paused() ?
		HIR "AI 玩家已暂停。\n" NOR : HIG "AI 玩家正在运行。\n" NOR;
	foreach (id in keys(profiles))
	{
		status = AI_PLAYER_D->query_player_status(id);
		if (! mapp(status))
		{
			msg += sprintf("%-18s 未加载\n", id);
			continue;
		}
		msg += sprintf("%-12s %-14s %-10s %s\n",
			status["name"] + "(" + id + ")",
			status["room"], status["goal"] + "/" + status["period"],
			status["fighting"] ? "战斗中" :
			(status["busy"] ? "忙碌中" :
			 sprintf("空闲 事件:%d", status["pending_events"])));
	}
	write(msg);
}

private int show_inspect(string value)
{
	mapping status;
	mapping last_activity;
	string id;
	string *recent;
	int next_action;

	id = resolve_id(value);
	if (! stringp(id) || ! mapp(status = AI_PLAYER_D->query_player_status(id)))
		return notify_fail("没有这个 AI 玩家，或角色尚未加载。\n");
	next_action = status["next_action"] - time();
	if (next_action < 0)
		next_action = 0;
	recent = status["recent_rooms"];
	last_activity = status["last_activity"];
	write(sprintf(
		"%s(%s)\n"
		"位置：%s  %s\n"
		"目标：%s（已持续 %d 秒）\n"
		"意图：%s\n"
		"动作：%s（%d 秒前）\n"
		"气血：%d/%d  精神：%d/%d\n"
		"食物：%d/%d  饮水：%d/%d\n"
		"携带货币：%d 文\n"
		"关系记忆：%d  下次行动：%d 秒  跟踪：%s\n"
		"当前时段：%s  待处理事件：%d\n"
		"当前活动：%s  步骤：%s  目标：%s\n"
		"最近房间：%s\n"
		"最近活动：%s  结果：%s\n",
		status["name"], id, status["room"], status["room_path"],
		status["goal"], time() - status["goal_since"], status["intent"],
		status["last_action"], time() - status["last_action_at"],
		status["qi"], status["qi_max"], status["jing"], status["jing_max"],
		status["food"], status["food_max"],
		status["water"], status["water_max"], status["money"],
		status["relations"],
		next_action, status["tracing"] ? "开启" : "关闭",
		status["period"], status["pending_events"],
		status["activity"], status["activity_step"],
		status["activity_target"],
		arrayp(recent) && sizeof(recent) ? implode(recent, " -> ") : "无",
		mapp(last_activity) ? last_activity["name"] : "无",
		mapp(last_activity) ? last_activity["outcome"] : "无"));
	return 1;
}

private int show_metrics(string value)
{
	mapping profiles;
	mapping data;
	string *ids;
	string id;
	string msg;

	profiles = AI_PLAYER_D->query_profiles();
	if (stringp(value) && value != "")
	{
		id = resolve_id(value);
		if (! stringp(id))
			return notify_fail("没有这个 AI 玩家。\n");
		ids = ({ id });
	} else
		ids = keys(profiles);
	msg = "AI 运行指标：\n";
	foreach (id in ids)
	{
		data = AI_PLAYER_D->query_metrics(id);
		if (! mapp(data))
			continue;
		msg += sprintf(
			"%s uptime=%ds actions=%d failed=%d errors=%d respawns=%d\n"
			"  path searches=%d nodes=%d cache=%d hit/%d miss\n"
			"  events queued=%d handled=%d dropped=%d pending=%d\n"
			"  activities started=%d completed=%d interrupted=%d resumed=%d\n"
			"  behavior patrol_stops=%d rests=%d social_meetings=%d\n"
			"  route failures=%d relocations=%d\n"
			"  adapters attempts=%d success=%d precondition=%d command=%d postcondition=%d\n"
			"  scenarios started=%d passed=%d failed=%d\n",
			id, time() - data["started_at"], data["actions"],
			data["action_failures"], data["errors"], data["respawns"],
			data["path_searches"], data["path_nodes"],
			data["cache_hits"], data["cache_misses"],
			data["events_enqueued"], data["events_handled"],
			data["events_dropped"],
			sizeof(AI_PLAYER_D->query_event_queue(id)),
			data["activities_started"], data["activities_completed"],
			data["activities_interrupted"], data["activities_resumed"],
			data["patrol_stops"], data["rests_completed"],
			data["social_meetings"],
			data["route_failures"], data["relocations"],
			data["adapter_attempts"], data["adapter_successes"],
			data["adapter_precondition_failures"],
			data["adapter_command_failures"],
			data["adapter_postcondition_failures"],
			data["scenarios_started"], data["scenarios_passed"],
			data["scenarios_failed"]);
	}
	write(msg);
	return 1;
}

private int show_stability()
{
	mapping profiles;
	mapping data;
	mapping status;
	string id;
	int loaded;

	profiles = AI_PLAYER_D->query_profiles();
	loaded = sizeof(AI_PLAYER_D->query_ai_players());
	write(sprintf("AI_STABILITY objects=%d players=%d profiles=%d paused=%d\n",
		sizeof(objects()), loaded, sizeof(keys(profiles)),
		AI_PLAYER_D->is_paused()));
	foreach (id in sort_array(keys(profiles), (: strcmp :)))
	{
		data = AI_PLAYER_D->query_metrics(id);
		status = AI_PLAYER_D->query_player_status(id);
		write(sprintf(
			"AI_STABILITY_PLAYER id=%s loaded=%d errors=%d action_failures=%d "
			"route_failures=%d relocations=%d respawns=%d pending=%d actions=%d\n",
			id, mapp(status), data["errors"], data["action_failures"],
			data["route_failures"], data["relocations"], data["respawns"],
			sizeof(AI_PLAYER_D->query_event_queue(id)), data["actions"]));
	}
	return 1;
}

private int show_scenario(string value)
{
	mapping status;
	string id;

	id = resolve_id(value);
	if (! stringp(id) || ! mapp(status = AI_PLAYER_D->query_scenario_status(id)))
		return notify_fail("没有这个 AI 玩家，或尚未运行测试场景。\n");
	write(sprintf(
		"%s 隔离场景：%s\n"
		"状态：%s  开始事件：%s  结束事件：%s  已恢复：%s\n"
		"原房间：%s  待处理事件：%d  详情：%s\n",
		id, status["type"], status["status"],
		status["start_event"] ? "是" : "否",
		status["end_event"] ? "是" : "否",
		status["restored"] ? "是" : "否",
		status["original_room"], status["pending_events"], status["detail"]));
	return 1;
}

private int show_events(string value)
{
	mixed *queue;
	mapping event;
	string id;
	string msg;
	int i;

	id = resolve_id(value);
	if (! stringp(id))
		return notify_fail("没有这个 AI 玩家。\n");
	queue = AI_PLAYER_D->query_event_queue(id);
	msg = id + " 的待处理事件：\n";
	if (! arrayp(queue) || ! sizeof(queue))
		msg += "  无\n";
	else
	{
		for (i = 0; i < sizeof(queue); i++)
		{
			event = queue[i];
			msg += sprintf("  %d. %s p=%d age=%ds source=%s(%s) detail=%s\n",
				i + 1, event["type"], event["priority"],
				time() - event["at"], event["source_name"],
				event["source_id"], event["detail"]);
		}
	}
	write(msg);
	return 1;
}

private int show_validation()
{
	string *issues;

	issues = AI_PLAYER_D->validate_profiles();
	if (! arrayp(issues) || ! sizeof(issues))
		write("AI 配置校验通过：5 个角色的日程房间与往返路径均有效。\n");
	else
		write("AI 配置校验发现问题：\n  " + implode(issues, "\n  ") + "\n");
	return 1;
}

private int show_selftest(string value)
{
	mapping profiles;
	mapping report;
	mapping checks;
	string *ids;
	string id;
	string key;
	string msg;

	profiles = AI_PLAYER_D->query_profiles();
	if (stringp(value) && value != "")
	{
		id = resolve_id(value);
		if (! stringp(id))
			return notify_fail("没有这个 AI 玩家。\n");
		ids = ({ id });
	} else
		ids = keys(profiles);
	msg = "AI 自检：\n";
	foreach (id in ids)
	{
		report = AI_PLAYER_D->selftest_player(id);
		msg += sprintf("%s: %s\n", id,
			report["passed"] ? "通过" : "失败");
		checks = report["checks"];
		foreach (key in keys(checks))
			msg += sprintf("  %s=%s", key, checks[key] ? "ok" : "fail");
		if (sizeof(keys(checks)))
			msg += "\n";
		if (arrayp(report["issues"]) && sizeof(report["issues"]))
			msg += "  " + implode(report["issues"], "\n  ") + "\n";
	}
	write(msg);
	return 1;
}

int main(object me, string arg)
{
	string id;
	string mode;
	string value;

	if (! arg || arg == "status")
	{
		show_status();
		return 1;
	}
	if (arg == "pause")
	{
		AI_PLAYER_D->set_paused(1);
		write("AI 玩家已暂停；保存和死亡恢复仍继续运行。\n");
		return 1;
	}
	if (arg == "resume")
	{
		AI_PLAYER_D->set_paused(0);
		write("AI 玩家已恢复运行。\n");
		return 1;
	}
	if (arg == "reload")
	{
		AI_PLAYER_D->reload_players();
		write("AI 玩家已重新检查并加载。\n");
		return 1;
	}
	if (arg == "save")
	{
		AI_PLAYER_D->save_all_players();
		write("AI 玩家档案已保存。\n");
		return 1;
	}
	if (arg == "metrics")
		return show_metrics(0);
	if (sscanf(arg, "metrics %s", value) == 1)
		return show_metrics(value);
	if (sscanf(arg, "events %s", value) == 1)
		return show_events(value);
	if (arg == "validate")
		return show_validation();
	if (arg == "selftest")
		return show_selftest(0);
	if (arg == "stability")
		return show_stability();
	if (sscanf(arg, "selftest %s", value) == 1)
		return show_selftest(value);
	if (sscanf(arg, "scenario combat %s", value) == 1)
	{
		id = resolve_id(value);
		if (! stringp(id) || ! AI_PLAYER_D->start_combat_scenario(id))
			return notify_fail("无法启动隔离战斗场景：角色忙碌、战斗中、场景已占用或测试房间不可用。\n");
		write(id + " 已启动隔离非致死战斗场景。\n");
		return 1;
	}
	if (sscanf(arg, "scenario status %s", value) == 1)
		return show_scenario(value);
	if (sscanf(arg, "activity supplies %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (mode != "seed" || ! stringp(id) ||
			! AI_PLAYER_D->start_supply_activity(id, 1))
			return notify_fail("无法启动补给活动：角色忙碌、战斗中或没有补给活动配置。\n");
		write(id + " 已安排补给活动并注入一次测试银两。\n");
		return 1;
	}
	if (sscanf(arg, "activity supplies %s", value) == 1)
	{
		id = resolve_id(value);
		if (! stringp(id) || ! AI_PLAYER_D->start_supply_activity(id, 0))
			return notify_fail("无法启动补给活动：角色忙碌、战斗中或没有补给活动配置。\n");
		write(id + " 已强制安排一次补给活动。\n");
		return 1;
	}
	if (sscanf(arg, "activity run %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (! stringp(id) || ! AI_PLAYER_D->start_profile_activity(id, mode))
			return notify_fail("无法启动角色活动：角色忙碌、战斗中或没有该活动。\n");
		write(id + " 已强制安排角色活动 " + mode + "。\n");
		return 1;
	}
	if (sscanf(arg, "inspect %s", value) == 1)
		return show_inspect(value);
	if (sscanf(arg, "trace %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (! stringp(id) || (mode != "on" && mode != "off"))
			return notify_fail("用法：aiplayer trace <id> <on|off>\n");
		AI_PLAYER_D->set_trace(id, mode == "on");
		write(id + " 的动作跟踪已" + (mode == "on" ? "开启" : "关闭") + "。\n");
		return 1;
	}
	if (sscanf(arg, "home %s", value) == 1)
	{
		id = resolve_id(value);
		if (! stringp(id) || ! AI_PLAYER_D->move_player_home(id))
			return notify_fail("无法将该 AI 玩家送回常驻点。\n");
		write(id + " 已返回常驻点并保存。\n");
		return 1;
	}
	if (sscanf(arg, "reset %s", value) == 1)
	{
		id = resolve_id(value);
		if (! stringp(id) || ! AI_PLAYER_D->reset_player(id))
			return notify_fail("无法复位该 AI 玩家。\n");
		write(id + " 的行为状态、生命状态和位置已复位。\n");
		return 1;
	}
	return notify_fail("用法：aiplayer [status|pause|resume|reload|save|metrics [id]|events <id>|validate|selftest [id]|stability|scenario combat <id>|scenario status <id>|activity supplies <id>|activity supplies <id> seed|activity run <id> <activity>|inspect <id>|trace <id> <on|off>|home <id>|reset <id>]\n");
}

int help(object me)
{
	write("用法：aiplayer [status|pause|resume|reload|save|metrics [id]|events <id>|validate|selftest [id]|stability|scenario combat <id>|scenario status <id>|activity supplies <id>|activity supplies <id> seed|activity run <id> <activity>|inspect <id>|trace <id> <on|off>|home <id>|reset <id>]\n");
	return 1;
}
