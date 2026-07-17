// Frozen bounded region graph and approved real-world schedule for AI v2.4.

#include <globals.h>

#define AI_TRAVEL_SCHEMA_VERSION 2
#define AI_TRAVEL_CAPABILITY_VERSION "v2.4"
#define AI_TRAVEL_MAX_NODES 24
#define AI_TRAVEL_MAX_DEPTH 8
#define AI_TRAVEL_MAX_COST 500
#define AI_TRAVEL_MAX_RISK 3
#define AI_TRAVEL_MAX_TRANSFERS 4
#define AI_TRAVEL_MAX_TIME 1800
#define AI_TRAVEL_MAX_RETRIES 2
#define AI_PLAYER_D "/adm/daemons/ai_playerd"
#define AI_TRAVEL_ACTOR "ai_qingfeng"
#define AI_TRAVEL_SCHEDULE_INTERVAL 10
#define AI_TRAVEL_SCHEDULE_RETRY_DELAY 60

mapping regions = ([
	"jiangnan" : ([ "id" : "jiangnan", "name" : "江南", "enabled" : 1 ]),
	"wudang" : ([ "id" : "wudang", "name" : "武当", "enabled" : 1 ]),
	"hangzhou" : ([ "id" : "hangzhou", "name" : "杭州", "enabled" : 1 ]),
	"shaolin" : ([ "id" : "shaolin", "name" : "少林", "enabled" : 1 ]),
]);

mapping nodes = ([
	"city_gate" : ([ "id" : "city_gate", "region" : "jiangnan",
		"name" : "扬州东门", "room" : "/d/city/dongmen", "kind" : "walk",
		"enabled" : 1, "safe" : 1 ]),
	"city_station" : ([ "id" : "city_station", "region" : "jiangnan",
		"name" : "扬州驿站", "room" : "/d/city/guangchang", "kind" : "carriage",
		"enabled" : 1, "safe" : 1 ]),
	"city_wharf" : ([ "id" : "city_wharf", "region" : "jiangnan",
		"name" : "扬州渡口", "room" : "/d/city/dongmen", "kind" : "boat",
		"enabled" : 1, "safe" : 1 ]),
	"wudang_foothill" : ([ "id" : "wudang_foothill", "region" : "wudang",
		"name" : "武当山脚", "room" : "/d/wudang/zixiaogate", "kind" : "carriage",
		"enabled" : 1, "safe" : 1 ]),
	"wudang_gate" : ([ "id" : "wudang_gate", "region" : "wudang",
		"name" : "武当山门", "room" : "/d/wudang/jiejianyan", "kind" : "walk",
		"enabled" : 1, "safe" : 1 ]),
	"hangzhou_wharf" : ([ "id" : "hangzhou_wharf", "region" : "hangzhou",
		"name" : "杭州码头", "room" : "/d/hangzhou/road10", "kind" : "boat",
		"enabled" : 1, "safe" : 1 ]),
	"shaolin_station" : ([ "id" : "shaolin_station", "region" : "shaolin",
		"name" : "少林驿站", "room" : "/d/shaolin/shanmen", "kind" : "carriage",
		"enabled" : 1, "safe" : 1 ]),
	"shaolin_gate" : ([ "id" : "shaolin_gate", "region" : "shaolin",
		"name" : "少林山门", "room" : "/d/shaolin/shanmen", "kind" : "walk",
		"enabled" : 1, "safe" : 1 ]),
]);

mapping edges = ([
	"city_gate_to_station" : ([ "id" : "city_gate_to_station", "from" : "city_gate",
		"to" : "city_station", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 120, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"station_to_city_gate" : ([ "id" : "station_to_city_gate", "from" : "city_station",
		"to" : "city_gate", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 120, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"city_station_to_wudang" : ([ "id" : "city_station_to_wudang", "from" : "city_station",
		"to" : "wudang_foothill", "transport" : "carriage", "cost" : 120, "risk" : 1,
		"time" : 600, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"wudang_to_city_station" : ([ "id" : "wudang_to_city_station", "from" : "wudang_foothill",
		"to" : "city_station", "transport" : "carriage", "cost" : 120, "risk" : 1,
		"time" : 600, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"wudang_foothill_to_gate" : ([ "id" : "wudang_foothill_to_gate", "from" : "wudang_foothill",
		"to" : "wudang_gate", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 240, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"wudang_gate_to_foothill" : ([ "id" : "wudang_gate_to_foothill", "from" : "wudang_gate",
		"to" : "wudang_foothill", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 240, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"city_gate_to_wharf" : ([ "id" : "city_gate_to_wharf", "from" : "city_gate",
		"to" : "city_wharf", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 180, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"wharf_to_city_gate" : ([ "id" : "wharf_to_city_gate", "from" : "city_wharf",
		"to" : "city_gate", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 180, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"city_wharf_to_hangzhou" : ([ "id" : "city_wharf_to_hangzhou", "from" : "city_wharf",
		"to" : "hangzhou_wharf", "transport" : "boat", "cost" : 80, "risk" : 1,
		"time" : 720, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"hangzhou_to_city_wharf" : ([ "id" : "hangzhou_to_city_wharf", "from" : "hangzhou_wharf",
		"to" : "city_wharf", "transport" : "boat", "cost" : 80, "risk" : 1,
		"time" : 720, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"hangzhou_to_shaolin" : ([ "id" : "hangzhou_to_shaolin", "from" : "hangzhou_wharf",
		"to" : "shaolin_station", "transport" : "carriage", "cost" : 150, "risk" : 2,
		"time" : 780, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"shaolin_to_hangzhou" : ([ "id" : "shaolin_to_hangzhou", "from" : "shaolin_station",
		"to" : "hangzhou_wharf", "transport" : "carriage", "cost" : 150, "risk" : 2,
		"time" : 780, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"shaolin_station_to_gate" : ([ "id" : "shaolin_station_to_gate", "from" : "shaolin_station",
		"to" : "shaolin_gate", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 120, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
	"shaolin_gate_to_station" : ([ "id" : "shaolin_gate_to_station", "from" : "shaolin_gate",
		"to" : "shaolin_station", "transport" : "walk", "cost" : 0, "risk" : 0,
		"time" : 120, "enabled" : 1, "prohibited" : 0,
		"allowed_roles" : ({ "traveler", "test" }) ]),
]);

mapping adapters = ([
	"test" : ([ "id" : "test", "name" : "确定性测试适配器", "side_effects" : 0,
		"supports" : ({ "walk", "carriage", "boat", "teleport" }) ]),
]);

mapping routes = ([
	"test_city_to_wudang" : ([ "id" : "test_city_to_wudang",
		"from" : "city_gate", "to" : "wudang_gate", "role" : "test",
		"enabled" : 1, "test_only" : 1 ]),
	"qingfeng_city_to_wudang" : ([ "id" : "qingfeng_city_to_wudang",
		"actor_id" : AI_TRAVEL_ACTOR, "from" : "city_station",
		"to" : "wudang_gate", "safe_return" : "city_station",
		"role" : "traveler", "enabled" : 1, "test_only" : 0 ]),
	"qingfeng_wudang_to_city" : ([ "id" : "qingfeng_wudang_to_city",
		"actor_id" : AI_TRAVEL_ACTOR, "from" : "wudang_gate",
		"to" : "city_station", "safe_return" : "wudang_gate",
		"role" : "traveler", "enabled" : 1, "test_only" : 0 ]),
]);

mapping schedules = ([
	AI_TRAVEL_ACTOR : ([ "actor_id" : AI_TRAVEL_ACTOR, "enabled" : 1,
		"periods" : ([
			"morning" : ([ "node" : "city_station",
				"route" : "qingfeng_wudang_to_city" ]),
			"day" : ([ "node" : "wudang_gate",
				"route" : "qingfeng_city_to_wudang" ]),
			"evening" : ([ "node" : "city_station",
				"route" : "qingfeng_wudang_to_city" ]),
			"night" : ([ "node" : "city_station",
				"route" : "qingfeng_wudang_to_city" ]),
		]) ]),
]);

mapping next_schedule_attempt = ([]);
mapping schedule_test_period = ([]);
mapping travel_recovery_guard = ([]);

private mapping execute_registered_travel(string route_id, int seed_money,
	string fault_mode, string trigger);
private mapping resume_travel(object me, mapping config, mapping state,
	string fault_mode);

void create()
{
	seteuid(ROOT_UID);
	set_heart_beat(AI_TRAVEL_SCHEDULE_INTERVAL);
}

private int has_role(mapping edge, string role)
{
	return arrayp(edge["allowed_roles"]) &&
		member_array(role, edge["allowed_roles"]) != -1;
}

private mapping default_budget()
{
	return ([ "max_cost" : AI_TRAVEL_MAX_COST, "max_risk" : AI_TRAVEL_MAX_RISK,
		"max_transfers" : AI_TRAVEL_MAX_TRANSFERS, "max_time" : AI_TRAVEL_MAX_TIME,
		"max_retries" : AI_TRAVEL_MAX_RETRIES ]);
}

private mixed *outgoing(string node_id, string role)
{
	mixed *result;
	mapping edge;
	string id;

	result = ({});
	foreach (id in sort_array(keys(edges), (: strcmp :)))
	{
		edge = edges[id];
		if (edge["from"] == node_id && edge["enabled"] &&
			! edge["prohibited"] && has_role(edge, role))
			result += ({ edge });
	}
	return result;
}

mapping query_schema()
{
	return ([ "capability_version" : AI_TRAVEL_CAPABILITY_VERSION,
		"schema_version" : AI_TRAVEL_SCHEMA_VERSION,
		"max_nodes" : AI_TRAVEL_MAX_NODES, "max_depth" : AI_TRAVEL_MAX_DEPTH,
		"budget" : default_budget(), "states" : ({ "planned", "executing",
			"charged", "arrived", "cancelled" }), "checkpoint_fields" : ({ "actor_id",
			"route_id", "state", "current_node", "edge_index", "charged_edges",
			"charged_total", "refunded", "retries", "started_at", "updated_at",
			"trigger", "event_seq", "last_event", "last_event_at", "cancel_reason" }),
		"cancel_reasons" : ({ "unknown_node",
			"unknown_route", "role_denied", "no_route", "path_limit", "budget_exceeded",
			"node_disabled", "edge_disabled", "actor_denied", "actor_unavailable",
			"wrong_start", "insufficient_funds", "charge_failed", "adapter_failed",
			"arrival_failed", "execution_timeout", "route_removed", "route_disabled",
			"invalid_checkpoint", "legacy_cancelled", "unsupported_schema",
			"migration_failed" }) ]);
}

mapping query_node(string id)
{
	return mapp(nodes[id]) ? copy(nodes[id]) : 0;
}

mapping query_route(string id)
{
	return mapp(routes[id]) ? copy(routes[id]) : 0;
}

mapping query_graph_stats()
{
	return ([ "regions" : sizeof(keys(regions)), "nodes" : sizeof(keys(nodes)),
		"edges" : sizeof(keys(edges)), "routes" : sizeof(keys(routes)) ]);
}

mapping query_schedule(string actor_id)
{
	return mapp(schedules[actor_id]) ? copy(schedules[actor_id]) : 0;
}

private string current_period()
{
	int hour;

	hour = localtime(time())[2];
	if (hour >= 6 && hour < 10)
		return "morning";
	if (hour >= 10 && hour < 17)
		return "day";
	if (hour >= 17 && hour < 22)
		return "evening";
	return "night";
}

private string node_for_room(string room_path)
{
	string id;

	foreach (id in ({ "city_station", "wudang_gate", "wudang_foothill",
		"city_gate", "city_wharf", "hangzhou_wharf", "shaolin_station",
		"shaolin_gate" }))
		if (mapp(nodes[id]) && nodes[id]["room"] == room_path)
			return id;
	return 0;
}

int is_approved_remote(string actor_id, string room_path)
{
	mapping schedule;
	mapping period_config;
	string period;
	string node_id;

	if (! mapp(schedule = schedules[actor_id]) || ! schedule["enabled"])
		return 0;
	node_id = node_for_room(room_path);
	if (! stringp(node_id) || nodes[node_id]["region"] == "jiangnan")
		return 0;
	foreach (period in ({ "morning", "day", "evening", "night" }))
	{
		period_config = schedule["periods"][period];
		if (mapp(period_config) && period_config["node"] == node_id)
			return 1;
	}
	return 0;
}

int should_hold_for_schedule(string actor_id, string room_path)
{
	mapping schedule;
	mapping period_config;
	mapping config;
	string period;
	string current_node;

	if (! mapp(schedule = schedules[actor_id]) || ! schedule["enabled"])
		return 0;
	period = stringp(schedule_test_period[actor_id]) ?
		schedule_test_period[actor_id] : current_period();
	period_config = schedule["periods"][period];
	config = mapp(period_config) ? routes[period_config["route"]] : 0;
	current_node = node_for_room(room_path);
	return mapp(config) && current_node == config["from"] &&
		current_node != period_config["node"];
}

mapping query_adapter(string id)
{
	return mapp(adapters[id]) ? copy(adapters[id]) : 0;
}

string *validate_graph()
{
	string *issues;
	string id;
	string *allowed;
	mapping node;
	mapping edge;
	string role;
	string transport;

	issues = ({});
	foreach (id in keys(regions))
		if (! mapp(regions[id]) || regions[id]["id"] != id || ! regions[id]["enabled"])
			issues += ({ "region:" + id + ":invalid" });
	foreach (id in keys(nodes))
	{
		node = nodes[id];
		if (! mapp(node) || node["id"] != id || ! mapp(regions[node["region"]]))
			issues += ({ "node:" + id + ":invalid_region_or_id" });
		if (! node["enabled"] || ! node["safe"])
			issues += ({ "node:" + id + ":disabled_or_unsafe" });
	}
	foreach (id in keys(edges))
	{
		edge = edges[id];
		transport = edge["transport"];
		allowed = edge["allowed_roles"];
		if (! mapp(edge) || edge["id"] != id || ! mapp(nodes[edge["from"]]) ||
			! mapp(nodes[edge["to"]]))
			issues += ({ "edge:" + id + ":invalid_endpoint_or_id" });
		if (member_array(transport, ({ "walk", "carriage", "boat", "teleport" })) == -1)
			issues += ({ "edge:" + id + ":invalid_transport" });
		if (! intp(edge["cost"]) || edge["cost"] < 0 || ! intp(edge["risk"]) ||
			edge["risk"] < 0 || ! intp(edge["time"]) || edge["time"] <= 0)
			issues += ({ "edge:" + id + ":invalid_limits" });
		if (! arrayp(allowed) || ! sizeof(allowed))
			issues += ({ "edge:" + id + ":missing_allowed_roles" });
		else
			foreach (role in allowed)
				if (! stringp(role) || role == "")
					issues += ({ "edge:" + id + ":invalid_role" });
	}
	foreach (id in keys(routes))
	{
		edge = routes[id];
		if (! mapp(edge) || edge["id"] != id || ! mapp(nodes[edge["from"]]) ||
			! mapp(nodes[edge["to"]]) || ! stringp(edge["role"]) ||
			! edge["enabled"])
			issues += ({ "route:" + id + ":invalid" });
		if (! edge["test_only"] &&
			(edge["actor_id"] != AI_TRAVEL_ACTOR ||
			 ! mapp(nodes[edge["safe_return"]])))
			issues += ({ "route:" + id + ":invalid_actor_or_safe_return" });
	}
	foreach (id in keys(schedules))
	{
		edge = schedules[id];
		if (! mapp(edge) || edge["actor_id"] != id || ! edge["enabled"] ||
			! mapp(edge["periods"]))
		{
			issues += ({ "schedule:" + id + ":invalid" });
			continue;
		}
		foreach (role in ({ "morning", "day", "evening", "night" }))
			if (! mapp(edge["periods"][role]) ||
				! mapp(nodes[edge["periods"][role]["node"]]) ||
				! mapp(routes[edge["periods"][role]["route"]]) ||
				routes[edge["periods"][role]["route"]]["actor_id"] != id)
				issues += ({ "schedule:" + id + ":invalid_" + role });
	}
	return issues;
}

varargs mapping plan_route(string from, string to, string role, mapping budget)
{
	string *queue;
	mapping previous;
	mapping previous_edge;
	mapping visited;
	mapping depth;
	mapping totals;
	mapping result;
	mapping edge;
	string current;
	string next;
	string *nodes_path;
	string *edges_path;
	int head;
	int total_cost;
	int total_risk;
	int total_time;
	int transfers;
	int budget_blocked;
	int path_limited;
	int i;
	string last_transport;

	if (! stringp(role) || role == "")
		role = "traveler";
	if (! mapp(budget))
		budget = default_budget();
	if (! mapp(nodes[from]) || ! mapp(nodes[to]))
		return ([ "state" : "cancelled", "cancel_reason" : "unknown_node",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	if (! nodes[from]["enabled"] || ! nodes[to]["enabled"])
		return ([ "state" : "cancelled", "cancel_reason" : "node_disabled",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	if (member_array(role, ({ "traveler", "test" })) == -1)
		return ([ "state" : "cancelled", "cancel_reason" : "role_denied",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	if (from == to)
		return ([ "state" : "arrived", "cancel_reason" : "none",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION, "nodes" : ({ from }),
			"edges" : ({}), "total_cost" : 0, "total_risk" : 0,
			"total_time" : 0, "transfers" : 0, "retries" : 0 ]);
	queue = ({ from });
	previous = ([ from : 0 ]);
	previous_edge = ([]);
	visited = ([ from : 1 ]);
	depth = ([ from : 0 ]);
	totals = ([ from : ([ "cost" : 0, "risk" : 0, "time" : 0,
		"transfers" : 0, "transport" : "" ]) ]);
	head = 0;
	while (head < sizeof(queue) && head < AI_TRAVEL_MAX_NODES)
	{
		current = queue[head++];
		if (depth[current] >= AI_TRAVEL_MAX_DEPTH)
		{
			path_limited = 1;
			continue;
		}
		foreach (edge in outgoing(current, role))
		{
			next = edge["to"];
			if (visited[next])
				continue;
			last_transport = totals[current]["transport"];
			total_cost = totals[current]["cost"] + edge["cost"];
			total_risk = totals[current]["risk"] + edge["risk"];
			total_time = totals[current]["time"] + edge["time"];
			transfers = totals[current]["transfers"] +
				(last_transport != "" && last_transport != edge["transport"] ? 1 : 0);
			if (total_cost > budget["max_cost"] || total_risk > budget["max_risk"] ||
				total_time > budget["max_time"] || transfers > budget["max_transfers"])
			{
				budget_blocked = 1;
				continue;
			}
			visited[next] = 1;
			previous[next] = current;
			previous_edge[next] = edge["id"];
			depth[next] = depth[current] + 1;
			totals[next] = ([ "cost" : total_cost, "risk" : total_risk,
				"time" : total_time, "transfers" : transfers,
				"transport" : edge["transport"] ]);
			queue += ({ next });
			if (next == to)
				break;
		}
		if (visited[to])
			break;
	}
	if (! visited[to])
		return ([ "state" : "cancelled", "cancel_reason" :
			(sizeof(queue) >= AI_TRAVEL_MAX_NODES || path_limited ? "path_limit" :
			 (budget_blocked ? "budget_exceeded" : "no_route")),
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	nodes_path = ({ to });
	edges_path = ({});
	current = to;
	while (current != from)
	{
		edges_path = ({ previous_edge[current] }) + edges_path;
		current = previous[current];
		nodes_path = ({ current }) + nodes_path;
	}
	result = ([ "state" : "planned", "cancel_reason" : "none",
		"schema_version" : AI_TRAVEL_SCHEMA_VERSION, "from" : from, "to" : to,
		"role" : role, "nodes" : nodes_path, "edges" : edges_path,
		"total_cost" : totals[to]["cost"], "total_risk" : totals[to]["risk"],
		"total_time" : totals[to]["time"],
		"transfers" : totals[to]["transfers"], "retries" : 0 ]);
	return result;
}

mapping plan_registered_route(string route_id)
{
	mapping config;
	mapping result;

	config = routes[route_id];
	if (! mapp(config) || ! config["enabled"])
		return ([ "state" : "cancelled", "cancel_reason" : "unknown_route",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION, "route_id" : route_id ]);
	result = plan_route(config["from"], config["to"], config["role"]);
	result["route_id"] = route_id;
	return result;
}

mapping create_travel_state(string actor_id, mapping route)
{
	if (! stringp(actor_id) || actor_id == "" || ! mapp(route) ||
		route["state"] != "planned" || ! arrayp(route["edges"]))
		return ([ "state" : "cancelled", "cancel_reason" : "no_route",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	return route + ([ "actor_id" : actor_id,
		"route_id" : stringp(route["route_id"]) ? route["route_id"] :
			"adhoc:" + route["from"] + ":" + route["to"],
		"state" : "planned", "current_node" : route["from"], "edge_index" : 0,
		"charged_edges" : ({}), "charged_total" : 0, "refunded" : 0,
		"retries" : 0, "started_at" : time(), "updated_at" : time(),
		"trigger" : "manual", "event_seq" : 0, "last_event" : "none",
		"last_event_at" : 0, "cancel_reason" : "none", "seeded_total" : 0 ]);
}

private int carried_money_value(object me)
{
	object ob;
	string money_id;
	int amount;
	int unit;
	int total;

	if (! objectp(me))
		return 0;
	total = 0;
	foreach (ob in all_inventory(me))
	{
		if (! objectp(ob) || ! stringp(money_id = ob->query("money_id")))
			continue;
		amount = ob->query_amount();
		if (amount < 1)
			continue;
		unit = money_id == "cash" ? 100000 :
			(money_id == "gold" ? 10000 :
			(money_id == "silver" ? 100 : 1));
		total += amount * unit;
	}
	return total;
}

private int checkpoint_travel(object me, mapping state)
{
	int saved;
	string error;

	if (! objectp(me) || ! mapp(state))
		return 0;
	state["updated_at"] = time();
	me->set("ai/state/travel", state);
	error = catch(saved = me->save());
	return ! stringp(error) && saved;
}

private int move_to_node(object me, mapping edge)
{
	mapping from_node;
	mapping to_node;
	string current;

	if (! objectp(me) || ! mapp(edge) || ! edge["enabled"] || edge["prohibited"])
		return 0;
	from_node = nodes[edge["from"]];
	to_node = nodes[edge["to"]];
	if (! mapp(from_node) || ! mapp(to_node) || ! from_node["enabled"] ||
		! to_node["enabled"] || ! to_node["safe"] || ! objectp(environment(me)))
		return 0;
	current = base_name(environment(me));
	if (current != from_node["room"])
		return 0;
	if (catch(me->move(to_node["room"])) || ! objectp(environment(me)))
		return 0;
	return base_name(environment(me)) == to_node["room"];
}

private void publish_travel_event(object me, mapping state, string type,
	mapping edge)
{
	string transport;
	string text;

	if (! objectp(me) || ! objectp(environment(me)) || ! mapp(state))
		return;
	transport = mapp(edge) ? edge["transport"] : "walk";
	if (type == "departed")
		text = me->query("name") + "收拾行装，经由" + transport + "离开了这里。\n";
	else
		text = me->query("name") + "经由" + transport + "抵达了这里。\n";
	message("vision", text, environment(me), ({ me }));
	state["event_seq"] = state["event_seq"] + 1;
	state["last_event"] = type;
	state["last_event_at"] = time();
}

private mapping cancel_travel(object me, mapping state, mapping config,
	string reason)
{
	mapping safe_node;
	string safe_id;

	state["state"] = "cancelled";
	state["cancel_reason"] = reason;
	if (intp(state["charged_total"]) && state["charged_total"] > 0 &&
		! state["refunded"])
	{
		MONEY_D->pay_player(me, state["charged_total"]);
		state["refunded"] = 1;
	}
	if (intp(state["seeded_total"]) && state["seeded_total"] > 0)
	{
		MONEY_D->player_pay(me, state["seeded_total"]);
		state["seeded_total"] = 0;
	}
	safe_id = mapp(config) && stringp(config["safe_return"]) ?
		config["safe_return"] : state["safe_return"];
	safe_node = nodes[safe_id];
	if (objectp(me) && mapp(safe_node) && safe_node["enabled"] && safe_node["safe"])
	{
		catch(me->move(safe_node["room"]));
		if (objectp(environment(me)) && base_name(environment(me)) == safe_node["room"])
			state["current_node"] = safe_id;
	}
	checkpoint_travel(me, state);
	return copy(state);
}

mapping query_travel_status(string actor_id)
{
	object me;
	mapping state;

	me = AI_PLAYER_D->query_ai_player(actor_id);
	if (! objectp(me))
		return 0;
	state = me->query("ai/state/travel");
	return mapp(state) ? copy(state) : ([ "actor_id" : actor_id,
		"schema_version" : AI_TRAVEL_SCHEMA_VERSION, "state" : "idle",
		"cancel_reason" : "none", "current_node" : "none", "route_id" : "none",
		"edge_index" : 0, "charged_total" : 0, "refunded" : 0,
		"retries" : 0 ]);
}

private string validate_checkpoint(mapping state, mapping config)
{
	string edge_id;
	string node_id;
	mapping edge;
	mapping node;
	int index;

	if (! mapp(state) || ! mapp(config))
		return "invalid_checkpoint";
	if (state["test_route_disabled"])
		return "route_disabled";
	if (config["actor_id"] != state["actor_id"])
		return "actor_denied";
	if (! config["enabled"])
		return "route_disabled";
	if (! intp(state["edge_index"]) || state["edge_index"] < 0 ||
		! arrayp(state["edges"]) || state["edge_index"] > sizeof(state["edges"]))
		return "invalid_checkpoint";
	node_id = state["current_node"];
	node = nodes[node_id];
	if (! stringp(node_id) || ! mapp(node))
		return "unknown_node";
	if (! node["enabled"] || ! node["safe"])
		return "node_disabled";
	index = state["edge_index"];
	if (index < sizeof(state["edges"]))
	{
		edge_id = state["edges"][index];
		edge = edges[edge_id];
		if (! mapp(edge))
			return "invalid_checkpoint";
		if (! edge["enabled"] || edge["prohibited"])
			return "edge_disabled";
		if (edge["from"] != node_id &&
			! (state["state"] == "executing" && edge["to"] == node_id))
			return "invalid_checkpoint";
	}
	return "none";
}

private mapping migrate_travel_state(object me, mapping state)
{
	mapping config;
	mapping route;
	string reason;
	int old_schema;

	if (! mapp(state))
		return ([ "state" : "cancelled", "cancel_reason" : "invalid_checkpoint",
			"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
	old_schema = intp(state["schema_version"]) ? state["schema_version"] : 1;
	if (old_schema > AI_TRAVEL_SCHEMA_VERSION)
	{
		state["schema_version"] = AI_TRAVEL_SCHEMA_VERSION;
		state["cancel_reason"] = "unsupported_schema";
		return state;
	}
	if (old_schema == AI_TRAVEL_SCHEMA_VERSION)
		return state;
	config = routes[state["route_id"]];
	if (! mapp(config))
	{
		state["schema_version"] = AI_TRAVEL_SCHEMA_VERSION;
		state["cancel_reason"] = "legacy_cancelled";
		return state;
	}
	route = plan_registered_route(state["route_id"]);
	if (! arrayp(state["edges"]) || ! sizeof(state["edges"]))
		state["edges"] = route["edges"];
	if (! arrayp(state["nodes"]) || ! sizeof(state["nodes"]))
		state["nodes"] = route["nodes"];
	if (! stringp(state["actor_id"]))
		state["actor_id"] = config["actor_id"];
	if (! stringp(state["safe_return"]))
		state["safe_return"] = config["safe_return"];
	if (! stringp(state["current_node"]))
		state["current_node"] = config["from"];
	if (! intp(state["edge_index"]))
		state["edge_index"] = 0;
	if (! arrayp(state["charged_edges"]))
		state["charged_edges"] = ({});
	if (! intp(state["charged_total"]))
		state["charged_total"] = 0;
	if (! intp(state["refunded"]))
		state["refunded"] = 0;
	if (! intp(state["retries"]))
		state["retries"] = 0;
	if (! intp(state["started_at"]) || state["started_at"] < 1)
		state["started_at"] = time();
	if (! intp(state["updated_at"]))
		state["updated_at"] = time();
	if (! stringp(state["state"]) || member_array(state["state"],
		({ "planned", "charged", "executing", "arrived", "cancelled" })) == -1)
	{
		state["schema_version"] = AI_TRAVEL_SCHEMA_VERSION;
		state["cancel_reason"] = "legacy_cancelled";
		return state;
	}
	if (! stringp(state["trigger"]))
		state["trigger"] = "recovery_migration";
	if (! intp(state["event_seq"]))
		state["event_seq"] = 0;
	if (! stringp(state["last_event"]))
		state["last_event"] = "none";
	state["schema_version"] = AI_TRAVEL_SCHEMA_VERSION;
	state["migrated_from_schema"] = old_schema;
	state["recovery"] = "migrated";
	state["updated_at"] = time();
	reason = validate_checkpoint(state, config);
	if (reason != "none")
		state["cancel_reason"] = "migration_failed";
	else
		state["cancel_reason"] = "none";
	if (objectp(me))
		checkpoint_travel(me, state);
	return state;
}

private mapping resume_travel(object me, mapping config, mapping state,
	string fault_mode)
{
	mapping edge;
	mapping to_node;
	string edge_id;
	string reason;
	int cost;
	int before_money;
	int attempts;
	int moved;
	int index;

	if (! objectp(me) || ! mapp(config) || ! mapp(state))
		return ([ "state" : "cancelled", "cancel_reason" : "invalid_checkpoint" ]);
	if (state["state"] == "arrived" || state["state"] == "cancelled")
		return copy(state);
	reason = validate_checkpoint(state, config);
	if (reason != "none")
		return cancel_travel(me, state, config, reason);
	for (index = state["edge_index"]; index < sizeof(state["edges"]); index++)
	{
		if (time() - state["started_at"] > AI_TRAVEL_MAX_TIME)
			return cancel_travel(me, state, config, "execution_timeout");
		edge_id = state["edges"][index];
		edge = edges[edge_id];
		if (! mapp(edge) || ! edge["enabled"] || edge["prohibited"] ||
			! has_role(edge, config["role"]))
			return cancel_travel(me, state, config, "edge_disabled");
		cost = edge["cost"];
		if (member_array(edge_id, state["charged_edges"]) == -1)
		{
			before_money = carried_money_value(me);
			if (cost > 0 && (! MONEY_D->player_pay(me, cost) ||
				before_money - carried_money_value(me) != cost))
				return cancel_travel(me, state, config, "charge_failed");
			state["state"] = cost > 0 ? "charged" : "executing";
			state["charged_edges"] += ({ edge_id });
			state["charged_total"] += cost;
			if (! checkpoint_travel(me, state))
				return cancel_travel(me, state, config, "adapter_failed");
		}
		if (fault_mode == "edge_disabled")
			return cancel_travel(me, state, config, "edge_disabled");
		if (state["state"] == "executing" && state["current_node"] == edge["to"])
		{
			state["edge_index"] = index + 1;
			checkpoint_travel(me, state);
			continue;
		}
		if (state["current_node"] != edge["from"])
			return cancel_travel(me, state, config, "invalid_checkpoint");
		if (state["last_departed_edge"] != edge_id)
		{
			publish_travel_event(me, state, "departed", edge);
			state["last_departed_edge"] = edge_id;
			if (! checkpoint_travel(me, state))
				return cancel_travel(me, state, config, "adapter_failed");
		}
		moved = 0;
		for (attempts = 0; attempts <= AI_TRAVEL_MAX_RETRIES; attempts++)
		{
			if (move_to_node(me, edge))
			{
				moved = 1;
				break;
			}
			state["retries"]++;
		}
		if (! moved)
			return cancel_travel(me, state, config, "adapter_failed");
		state["state"] = "executing";
		state["edge_index"] = index + 1;
		state["current_node"] = edge["to"];
		publish_travel_event(me, state, "arrived", edge);
		map_delete(state, "last_departed_edge");
		if (! checkpoint_travel(me, state))
			return cancel_travel(me, state, config, "adapter_failed");
	}
	to_node = nodes[config["to"]];
	if (! objectp(environment(me)) || base_name(environment(me)) != to_node["room"])
		return cancel_travel(me, state, config, "arrival_failed");
	state["state"] = "arrived";
	state["cancel_reason"] = "none";
	state["arrived_at"] = time();
	state["seeded_total"] = 0;
	checkpoint_travel(me, state);
	return copy(state);
}

private mapping execute_registered_travel(string route_id, int seed_money,
	string fault_mode, string trigger)
{
	object me;
	mapping config;
	mapping route;
	mapping state;
	mapping result;
	int original_money;

	if (stringp(fault_mode) && fault_mode != "" &&
		member_array(fault_mode, ({ "insufficient", "edge_disabled" })) == -1)
		return ([ "state" : "cancelled", "cancel_reason" : "actor_denied" ]);
	config = routes[route_id];
	if (! mapp(config) || config["test_only"])
		return ([ "state" : "cancelled", "cancel_reason" : "unknown_route" ]);
	if (! config["enabled"])
		return ([ "state" : "cancelled", "cancel_reason" : "route_disabled" ]);
	if (config["actor_id"] != AI_TRAVEL_ACTOR)
		return ([ "state" : "cancelled", "cancel_reason" : "actor_denied" ]);
	me = AI_PLAYER_D->query_ai_player(config["actor_id"]);
	if (! objectp(me) || me->is_ghost() || me->is_fighting() || me->is_busy())
		return ([ "state" : "cancelled", "cancel_reason" : "actor_unavailable" ]);
	if (! objectp(environment(me)) || base_name(environment(me)) != nodes[config["from"]]["room"])
		return ([ "state" : "cancelled", "cancel_reason" : "wrong_start" ]);
	route = plan_registered_route(route_id);
	if (route["state"] != "planned")
		return route;
	state = create_travel_state(config["actor_id"], route);
	state["safe_return"] = config["safe_return"];
	state["trigger"] = stringp(trigger) ? trigger : "manual";
	if (strsrch(state["trigger"], "schedule_") == 0 &&
		me->query_temp("ai/travel_auto_test_seeded") > 0)
	{
		state["seeded_total"] = me->query_temp("ai/travel_auto_test_seeded");
		me->delete_temp("ai/travel_auto_test_seeded");
	}
	if (! checkpoint_travel(me, state))
		return cancel_travel(me, state, config, "adapter_failed");
	if (fault_mode == "insufficient")
	{
		original_money = carried_money_value(me);
		if (original_money > 0)
			MONEY_D->player_pay(me, original_money);
	}
	if (seed_money && fault_mode != "insufficient")
	{
		MONEY_D->pay_player(me, route["total_cost"]);
		state["seeded_total"] = route["total_cost"];
	}
	if (carried_money_value(me) < route["total_cost"])
	{
		result = cancel_travel(me, state, config, "insufficient_funds");
		if (original_money > 0)
		{
			MONEY_D->pay_player(me, original_money);
			me->save();
		}
		return result;
	}
	result = resume_travel(me, config, state, fault_mode);
	if (original_money > 0 && result["state"] == "cancelled")
	{
		MONEY_D->pay_player(me, original_money);
		me->save();
	}
	return result;
}

mapping recover_travel(string actor_id)
{
	object me;
	mapping state;
	mapping config;
	string reason;

	if (travel_recovery_guard[actor_id])
		return query_travel_status(actor_id);
	me = AI_PLAYER_D->query_ai_player(actor_id);
	if (! objectp(me))
		return ([ "actor_id" : actor_id, "state" : "cancelled",
			"cancel_reason" : "actor_unavailable" ]);
	state = me->query("ai/state/travel");
	if (! mapp(state))
		return query_travel_status(actor_id);
	if (state["state"] == "arrived" || state["state"] == "cancelled")
		return copy(state);
	travel_recovery_guard[actor_id] = 1;
	state = migrate_travel_state(me, state);
	config = routes[state["route_id"]];
	if (state["cancel_reason"] == "unsupported_schema" ||
		state["cancel_reason"] == "legacy_cancelled" ||
		state["cancel_reason"] == "migration_failed")
		state = cancel_travel(me, state, config, state["cancel_reason"]);
	else if (! mapp(config))
		state = cancel_travel(me, state, config, "route_removed");
	else if (state["test_route_disabled"] || ! config["enabled"])
		state = cancel_travel(me, state, config, "route_disabled");
	else
	{
		reason = validate_checkpoint(state, config);
		if (reason != "none")
			state = cancel_travel(me, state, config, reason);
		else
		{
			if (! objectp(environment(me)) ||
				base_name(environment(me)) != nodes[state["current_node"]]["room"])
				catch(me->move(nodes[state["current_node"]]["room"]));
			state = resume_travel(me, config, state, 0);
		}
	}
	map_delete(travel_recovery_guard, actor_id);
	return copy(state);
}

int prepare_recovery_test(string actor_id, string mode)
{
	object me;
	mapping config;
	mapping route;
	mapping state;
	mapping edge;
	string edge_id;
	int i;

	if (base_name(previous_object()) != "/cmds/adm/aiplayer" ||
		actor_id != AI_TRAVEL_ACTOR || member_array(mode, ({ "planned", "charged",
		"executing", "arrived", "legacy", "removed", "disabled", "invalid",
		"future", "legacy_invalid", "legacy_removed" })) == -1)
		return 0;
	config = routes["qingfeng_city_to_wudang"];
	me = AI_PLAYER_D->query_ai_player(actor_id);
	route = plan_registered_route("qingfeng_city_to_wudang");
	if (! mapp(config) || ! objectp(me) || route["state"] != "planned")
		return 0;
	catch(me->move(nodes[config["from"]]["room"]));
	state = create_travel_state(actor_id, route);
	state["safe_return"] = config["safe_return"];
	state["trigger"] = "recovery_prepare";
	me->set_temp("ai/travel_recovery_prepared", 1);
	MONEY_D->pay_player(me, route["total_cost"]);
	state["seeded_total"] = route["total_cost"];
	if (mode == "removed")
		state["route_id"] = "deleted_test_route";
	if (mode == "disabled")
		state["test_route_disabled"] = 1;
	if (mode == "invalid" || mode == "legacy_invalid")
		state["current_node"] = "deleted_test_node";
	if (mode == "legacy_removed")
		state["route_id"] = "deleted_test_route";
	if (mode == "future")
		state["schema_version"] = AI_TRAVEL_SCHEMA_VERSION + 1;
	if (mode == "charged" || mode == "executing" || mode == "arrived")
	{
		for (i = 0; i < sizeof(route["edges"]); i++)
		{
			edge_id = route["edges"][i];
			edge = edges[edge_id];
			if (edge["cost"] > 0)
				MONEY_D->player_pay(me, edge["cost"]);
			state["charged_edges"] += ({ edge_id });
			state["charged_total"] += edge["cost"];
			if (mode == "charged")
				break;
			if (mode == "executing")
			{
				catch(me->move(nodes[edge["to"]]["room"]));
				state["current_node"] = edge["to"];
				state["edge_index"] = i + 1;
				break;
			}
		}
		if (mode == "arrived")
		{
			state["current_node"] = config["to"];
			state["edge_index"] = sizeof(route["edges"]);
			catch(me->move(nodes[config["to"]]["room"]));
			state["state"] = "arrived";
			state["event_seq"] = 4;
			state["last_event"] = "arrived";
		}
		else
			state["state"] = mode;
	}
	if (mode == "legacy" || mode == "legacy_invalid" || mode == "legacy_removed")
	{
		state["schema_version"] = 1;
		map_delete(state, "trigger");
		map_delete(state, "event_seq");
		map_delete(state, "last_event");
	}
	return checkpoint_travel(me, state);
}

varargs mapping run_registered_travel(string route_id, int seed_money,
	string fault_mode)
{
	if (base_name(previous_object()) != "/cmds/adm/aiplayer")
		return ([ "state" : "cancelled", "cancel_reason" : "actor_denied" ]);
	return execute_registered_travel(route_id, seed_money, fault_mode, "admin");
}

varargs mapping run_schedule_period(string actor_id, string period,
	int seed_money)
{
	mapping schedule;
	mapping period_config;

	if (base_name(previous_object()) != "/cmds/adm/aiplayer")
		return ([ "state" : "cancelled", "cancel_reason" : "actor_denied" ]);
	schedule = schedules[actor_id];
	period_config = mapp(schedule) ? schedule["periods"][period] : 0;
	if (! mapp(schedule) || ! schedule["enabled"] || ! mapp(period_config))
		return ([ "state" : "cancelled", "cancel_reason" : "unknown_route" ]);
	return execute_registered_travel(period_config["route"], seed_money, 0,
		"schedule_test_" + period);
}

int prepare_auto_schedule_test(string actor_id, string period)
{
	object me;
	mapping schedule;
	mapping period_config;
	mapping config;
	mapping route;
	int money;

	if (base_name(previous_object()) != "/cmds/adm/aiplayer" ||
		actor_id != AI_TRAVEL_ACTOR)
		return 0;
	schedule = schedules[actor_id];
	period_config = mapp(schedule) ? schedule["periods"][period] : 0;
	config = mapp(period_config) ? routes[period_config["route"]] : 0;
	me = AI_PLAYER_D->query_ai_player(actor_id);
	if (! mapp(config) || ! objectp(me) || ! objectp(environment(me)) ||
		base_name(environment(me)) != nodes[config["from"]]["room"])
		return 0;
	route = plan_registered_route(config["id"]);
	if (route["state"] != "planned")
		return 0;
	MONEY_D->pay_player(me, route["total_cost"]);
	me->set_temp("ai/travel_auto_test_seeded", route["total_cost"]);
	schedule_test_period[actor_id] = period;
	next_schedule_attempt[actor_id] = time();
	return 1;
}

protected void heart_beat()
{
	object me;
	mapping schedule;
	mapping period_config;
	string actor_id;
	string period;
	string actor_period;
	string current_node;
	mapping result;

	period = current_period();
	foreach (actor_id in keys(schedules))
	{
		me = AI_PLAYER_D->query_ai_player(actor_id);
		if (objectp(me) && ! me->query_temp("ai/travel_recovery_prepared") &&
			mapp(me->query("ai/state/travel")) &&
			member_array(me->query("ai/state/travel")["state"],
			({ "planned", "charged", "executing" })) != -1)
		{
			recover_travel(actor_id);
			next_schedule_attempt[actor_id] = time() + AI_TRAVEL_SCHEDULE_RETRY_DELAY;
		}
	}
	if (AI_PLAYER_D->is_paused())
		return;
	foreach (actor_id in keys(schedules))
	{
		schedule = schedules[actor_id];
		if (! mapp(schedule) || ! schedule["enabled"] ||
			next_schedule_attempt[actor_id] > time())
			continue;
		actor_period = stringp(schedule_test_period[actor_id]) ?
			schedule_test_period[actor_id] : period;
		period_config = schedule["periods"][actor_period];
		me = AI_PLAYER_D->query_ai_player(actor_id);
		if (! mapp(period_config) || ! objectp(me) || ! objectp(environment(me)))
			continue;
		current_node = node_for_room(base_name(environment(me)));
		if (current_node == period_config["node"])
		{
			next_schedule_attempt[actor_id] = time() + AI_TRAVEL_SCHEDULE_RETRY_DELAY;
			continue;
		}
		if (mapp(routes[period_config["route"]]) &&
			current_node == routes[period_config["route"]]["from"])
		{
			result = execute_registered_travel(period_config["route"], 0, 0,
				"schedule_" + actor_period);
			map_delete(schedule_test_period, actor_id);
			next_schedule_attempt[actor_id] = time() +
				(result["state"] == "arrived" ? AI_TRAVEL_SCHEDULE_RETRY_DELAY : 120);
		}
	}
}

mapping execute_adapter(string adapter_id, mapping request)
{
	if (! mapp(adapters[adapter_id]) || adapter_id != "test" || ! mapp(request) ||
		! mapp(edges[request["edge_id"]]))
		return ([ "state" : "cancelled", "cancel_reason" : "adapter_failed" ]);
	return ([ "state" : "executed", "adapter" : adapter_id,
		"side_effects" : 0, "edge_id" : request["edge_id"],
		"schema_version" : AI_TRAVEL_SCHEMA_VERSION ]);
}

mapping selftest()
{
	mapping result;
	mapping route;
	mapping state;
	mapping adapter;
	mapping denied;
	mapping over_budget;
	mapping checks;
	string *issues;
	string key;

	issues = validate_graph();
	route = plan_registered_route("test_city_to_wudang");
	state = create_travel_state("ai_qingfeng", route);
	adapter = execute_adapter("test", ([ "edge_id" : "city_gate_to_station" ]));
	denied = plan_route("city_gate", "wudang_gate", "unknown_role");
	over_budget = plan_route("city_gate", "wudang_gate", "test",
		([ "max_cost" : 100, "max_risk" : 3, "max_transfers" : 4,
			"max_time" : 1800, "max_retries" : 2 ]));
	checks = ([
		"graph" : ! sizeof(issues),
		"single_route" : route["state"] == "planned" && sizeof(route["edges"]) == 3,
		"bounded_budget" : route["total_cost"] == 120 &&
			route["total_risk"] == 1 && route["total_time"] == 960 &&
			route["transfers"] == 2,
		"checkpoint" : state["actor_id"] == "ai_qingfeng" &&
			state["route_id"] == "test_city_to_wudang" &&
			state["edge_index"] == 0 && arrayp(state["charged_edges"]),
		"role_denied" : denied["cancel_reason"] == "role_denied",
		"budget_cancel" : over_budget["cancel_reason"] == "budget_exceeded",
		"test_adapter" : adapter["state"] == "executed",
	]);
	foreach (key in keys(checks))
		if (! checks[key])
			issues += ({ key + ":failed" });
	result = ([ "capability_version" : AI_TRAVEL_CAPABILITY_VERSION,
		"schema_version" : AI_TRAVEL_SCHEMA_VERSION, "checks" : checks,
		"issues" : issues, "passed" : ! sizeof(issues) ]);
	return result;
}
