// Administrative controls for standalone AI players.

#include <ansi.h>

#define AI_PLAYER_D "/adm/daemons/ai_playerd"
#define AI_TRAVEL_D "/adm/daemons/ai_travel"

private int show_travel_route(string from, string to, string role);

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
		"战斗决策：%s  攻击者：%s  原因：%s  撤退尝试：%d\n"
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
		status["combat_state"], status["combat_attacker"],
		status["combat_reason"], status["combat_retreat_attempts"],
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
			"  recovery migrations=%d cancellations=%d checkpoints=%d failed=%d reconciliations=%d\n"
			"  route failures=%d relocations=%d\n"
			"  adapters attempts=%d success=%d precondition=%d command=%d postcondition=%d\n"
			"  defense observed=%d defended=%d retreat_decisions=%d attempts=%d success=%d failed=%d trapped=%d recoveries=%d\n"
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
			data["activity_migrations"], data["activity_cancellations"],
			data["activity_checkpoints"],
			data["activity_checkpoint_failures"],
			data["social_reconciliations"],
			data["route_failures"], data["relocations"],
			data["adapter_attempts"], data["adapter_successes"],
			data["adapter_precondition_failures"],
			data["adapter_command_failures"],
			data["adapter_postcondition_failures"],
			data["combat_observations"], data["combat_defenses"],
			data["combat_retreat_decisions"],
			data["combat_retreat_attempts"],
			data["combat_retreat_successes"],
			data["combat_retreat_failures"], data["combat_trapped"],
			data["combat_recoveries"],
			data["scenarios_started"], data["scenarios_passed"],
			data["scenarios_failed"]);
	}
	write(msg);
	return 1;
}

private int show_recovery(string value)
{
	mapping status;
	string id;
	string activity;
	string action;
	string step;
	string partner;
	string room;
	string outcome;
	string combat;

	id = resolve_id(value);
	if (! stringp(id) || ! mapp(status = AI_PLAYER_D->query_recovery_status(id)))
		return notify_fail("没有这个 AI 玩家，或角色尚未加载。\n");
	activity = stringp(status["activity"]) ? status["activity"] : "none";
	action = stringp(status["action"]) ? status["action"] : "none";
	step = stringp(status["step"]) ? status["step"] : "none";
	partner = stringp(status["partner"]) ? status["partner"] : "none";
	room = stringp(status["room"]) ? status["room"] : "none";
	outcome = stringp(status["last_outcome"]) ?
		status["last_outcome"] : "none";
	combat = stringp(status["combat_state"]) ? status["combat_state"] : "idle";
	write(sprintf(
		"AI_RECOVERY id=%s schema=%d active=%d activity=%s action=%s step=%s "
		"partner=%s synthetic=%d room=%s last_outcome=%s money=%d "
		"food_items=%d water_items=%d food=%d water=%d busy=%d fighting=%d "
		"combat=%s pending=%d save_in=%d scenario=%d\n",
		id, status["schema"], status["active"], activity, action, step,
		partner, status["synthetic"], room, outcome, status["money"],
		status["food_items"], status["water_items"], status["food"],
		status["water"], status["busy"], status["fighting"], combat,
		status["pending"], status["save_in"], status["scenario"]));
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
		"决策：%s  尝试撤退：%s  已撤退：%s  受困：%s\n"
		"失能：%s  死亡：%s  复活：%s\n"
		"原房间：%s  待处理事件：%d  详情：%s\n"
		"AI_SCENARIO id=%s mode=%s status=%s decision=%s attempted=%d attempts=%d "
		"retreated=%d trapped=%d incapacitated=%d death=%d respawned=%d restored=%d\n",
		id, status["type"], status["status"],
		status["start_event"] ? "是" : "否",
		status["end_event"] ? "是" : "否",
		status["restored"] ? "是" : "否",
		status["decision"],
		status["retreat_attempted"] ? "是" : "否",
		status["retreated"] ? "是" : "否",
		status["trapped"] ? "是" : "否",
		status["incapacitated"] ? "是" : "否",
		status["death_observed"] ? "是" : "否",
		status["respawned"] ? "是" : "否",
		status["original_room"], status["pending_events"], status["detail"],
		id, status["type"], status["status"], status["decision"],
		status["retreat_attempted"], status["retreat_attempts"], status["retreated"],
		status["trapped"], status["incapacitated"],
		status["death_observed"], status["respawned"], status["restored"]));
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

private int show_travel_schema()
{
	mapping schema;
	mapping budget;

	schema = AI_TRAVEL_D->query_schema();
	budget = schema["budget"];
	write(sprintf(
		"AI_TRAVEL_SCHEMA capability=%s schema=%d max_nodes=%d max_depth=%d "
		"max_cost=%d max_risk=%d max_transfers=%d max_time=%d max_retries=%d\n",
		schema["capability_version"], schema["schema_version"],
		schema["max_nodes"], schema["max_depth"], budget["max_cost"],
		budget["max_risk"], budget["max_transfers"], budget["max_time"],
		budget["max_retries"]));
	return 1;
}

private int show_travel_validation()
{
	string *issues;
	mapping stats;

	issues = AI_TRAVEL_D->validate_graph();
	stats = AI_TRAVEL_D->query_graph_stats();
	if (! arrayp(issues) || ! sizeof(issues))
		write(sprintf("AI_TRAVEL_VALIDATE status=passed regions=%d nodes=%d edges=%d routes=%d\n",
			stats["regions"], stats["nodes"], stats["edges"], stats["routes"]));
	else
		write("AI_TRAVEL_VALIDATE status=failed issues=" + implode(issues, ",") + "\n");
	return 1;
}

private int show_registered_travel_route(string route_id)
{
	mapping route;

	route = AI_TRAVEL_D->query_route(route_id);
	if (! mapp(route))
		return notify_fail("没有这个注册旅行路线。\n");
	return show_travel_route(route["from"], route["to"], route["role"]);
}

private int show_travel_node(string value)
{
	mapping node;

	node = AI_TRAVEL_D->query_node(value);
	if (! mapp(node))
		return notify_fail("没有这个旅行节点。\n");
	write(sprintf("AI_TRAVEL_NODE id=%s region=%s name=%s room=%s kind=%s enabled=%d safe=%d\n",
		node["id"], node["region"], node["name"], node["room"], node["kind"],
		node["enabled"], node["safe"]));
	return 1;
}

private int show_travel_route(string from, string to, string role)
{
	mapping route;
	string nodes;
	string edges;

	route = AI_TRAVEL_D->plan_route(from, to, role, 0);
	if (! mapp(route))
		return notify_fail("旅行规划器没有返回结果。\n");
	nodes = arrayp(route["nodes"]) ? implode(route["nodes"], ">") : "";
	edges = arrayp(route["edges"]) ? implode(route["edges"], ">") : "";
	write(sprintf(
		"AI_TRAVEL_ROUTE from=%s to=%s role=%s state=%s cancel_reason=%s "
		"cost=%d risk=%d time=%d transfers=%d retries=%d nodes=%s edges=%s\n",
		from, to, role, route["state"], route["cancel_reason"],
		intp(route["total_cost"]) ? route["total_cost"] : 0,
		intp(route["total_risk"]) ? route["total_risk"] : 0,
		intp(route["total_time"]) ? route["total_time"] : 0,
		intp(route["transfers"]) ? route["transfers"] : 0,
		intp(route["retries"]) ? route["retries"] : 0, nodes, edges));
	return 1;
}

private int show_travel_selftest()
{
	mapping report;
	mapping checks;
	string key;

	report = AI_TRAVEL_D->selftest();
	checks = report["checks"];
	write(sprintf("AI_TRAVEL_SELFTEST capability=%s schema=%d status=%s\n",
		report["capability_version"], report["schema_version"],
		report["passed"] ? "passed" : "failed"));
	foreach (key in keys(checks))
		write(sprintf("  %s=%s\n", key, checks[key] ? "ok" : "fail"));
	if (arrayp(report["issues"]) && sizeof(report["issues"]))
		write("  issues=" + implode(report["issues"], ",") + "\n");
	return report["passed"];
}

private int show_travel_status(string value)
{
	mapping status;
	mapping schema;
	string actor_id;

	actor_id = resolve_id(value);
	if (! stringp(actor_id) ||
		! mapp(status = AI_TRAVEL_D->query_travel_status(actor_id)))
		return notify_fail("没有这个 AI 玩家或旅行状态。\n");
	schema = AI_TRAVEL_D->query_schema();
	write(sprintf(
		"AI_TRAVEL_STATUS actor=%s capability=%s schema=%d state=%s route=%s "
		"node=%s edge_index=%d charged=%d refunded=%d retries=%d trigger=%s "
		"event_seq=%d last_event=%s cancel_reason=%s\n",
		actor_id, schema["capability_version"], status["schema_version"],
		status["state"], status["route_id"],
		status["current_node"], status["edge_index"], status["charged_total"],
		status["refunded"], status["retries"],
		stringp(status["trigger"]) ? status["trigger"] : "none",
		intp(status["event_seq"]) ? status["event_seq"] : 0,
		stringp(status["last_event"]) ? status["last_event"] : "none",
		status["cancel_reason"]));
	return 1;
}

private int show_travel_recovery(string value)
{
	mapping status;
	mapping schema;
	string actor_id;

	actor_id = resolve_id(value);
	if (! stringp(actor_id) ||
		! mapp(status = AI_TRAVEL_D->query_travel_status(actor_id)))
		return notify_fail("没有这个 AI 玩家或旅行状态。\n");
	schema = AI_TRAVEL_D->query_schema();
	write(sprintf("AI_TRAVEL_RECOVERY actor=%s capability=%s schema=%d state=%s route=%s "
		"node=%s edge_index=%d charged=%d refunded=%d recovery=%s "
		"migrated_from=%d event_seq=%d last_event=%s cancel_reason=%s\n",
		actor_id, schema["capability_version"], status["schema_version"],
		status["state"], status["route_id"],
		status["current_node"], status["edge_index"], status["charged_total"],
		status["refunded"], stringp(status["recovery"]) ? status["recovery"] : "none",
		intp(status["migrated_from_schema"]) ? status["migrated_from_schema"] : 0,
		intp(status["event_seq"]) ? status["event_seq"] : 0,
		stringp(status["last_event"]) ? status["last_event"] : "none",
		status["cancel_reason"]));
	return 1;
}

private int run_travel_recovery(string value)
{
	mapping result;
	string actor_id;

	actor_id = resolve_id(value);
	if (! stringp(actor_id) || ! mapp(result = AI_TRAVEL_D->recover_travel(actor_id)))
		return notify_fail("无法恢复该 AI 玩家旅行。\n");
	write(sprintf("AI_TRAVEL_RECOVER actor=%s state=%s route=%s node=%s edge_index=%d "
		"charged=%d refunded=%d recovery=%s cancel_reason=%s\n",
		actor_id, result["state"], stringp(result["route_id"]) ? result["route_id"] : "none",
		stringp(result["current_node"]) ? result["current_node"] : "none",
		intp(result["edge_index"]) ? result["edge_index"] : 0,
		intp(result["charged_total"]) ? result["charged_total"] : 0,
		intp(result["refunded"]) ? result["refunded"] : 0,
		stringp(result["recovery"]) ? result["recovery"] : "none",
		stringp(result["cancel_reason"]) ? result["cancel_reason"] : "none"));
	return 1;
}

private int run_travel(string route_id, string mode)
{
	mapping result;
	string fault_mode;
	int seed_money;

	if (stringp(mode) && member_array(mode,
		({ "seed", "insufficient", "edge_disabled" })) == -1)
		return notify_fail("旅行闭环模式必须是 seed、insufficient 或 edge_disabled。\n");
	seed_money = mode == "seed" || mode == "edge_disabled";
	fault_mode = mode == "insufficient" || mode == "edge_disabled" ? mode : 0;
	result = AI_TRAVEL_D->run_registered_travel(route_id, seed_money, fault_mode);
	if (! mapp(result))
		return notify_fail("旅行执行器没有返回结果。\n");
	write(sprintf(
		"AI_TRAVEL_RUN route=%s actor=%s state=%s node=%s edge_index=%d "
		"charged=%d refunded=%d retries=%d cancel_reason=%s\n",
		route_id, stringp(result["actor_id"]) ? result["actor_id"] : "none",
		result["state"], stringp(result["current_node"]) ? result["current_node"] : "none",
		intp(result["edge_index"]) ? result["edge_index"] : 0,
		intp(result["charged_total"]) ? result["charged_total"] : 0,
		intp(result["refunded"]) ? result["refunded"] : 0,
		intp(result["retries"]) ? result["retries"] : 0,
		stringp(result["cancel_reason"]) ? result["cancel_reason"] : "none"));
	return 1;
}

private int run_travel_schedule(string value, string period, string mode)
{
	mapping result;
	string actor_id;

	actor_id = resolve_id(value);
	if (! stringp(actor_id) ||
		member_array(period, ({ "morning", "day", "evening", "night" })) == -1 ||
		(stringp(mode) && mode != "seed"))
		return notify_fail("用法：aiplayer travel schedule <id> <morning|day|evening|night> [seed]\n");
	result = AI_TRAVEL_D->run_schedule_period(actor_id, period, mode == "seed");
	if (! mapp(result))
		return notify_fail("旅行日程执行器没有返回结果。\n");
	write(sprintf(
		"AI_TRAVEL_SCHEDULE actor=%s period=%s state=%s route=%s node=%s "
		"charged=%d refunded=%d trigger=%s event_seq=%d cancel_reason=%s\n",
		actor_id, period, result["state"],
		stringp(result["route_id"]) ? result["route_id"] : "none",
		stringp(result["current_node"]) ? result["current_node"] : "none",
		intp(result["charged_total"]) ? result["charged_total"] : 0,
		intp(result["refunded"]) ? result["refunded"] : 0,
		stringp(result["trigger"]) ? result["trigger"] : "none",
		intp(result["event_seq"]) ? result["event_seq"] : 0,
		stringp(result["cancel_reason"]) ? result["cancel_reason"] : "none"));
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
	if (arg == "travel schema")
		return show_travel_schema();
	if (arg == "travel validate")
		return show_travel_validation();
	if (arg == "travel selftest")
		return show_travel_selftest();
	if (sscanf(arg, "travel recovery prepare %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (! stringp(id) || member_array(mode, ({ "planned", "charged",
		    "executing", "arrived", "legacy", "removed", "disabled", "invalid",
		    "future", "legacy_invalid", "legacy_removed" })) == -1 ||
		    ! AI_TRAVEL_D->prepare_recovery_test(id, mode))
			return notify_fail("无法准备旅行恢复测试：模式必须是 planned、charged、executing、arrived、legacy、removed、disabled、invalid、future、legacy_invalid 或 legacy_removed。\n");
		write(id + " 已准备旅行恢复测试 " + mode + "。\n");
		return 1;
	}
	if (sscanf(arg, "travel recovery %s", value) == 1)
		return show_travel_recovery(value);
	if (sscanf(arg, "travel recover %s", value) == 1)
		return run_travel_recovery(value);
	if (sscanf(arg, "travel status %s", value) == 1)
		return show_travel_status(value);
	if (sscanf(arg, "travel schedule %s %s %s", value, id, mode) == 3)
		return run_travel_schedule(value, id, mode);
	if (sscanf(arg, "travel schedule %s %s", value, id) == 2)
		return run_travel_schedule(value, id, 0);
	if (sscanf(arg, "travel auto %s %s", value, id) == 2)
	{
		value = resolve_id(value);
		if (! stringp(value) ||
			member_array(id, ({ "morning", "day", "evening", "night" })) == -1 ||
			! AI_TRAVEL_D->prepare_auto_schedule_test(value, id))
			return notify_fail("无法准备自动旅行日程测试。\n");
		write(value + " 已准备自动旅行日程测试 " + id + "。\n");
		return 1;
	}
	if (sscanf(arg, "travel run %s %s", value, mode) == 2)
		return run_travel(value, mode);
	if (sscanf(arg, "travel run %s", value) == 1)
		return run_travel(value, 0);
	if (sscanf(arg, "travel inspect %s", value) == 1)
		return show_travel_node(value);
	if (sscanf(arg, "travel route %s %s %s", value, mode, id) == 3)
		return show_travel_route(value, mode, id);
	if (sscanf(arg, "travel route %s %s", value, mode) == 2)
		return show_travel_route(value, mode, "traveler");
	if (sscanf(arg, "travel route %s", value) == 1)
		return show_registered_travel_route(value);
	if (arg == "metrics")
		return show_metrics(0);
	if (sscanf(arg, "metrics %s", value) == 1)
		return show_metrics(value);
	if (sscanf(arg, "events %s", value) == 1)
		return show_events(value);
	if (sscanf(arg, "recovery prepare %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (! stringp(id) || (mode != "supplies" && mode != "savepoint" &&
		    mode != "legacy" && mode != "invalid") ||
		    ! AI_PLAYER_D->prepare_recovery_test(id, mode))
			return notify_fail("无法准备恢复测试：模式必须是 supplies、savepoint、legacy 或 invalid，且角色状态必须匹配。\n");
		write(id + " 已准备恢复测试 " + mode + "。\n");
		return 1;
	}
	if (sscanf(arg, "recovery %s", value) == 1)
		return show_recovery(value);
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
	if (sscanf(arg, "scenario defense %s %s", value, mode) == 2)
	{
		id = resolve_id(value);
		if (! stringp(id) || member_array(mode, ({ "defend", "retreat",
		    "noexit", "blocked", "unconscious", "death" })) == -1 ||
		    ! AI_PLAYER_D->start_combat_scenario(id, mode))
			return notify_fail("无法启动有限自卫场景；模式必须是 defend、retreat、noexit、blocked、unconscious 或 death。\n");
		write(id + " 已启动有限自卫场景 " + mode + "。\n");
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
	return notify_fail("用法：aiplayer [status|pause|resume|reload|save|travel schema|travel validate|travel selftest|travel inspect <node>|travel route <route_id>|travel route <from> <to> [role]|travel run <route_id> [seed]|travel status <id>|travel recovery <id>|travel recover <id>|travel recovery prepare <id> <planned|charged|executing|arrived|legacy|removed|disabled|invalid|future|legacy_invalid|legacy_removed>|metrics [id]|events <id>|recovery <id>|recovery prepare <id> <supplies|savepoint|legacy|invalid>|validate|selftest [id]|stability|scenario combat <id>|scenario defense <id> <defend|retreat|noexit|blocked|unconscious|death>|scenario status <id>|activity supplies <id>|activity supplies <id> seed|activity run <id> <activity>|inspect <id>|trace <id> <on|off>|home <id>|reset <id>]\n");
}

int help(object me)
{
	write("用法：aiplayer [status|pause|resume|reload|save|travel schema|travel validate|travel selftest|travel inspect <node>|travel route <route_id>|travel route <from> <to> [role]|travel run <route_id> [seed]|travel status <id>|travel recovery <id>|travel recover <id>|travel recovery prepare <id> <planned|charged|executing|arrived|legacy|removed|disabled|invalid|future|legacy_invalid|legacy_removed>|metrics [id]|events <id>|recovery <id>|recovery prepare <id> <supplies|savepoint|legacy|invalid>|validate|selftest [id]|stability|scenario combat <id>|scenario defense <id> <defend|retreat|noexit|blocked|unconscious|death>|scenario status <id>|activity supplies <id>|activity supplies <id> seed|activity run <id> <activity>|inspect <id>|trace <id> <on|off>|home <id>|reset <id>]\n");
	return 1;
}
