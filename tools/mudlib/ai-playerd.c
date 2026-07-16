// Persistent autonomous player actors for the Android standalone world.

#include <ansi.h>
#include <command.h>
#include <globals.h>

#define AI_MIN_ACTION_DELAY 8
#define AI_ACTION_JITTER 10
#define AI_SAVE_INTERVAL 300
#define AI_SURVIVAL_INTERVAL 10
#define AI_MAX_PATH_NODES 160
#define AI_MAX_PATH_DEPTH 18
#define AI_MAX_RECENT_ROOMS 6
#define AI_MAX_RELATIONS 12
#define AI_MAX_EVENTS 16
#define AI_ROUTE_CACHE_TTL 300
#define AI_RELATION_DAILY_GAIN 3
#define AI_ACTIVITY_START_DELAY 20
#define AI_TEST_ROOM "/d/standalone/ai_test"
#define AI_TEST_DUMMY "/clone/npc/ai_test_dummy"

#define ADAPTER_SUCCESS 1
#define ADAPTER_PRECONDITION_FAILED -1
#define ADAPTER_COMMAND_FAILED -2
#define ADAPTER_POSTCONDITION_FAILED -3

mapping profiles = ([
	"ai_qingfeng" : ([
		"name" : "沈清风",
		"gender" : "男性",
		"title" : "江湖游侠",
		"home" : "/d/city/guangchang",
		"zone" : "/d/city/",
		"level" : 55,
		"exp" : 180000,
		"route" : ({
			"/d/city/guangchang",
			"/d/city/dongmen",
			"/d/city/kedian",
			"/d/city/xiaohuayuan",
		}),
		"route_offset" : 0,
		"dwell" : 55,
		"talk_chance" : 10,
		"courage" : 55,
		"schedule" : ([
			"morning" : ({ "/d/city/guangchang", "/d/city/xiaohuayuan" }),
			"day" : ({ "/d/city/dongmen", "/d/city/guangchang" }),
			"evening" : ({ "/d/city/kedian", "/d/city/guangchang" }),
			"night" : ({ "/d/city/guangchang" }),
		]),
		"period_lines" : ([
			"morning" : "一日之计在于晨，趁早活动活动筋骨。",
			"day" : "白日里行人多，城里城外都要多留意。",
			"evening" : "天色不早了，也该找个地方歇脚。",
			"night" : "夜深露重，还是少在外面走动为好。",
		]),
		"greeting_lines" : ([
			"morning" : "这么早便出来走动了。",
			"day" : "城里人多，行路时多留意脚下。",
			"evening" : "天色渐晚，办完事便早些歇息吧。",
			"night" : "夜路难行，莫在外面耽搁太久。",
		]),
		"room_lines" : ([
			"/d/city/dongmen" : "东门来往的人不少，城外的动静也值得留意。",
			"/d/city/xiaohuayuan" : "小花园清静，正适合停下来理一理思绪。",
		]),
		"activities" : ({
			([ "id" : "city_watch", "name" : "巡视城中",
			   "target" : "/d/city/dongmen", "action" : "patrol",
			   "stops" : ({ "/d/city/dongmen", "/d/city/guangchang",
				"/d/city/xiaohuayuan" }),
			   "cooldown" : 210, "periods" : ({ "morning", "day" }) ]),
			([ "id" : "check_supplies", "name" : "检查行囊",
			   "target" : "/d/city/kedian", "action" : "supplies",
			   "cooldown" : 300, "periods" : ({ "evening" }) ]),
			([ "id" : "inn_rest", "name" : "客栈歇脚",
			   "target" : "/d/city/kedian", "action" : "rest",
			   "line" : "赶了半日路，且在这里坐一会儿。",
			   "duration" : 24, "cooldown" : 360,
			   "periods" : ({ "evening" }) ]),
			([ "id" : "meet_wantang", "name" : "与苏晚棠碰面",
			   "target" : "/d/city/xiaohuayuan", "action" : "social",
			   "partner" : "ai_wantang",
			   "line" : "苏姑娘，今日城中可有什么新消息？",
			   "cooldown" : 480, "periods" : ({ "day", "evening" }) ]),
		}),
		"lines" : ({
			"听说最近城外不太平，出门还是小心些好。",
			"练武最忌心浮气躁，根基还是要一步一步打牢。",
			"扬州人来人往，倒是个打听江湖消息的好地方。",
		}),
	]),
	"ai_wantang" : ([
		"name" : "苏晚棠",
		"gender" : "女性",
		"title" : "云游剑客",
		"home" : "/d/city/kedian",
		"zone" : "/d/city/",
		"level" : 48,
		"exp" : 130000,
		"route" : ({
			"/d/city/kedian",
			"/d/city/guangchang",
			"/d/city/yaopu_neishi",
			"/d/city/dongmen",
		}),
		"route_offset" : 1,
		"dwell" : 75,
		"talk_chance" : 8,
		"courage" : 48,
		"schedule" : ([
			"morning" : ({ "/d/city/kedian", "/d/city/yaopu_neishi" }),
			"day" : ({ "/d/city/guangchang", "/d/city/dongmen" }),
			"evening" : ({ "/d/city/kedian", "/d/city/guangchang" }),
			"night" : ({ "/d/city/kedian" }),
		]),
		"period_lines" : ([
			"morning" : "清早的客栈还算安静，正好整理一下行装。",
			"day" : "白天消息传得快，真假却更难分辨。",
			"evening" : "赶了一天路，晚间总算能坐下来喝口茶。",
			"night" : "夜里客栈人杂，随身东西还是看紧些好。",
		]),
		"greeting_lines" : ([
			"morning" : "清早人少，正好慢慢整理行程。",
			"day" : "白日消息多，听来却要多想一层。",
			"evening" : "赶了一天路，也该坐下喝口茶了。",
			"night" : "夜里客栈人杂，随身东西要看紧些。",
		]),
		"room_lines" : ([
			"/d/city/zuixianlou" : "醉仙楼客来客往，既能补充干粮，也能听见不少消息。",
			"/d/city/yaopu_neishi" : "出门在外，常用药物还是备齐些稳妥。",
		]),
		"activities" : ({
			([ "id" : "gather_news", "name" : "打听消息",
			   "target" : "/d/city/guangchang", "action" : "patrol",
			   "stops" : ({ "/d/city/guangchang", "/d/city/dongmen" }),
			   "cooldown" : 240, "periods" : ({ "day" }) ]),
			([ "id" : "pack_supplies", "name" : "整理补给",
			   "target" : "/d/city/zuixianlou", "action" : "supplies",
			   "vendor" : "xiao er", "food_id" : "baozi",
			   "food_cost" : 50, "water_id" : "jiudai",
			   "water_cost" : 1500,
			   "cooldown" : 300,
			   "periods" : ({ "morning", "evening" }) ]),
			([ "id" : "inn_rest", "name" : "客栈休整",
			   "target" : "/d/city/kedian", "action" : "rest",
			   "line" : "行囊已经理好，坐下来歇一会儿再走。",
			   "duration" : 28, "cooldown" : 360,
			   "periods" : ({ "evening" }) ]),
			([ "id" : "meet_qingfeng", "name" : "与沈清风交换消息",
			   "target" : "/d/city/xiaohuayuan", "action" : "social",
			   "partner" : "ai_qingfeng",
			   "line" : "沈兄，我正好听来两桩城里的消息。",
			   "cooldown" : 480, "periods" : ({ "day", "evening" }) ]),
		}),
		"lines" : ({
			"客栈里消息虽多，真假却要自己分辨。",
			"今日走得有些累了，正好在这里歇歇脚。",
			"一柄好剑未必难得，难得的是用剑的人沉得住气。",
		}),
	]),
	"ai_yanqiu" : ([
		"name" : "赵砚秋",
		"gender" : "男性",
		"title" : "武当俗家弟子",
		"home" : "/d/wudang/jiejianyan",
		"zone" : "/d/wudang/",
		"level" : 62,
		"exp" : 260000,
		"route" : ({
			"/d/wudang/jiejianyan",
			"/d/wudang/slxl3",
			"/d/wudang/guangchang",
			"/d/wudang/zixiaogate",
		}),
		"route_offset" : 2,
		"dwell" : 85,
		"talk_chance" : 6,
		"courage" : 72,
		"schedule" : ([
			"morning" : ({ "/d/wudang/jiejianyan", "/d/wudang/slxl3" }),
			"day" : ({ "/d/wudang/guangchang", "/d/wudang/zixiaogate" }),
			"evening" : ({ "/d/wudang/slxl3", "/d/wudang/jiejianyan" }),
			"night" : ({ "/d/wudang/jiejianyan" }),
		]),
		"period_lines" : ([
			"morning" : "晨间山气清冽，正适合吐纳行气。",
			"day" : "日间来往山路的人不少，礼数不可废。",
			"evening" : "暮色入山，招式也该收一收了。",
			"night" : "山中夜静，正好回想今日所学。",
		]),
		"greeting_lines" : ([
			"morning" : "晨间山气清冽，行路正要稳住呼吸。",
			"day" : "山路石阶多，登山时莫要着急。",
			"evening" : "暮色入山，若要下山最好趁早。",
			"night" : "山中夜静，行路时请放轻脚步。",
		]),
		"room_lines" : ([
			"/d/wudang/zixiaogate" : "紫霄宫前来客不少，礼数和戒备都不可少。",
			"/d/wudang/slxl3" : "石梁险窄，练步法时尤其要沉住气。",
		]),
		"activities" : ({
			([ "id" : "morning_training", "name" : "晨间练功",
			   "target" : "/d/wudang/slxl3", "action" : "reflect",
			   "line" : "一招一式都要从根基练起，半点取巧不得。",
			   "cooldown" : 240, "periods" : ({ "morning" }) ]),
			([ "id" : "mountain_watch", "name" : "巡视山路",
			   "target" : "/d/wudang/zixiaogate", "action" : "patrol",
			   "stops" : ({ "/d/wudang/zixiaogate", "/d/wudang/guangchang" }),
			   "cooldown" : 210, "periods" : ({ "day", "evening" }) ]),
			([ "id" : "quiet_rest", "name" : "静坐调息",
			   "target" : "/d/wudang/jiejianyan", "action" : "rest",
			   "line" : "行功不可一味求快，先静坐片刻再说。",
			   "duration" : 26, "cooldown" : 360,
			   "periods" : ({ "evening" }) ]),
		}),
		"lines" : ({
			"山中清静，正适合静下心来琢磨招式。",
			"师门武学讲究以静制动，急不得。",
			"下山许久，也该找机会回去向师长请安了。",
		}),
	]),
	"ai_zhiyuan" : ([
		"name" : "林知远",
		"gender" : "男性",
		"title" : "少林俗家弟子",
		"home" : "/d/shaolin/shanmen",
		"zone" : "/d/shaolin/",
		"level" : 66,
		"exp" : 310000,
		"route" : ({
			"/d/shaolin/shanmen",
			"/d/shaolin/shijie8",
			"/d/shaolin/guangchang1",
			"/d/shaolin/shijie10",
		}),
		"route_offset" : 3,
		"dwell" : 95,
		"talk_chance" : 5,
		"courage" : 78,
		"schedule" : ([
			"morning" : ({ "/d/shaolin/shanmen", "/d/shaolin/shijie8" }),
			"day" : ({ "/d/shaolin/guangchang1", "/d/shaolin/shijie10" }),
			"evening" : ({ "/d/shaolin/shijie8", "/d/shaolin/shanmen" }),
			"night" : ({ "/d/shaolin/shanmen" }),
		]),
		"period_lines" : ([
			"morning" : "晨钟过后，山门前也渐渐有了人声。",
			"day" : "白日功课繁多，更要守住一颗平常心。",
			"evening" : "暮鼓将近，今日的功课也该收尾了。",
			"night" : "夜深寺静，少说几句也是修行。",
		]),
		"greeting_lines" : ([
			"morning" : "晨钟方歇，入寺还请放轻脚步。",
			"day" : "白日香客不少，请沿石阶缓行。",
			"evening" : "暮鼓将近，若有事最好早些办完。",
			"night" : "夜深寺静，还请莫要高声喧哗。",
		]),
		"room_lines" : ([
			"/d/shaolin/shanmen" : "山门既要守礼，也不能疏忽来往动静。",
			"/d/shaolin/shijie8" : "石阶虽长，一步一步走稳便是功课。",
		]),
		"activities" : ({
			([ "id" : "guard_gate", "name" : "照看山门",
			   "target" : "/d/shaolin/shanmen", "action" : "patrol",
			   "stops" : ({ "/d/shaolin/shanmen", "/d/shaolin/shijie8",
				"/d/shaolin/guangchang1" }),
			   "cooldown" : 210, "periods" : ({ "morning", "day" }) ]),
			([ "id" : "evening_practice", "name" : "晚间功课",
			   "target" : "/d/shaolin/shijie8", "action" : "reflect",
			   "line" : "今日功课尚有不足，还需再静心琢磨。",
			   "cooldown" : 270, "periods" : ({ "evening" }) ]),
			([ "id" : "gate_rest", "name" : "山门静息",
			   "target" : "/d/shaolin/shanmen", "action" : "rest",
			   "line" : "守山也要张弛有度，且静息片刻。",
			   "duration" : 24, "cooldown" : 360,
			   "periods" : ({ "evening" }) ]),
		}),
		"lines" : ({
			"少林山门清净，来往香客却从来不少。",
			"拳脚功夫练到最后，练的还是一口气。",
			"江湖争斗无休无止，能忍一步也是修行。",
		}),
	]),
	"ai_songlan" : ([
		"name" : "陈松岚",
		"gender" : "女性",
		"title" : "西湖侠女",
		"home" : "/d/hangzhou/road10",
		"zone" : "/d/hangzhou/",
		"level" : 58,
		"exp" : 210000,
		"route" : ({
			"/d/hangzhou/road10",
			"/d/hangzhou/kedian",
			"/d/hangzhou/baoshishan",
			"/d/hangzhou/lingyinsi",
		}),
		"route_offset" : 1,
		"dwell" : 70,
		"talk_chance" : 9,
		"courage" : 62,
		"schedule" : ([
			"morning" : ({ "/d/hangzhou/road10", "/d/hangzhou/lingyinsi" }),
			"day" : ({ "/d/hangzhou/baoshishan", "/d/hangzhou/road10" }),
			"evening" : ({ "/d/hangzhou/kedian", "/d/hangzhou/road10" }),
			"night" : ({ "/d/hangzhou/road10" }),
		]),
		"period_lines" : ([
			"morning" : "清晨湖面雾气未散，远山倒显得更近了。",
			"day" : "白日游人如织，湖边总有看不完的新鲜事。",
			"evening" : "夕照落在湖上，这时候的景色最好。",
			"night" : "夜里湖风渐凉，还是早些回去吧。",
		]),
		"greeting_lines" : ([
			"morning" : "清晨湖上雾重，沿岸走时要当心。",
			"day" : "白日游人多，倒总能遇见些新鲜事。",
			"evening" : "夕照正好，不妨慢些赶路。",
			"night" : "夜里湖风凉，还是早些回去吧。",
		]),
		"room_lines" : ([
			"/d/hangzhou/baoshishan" : "宝石山上望湖最清楚，只是山路也最费脚力。",
			"/d/hangzhou/lingyinsi" : "灵隐寺一带清幽，连心绪也容易静下来。",
		]),
		"activities" : ({
			([ "id" : "lake_walk", "name" : "沿湖巡游",
			   "target" : "/d/hangzhou/baoshishan", "action" : "patrol",
			   "stops" : ({ "/d/hangzhou/baoshishan", "/d/hangzhou/road10",
				"/d/hangzhou/lingyinsi" }),
			   "cooldown" : 210, "periods" : ({ "morning", "day" }) ]),
			([ "id" : "travel_supplies", "name" : "补充旅粮",
			   "target" : "/d/hangzhou/kedian", "action" : "supplies",
			   "cooldown" : 300, "periods" : ({ "evening" }) ]),
			([ "id" : "lakeside_rest", "name" : "湖畔歇脚",
			   "target" : "/d/hangzhou/road10", "action" : "rest",
			   "line" : "湖风正好，就在这里歇一歇脚。",
			   "duration" : 26, "cooldown" : 360,
			   "periods" : ({ "evening" }) ]),
		}),
		"lines" : ({
			"西湖景色虽好，最近却总觉得有人在暗中窥探。",
			"行走江湖，银两和干粮一样都不能少。",
			"有空看看风景，比整日打打杀杀有意思多了。",
		}),
	]),
]);

mapping actors = ([]);
mapping tracing = ([]);
mapping route_cache = ([]);
mapping metrics = ([]);
mapping event_queues = ([]);
mapping scenarios = ([]);
mapping next_load_attempt = ([]);
int paused;

private void load_all_players();
private object load_player(string id, mapping profile);
private void initialize_player(object me, string id, mapping profile);
private void initialize_login(object login, string id, mapping profile);
private void initialize_runtime_state(object me, mapping profile);
private void think(object me, mapping profile);
private void trace_event(object me, string event);
private void move_locally(object me, mapping profile);
private int move_toward(object me, mapping profile, string target);
private void respawn_player(object me, mapping profile);
private void say_profile_line(object me, mapping profile);
private void say_context_line(object me, mapping profile);
private void process_player(string id);
private int run_action(object me, string verb, string arg);
private string current_period();
private string find_route_step(object me, string start, string target, string zone);
private void perceive_world(object me, mapping profile);
private int handle_next_event(object me, mapping profile);
private int run_activity(object me, mapping profile);
private void interrupt_activity(object me, string reason);
private void finish_activity(object me, mapping activity, string outcome);
private void recover_route_failure(object me, mapping profile);
private void add_metric(string id, string key, int amount);
private int inventory_count(object me, string item_id);
private int carried_money_value(object me);
private object find_usable_supply(object me, string item_id, string need);
private int purchase_supply(object me, mapping activity, string need);
private int consume_supply(object me, mapping activity, string need);
private int rest_safely(object me, mapping activity);
private int run_patrol_activity(object me, mapping profile, mapping activity);
private int run_social_activity(object me, mapping profile, mapping activity);
private string contextual_greeting(object me, mapping profile,
	string player_name, string tier);
private void mark_scenario_event(string id, string type);
private void clear_scenario_events(string id);
private void enqueue_event(object me, string type, object source,
	string source_id, string source_name, string detail, int priority);
object *query_ai_players();

void create()
{
	seteuid(ROOT_UID);
	set_heart_beat(2);
	call_out("load_all_players", 3);
}

int clean_up() { return 1; }

int is_paused() { return paused; }

void set_paused(int value)
{
	object me;

	paused = value ? 1 : 0;
	if (! paused)
	{
		foreach (me in query_ai_players())
			if (objectp(me))
				me->set_temp("ai/next_action",
					time() + 2 + random(AI_ACTION_JITTER));
	}
}

mapping query_profiles()
{
	return copy(profiles);
}

private mapping metric_bucket(string id)
{
	mapping bucket;

	if (! mapp(metrics[id]))
	{
		bucket = ([
			"started_at" : time(),
			"actions" : 0,
			"action_failures" : 0,
			"path_searches" : 0,
			"path_nodes" : 0,
			"cache_hits" : 0,
			"cache_misses" : 0,
			"events_enqueued" : 0,
			"events_dropped" : 0,
			"events_handled" : 0,
			"errors" : 0,
			"respawns" : 0,
			"activities_started" : 0,
			"activities_completed" : 0,
			"activities_interrupted" : 0,
			"activities_resumed" : 0,
			"route_failures" : 0,
			"relocations" : 0,
			"adapter_attempts" : 0,
			"adapter_successes" : 0,
			"adapter_precondition_failures" : 0,
			"adapter_command_failures" : 0,
			"adapter_postcondition_failures" : 0,
			"scenarios_started" : 0,
			"scenarios_passed" : 0,
			"scenarios_failed" : 0,
			"rests_completed" : 0,
			"patrol_stops" : 0,
			"social_meetings" : 0,
		]);
		metrics[id] = bucket;
	}
	return metrics[id];
}

private void add_metric(string id, string key, int amount)
{
	mapping bucket;

	if (! stringp(id) || ! mapp(profiles[id]))
		return;
	bucket = metric_bucket(id);
	bucket[key] += amount;
	metrics[id] = bucket;
}

mapping query_metrics(string id)
{
	if (! mapp(profiles[id]))
		return 0;
	return copy(metric_bucket(id));
}

mixed *query_event_queue(string id)
{
	mixed *queue;

	if (! mapp(profiles[id]))
		return 0;
	queue = event_queues[id];
	return arrayp(queue) ? queue + ({}) : ({});
}

object *query_ai_players()
{
	object *result;
	string id;

	result = ({});
	foreach (id in keys(profiles))
	{
		if (objectp(actors[id]))
			result += ({ actors[id] });
	}
	return result;
}

mapping query_player_status(string id)
{
	object me;
	object room;
	mapping status;
	string *recent;
	mapping relations;
	string goal;
	string intent;
	string last_action;
	mapping activity;

	if (! mapp(profiles[id]) || ! objectp(me = actors[id]))
		return 0;

	room = environment(me);
	recent = me->query("ai/state/recent_rooms");
	relations = me->query("ai/state/relations");
	goal = me->query("ai/state/goal");
	intent = me->query("ai/state/intent");
	last_action = me->query("ai/state/last_action");
	activity = me->query("ai/state/activity");
	status = ([
		"id" : id,
		"name" : me->query("name"),
		"room" : objectp(room) ? room->query("short") : "无位置",
		"room_path" : objectp(room) ? base_name(room) : "",
		"goal" : stringp(goal) ? goal : "初始化",
		"goal_since" : me->query("ai/state/goal_since"),
		"intent" : stringp(intent) ? intent : "无",
		"last_action" : stringp(last_action) ? last_action : "无",
		"last_action_at" : me->query("ai/state/last_action_at"),
		"next_action" : me->query_temp("ai/next_action"),
		"food" : me->query("food"),
		"food_max" : me->max_food_capacity(),
		"water" : me->query("water"),
		"water_max" : me->max_water_capacity(),
		"money" : carried_money_value(me),
		"qi" : me->query("qi"),
		"qi_max" : me->query("max_qi") + me->query_gain_qi(),
		"jing" : me->query("jing"),
		"jing_max" : me->query("max_jing"),
		"recent_rooms" : arrayp(recent) ? recent : ({}),
		"relations" : mapp(relations) ? sizeof(relations) : 0,
		"fighting" : me->is_fighting() ? 1 : 0,
		"busy" : me->is_busy() ? 1 : 0,
		"ghost" : me->is_ghost() ? 1 : 0,
		"tracing" : tracing[id] ? 1 : 0,
		"period" : current_period(),
		"pending_events" : arrayp(event_queues[id]) ?
			sizeof(event_queues[id]) : 0,
		"activity" : mapp(activity) ? activity["name"] : "无",
		"activity_id" : mapp(activity) ? activity["id"] : "",
		"activity_step" : mapp(activity) ? activity["step"] : "",
		"activity_target" : mapp(activity) ? activity["target"] : "",
		"activity_interrupted" : mapp(activity) &&
			activity["interrupted"] ? 1 : 0,
		"last_activity" : me->query("ai/state/last_activity"),
	]);
	return status;
}

int set_trace(string id, int enabled)
{
	if (! mapp(profiles[id]))
		return 0;
	if (enabled)
		tracing[id] = 1;
	else
		map_delete(tracing, id);
	return 1;
}

int move_player_home(string id)
{
	object me;

	if (! mapp(profiles[id]) || ! objectp(me = actors[id]))
		return 0;
	me->remove_all_killer();
	if (catch(me->move(profiles[id]["home"])) || ! environment(me))
		return 0;
	me->delete_temp("ai/dwell_until");
	me->set("ai/state/intent", "返回常驻点");
	me->set("ai/state/last_action", "home");
	me->set("ai/state/last_action_at", time());
	me->save();
	return 1;
}

int reset_player(string id)
{
	object me;
	mapping profile;

	if (! mapp(profile = profiles[id]) || ! objectp(me = actors[id]))
		return 0;
	me->delete("ai/state");
	me->delete_temp("ai");
	me->remove_all_killer();
	if (me->is_ghost())
		respawn_player(me, profile);
	me->set("food", me->max_food_capacity());
	me->set("water", me->max_water_capacity());
	me->set("eff_qi", me->query("max_qi") + me->query_gain_qi());
	me->set("qi", me->query("eff_qi"));
	me->set("eff_jing", me->query("max_jing"));
	me->set("jing", me->query("max_jing"));
	me->set("neili", me->query("max_neili"));
	initialize_runtime_state(me, profile);
	if (catch(me->move(profile["home"])) || ! environment(me))
		return 0;
	me->save();
	return 1;
}

mapping query_scenario_status(string id)
{
	mapping scenario;
	mapping result;

	if (! mapp(profiles[id]) || ! mapp(scenario = scenarios[id]))
		return 0;
	result = ([
		"id" : id,
		"type" : scenario["type"],
		"status" : scenario["status"],
		"started_at" : scenario["started_at"],
		"finished_at" : scenario["finished_at"],
		"original_room" : scenario["original_room"],
		"start_event" : scenario["start_event"] ? 1 : 0,
		"end_event" : scenario["end_event"] ? 1 : 0,
		"restored" : scenario["restored"] ? 1 : 0,
		"pending_events" : arrayp(event_queues[id]) ?
			sizeof(event_queues[id]) : 0,
		"detail" : scenario["detail"],
	]);
	return result;
}

int start_combat_scenario(string id)
{
	object me;
	object room;
	object dummy;
	mapping scenario;
	string other_id;
	string error;

	if (! mapp(profiles[id]) || ! objectp(me = actors[id]) ||
	    me->is_ghost() || me->is_fighting() || me->is_busy() ||
	    ! objectp(environment(me)))
		return 0;
	foreach (other_id in keys(scenarios))
		if (mapp(scenarios[other_id]) &&
		    (scenarios[other_id]["status"] == "preparing" ||
		     scenarios[other_id]["status"] == "running" ||
		     scenarios[other_id]["status"] == "stopping"))
			return 0;

	error = catch(call_other(AI_TEST_ROOM, "???"));
	room = find_object(AI_TEST_ROOM);
	if (stringp(error) || ! objectp(room))
		return 0;
	dummy = new(AI_TEST_DUMMY);
	if (! objectp(dummy))
		return 0;

	scenario = ([
		"type" : "combat",
		"status" : "preparing",
		"started_at" : time(),
		"original_room" : base_name(environment(me)),
		"qi" : me->query("qi"),
		"eff_qi" : me->query("eff_qi"),
		"jing" : me->query("jing"),
		"eff_jing" : me->query("eff_jing"),
		"neili" : me->query("neili"),
		"activity" : mapp(me->query("ai/state/activity")) ?
			copy(me->query("ai/state/activity")) : 0,
		"goal" : me->query("ai/state/goal"),
		"goal_since" : me->query("ai/state/goal_since"),
		"intent" : me->query("ai/state/intent"),
		"event_queue" : arrayp(event_queues[id]) ?
			event_queues[id] + ({}) : ({}),
		"next_action" : me->query_temp("ai/next_action"),
		"dummy" : dummy,
		"start_event" : 0,
		"end_event" : 0,
		"restored" : 0,
		"detail" : "preparing isolated nonlethal combat",
	]);
	scenarios[id] = scenario;
	event_queues[id] = ({});
	me->set_temp("ai/scenario", "combat");
	me->remove_all_enemy(1);
	me->remove_all_killer();
	me->delete_temp("pending/fight");
	if (catch(me->move(room)) || environment(me) != room ||
	    ! dummy->move(room))
	{
		if (objectp(dummy))
			destruct(dummy);
		me->delete_temp("ai/scenario");
		event_queues[id] = scenario["event_queue"];
		if (file_size(scenario["original_room"] + ".c") >= 0)
			catch(me->move(scenario["original_room"]));
		scenario["status"] = "failed";
		scenario["finished_at"] = time();
		scenario["detail"] = "unable to enter isolated room";
		scenarios[id] = scenario;
		add_metric(id, "scenarios_failed", 1);
		return 0;
	}

	dummy->set("ai_test_owner", id);
	me->delete_temp("ai/nearby_fighters");
	me->set_temp("ai/was_fighting", 0);
	me->fight_ob(dummy);
	dummy->fight_ob(me);
	scenario["status"] = "running";
	scenario["detail"] = "combat running";
	scenarios[id] = scenario;
	add_metric(id, "scenarios_started", 1);
	perceive_world(me, profiles[id]);
	call_out("stop_combat_scenario", 5, id);
	return 1;
}

int start_supply_activity(string id, int seed_money)
{
	object me;
	mapping profile;
	mapping activity;
	mapping candidate;

	if (! mapp(profile = profiles[id]) || ! objectp(me = actors[id]) ||
	    me->is_ghost() || me->is_fighting() || me->is_busy())
		return 0;
	foreach (candidate in profile["activities"])
		if (mapp(candidate) && candidate["action"] == "supplies")
		{
			activity = copy(candidate);
			break;
		}
	if (! mapp(activity))
		return 0;
	if (seed_money)
		MONEY_D->pay_player(me, 3000);
	activity["step"] = "travel";
	activity["started_at"] = time();
	activity["interrupted"] = 0;
	me->set("ai/state/activity", activity);
	me->set_temp("ai/force_activity", 1);
	me->set_temp("ai/next_activity", time());
	me->set_temp("ai/next_action", time() + 1);
	add_metric(id, "activities_started", 1);
	trace_event(me, "activity_forced=" + activity["id"]);
	return 1;
}

int start_profile_activity(string id, string activity_id)
{
	object me;
	mapping profile;
	mapping candidate;
	mapping activity;

	if (! mapp(profile = profiles[id]) || ! objectp(me = actors[id]) ||
	    ! stringp(activity_id) || me->is_ghost() || me->is_fighting() ||
	    me->is_busy())
		return 0;
	foreach (candidate in profile["activities"])
		if (mapp(candidate) && candidate["id"] == activity_id)
		{
			activity = copy(candidate);
			break;
		}
	if (! mapp(activity))
		return 0;
	activity["step"] = "travel";
	activity["started_at"] = time();
	activity["interrupted"] = 0;
	me->set("ai/state/activity", activity);
	me->set_temp("ai/force_activity", 1);
	me->set_temp("ai/next_activity", time());
	me->set_temp("ai/next_action", time() + 1);
	add_metric(id, "activities_started", 1);
	trace_event(me, "activity_forced=" + activity["id"]);
	return 1;
}

void stop_combat_scenario(string id)
{
	object me;
	object dummy;
	mapping scenario;
	string restore_room;

	if (! mapp(scenario = scenarios[id]) ||
	    scenario["status"] != "running")
		return;
	me = actors[id];
	dummy = scenario["dummy"];
	scenario["status"] = "stopping";
	scenarios[id] = scenario;
	if (objectp(me))
	{
		me->remove_all_enemy(1);
		me->remove_all_killer();
		me->delete_temp("pending/fight");
	}
	if (objectp(dummy))
	{
		dummy->remove_all_enemy(1);
		dummy->remove_all_killer();
	}
	if (! objectp(me))
	{
		scenario["status"] = "failed";
		scenario["finished_at"] = time();
		scenario["detail"] = "AI player disappeared during scenario";
		scenarios[id] = scenario;
		add_metric(id, "scenarios_failed", 1);
		if (objectp(dummy))
			destruct(dummy);
		return;
	}

	me->set("qi", scenario["qi"]);
	me->set("eff_qi", scenario["eff_qi"]);
	me->set("jing", scenario["jing"]);
	me->set("eff_jing", scenario["eff_jing"]);
	me->set("neili", scenario["neili"]);
	if (mapp(scenario["activity"]))
		me->set("ai/state/activity", scenario["activity"]);
	else
		me->delete("ai/state/activity");
	restore_room = scenario["original_room"];
	if (! stringp(restore_room) || file_size(restore_room + ".c") < 0)
		restore_room = profiles[id]["home"];
	if (! catch(me->move(restore_room)) && objectp(environment(me)) &&
	    base_name(environment(me)) == restore_room)
		scenario["restored"] = 1;
	me->set_temp("ai/next_action", scenario["next_action"] > time() ?
		scenario["next_action"] : time() + 5);
	perceive_world(me, profiles[id]);
	me->delete_temp("ai/scenario");
	me->delete_temp("ai/nearby_fighters");
	clear_scenario_events(id);
	if (arrayp(scenario["event_queue"]))
		event_queues[id] = scenario["event_queue"];
	else
		event_queues[id] = ({});
	if (stringp(scenario["goal"]))
		me->set("ai/state/goal", scenario["goal"]);
	if (intp(scenario["goal_since"]))
		me->set("ai/state/goal_since", scenario["goal_since"]);
	if (stringp(scenario["intent"]))
		me->set("ai/state/intent", scenario["intent"]);
	if (objectp(dummy))
		destruct(dummy);
	scenario["finished_at"] = time();
	if (scenario["start_event"] && scenario["end_event"] &&
	    scenario["restored"])
	{
		scenario["status"] = "passed";
		scenario["detail"] = "combat events observed and player restored";
		add_metric(id, "scenarios_passed", 1);
	} else
	{
		scenario["status"] = "failed";
		scenario["detail"] = "missing combat event or restore postcondition";
		add_metric(id, "scenarios_failed", 1);
	}
	map_delete(scenario, "dummy");
	scenarios[id] = scenario;
	catch(me->save());
}

private void initialize_login(object login, string id, mapping profile)
{
	login->set("id", id);
	login->set("surname", "");
	login->set("purename", profile["name"]);
	login->set("name", profile["name"]);
	login->set("registered", 1);
	login->set("body", USER_OB);
	login->set("email", "local-ai@invalid");
	login->set("password", crypt(id + "-standalone-ai", "zj"));
	login->set("last_on", time());
	login->set("last_from", "standalone-ai");
}

private void initialize_player(object me, string id, mapping profile)
{
	int level;

	level = profile["level"];
	me->set_name(profile["name"], ({ id }));
	me->set("ai_player", 1);
	me->set("ai/profile", id);
	me->set("registered", 1);
	me->set("born", "中原人士");
	me->set("race", "人类");
	me->set("gender", profile["gender"]);
	me->set("title", profile["title"]);
	me->set("character", "忠厚老实");
	me->set("can_speak", 1);
	me->set("birthday", time() - 20 * 259200);
	me->set("str", 20 + random(6));
	me->set("dex", 20 + random(6));
	me->set("con", 20 + random(6));
	me->set("int", 20 + random(6));
	me->set("kar", 15 + random(11));
	me->set("per", 18 + random(8));
	me->set("combat_exp", profile["exp"]);
	me->set("potential", 5000);
	me->set("experience", 1000);
	me->set("score", 100);
	me->set("max_neili", level * 12);
	me->set("neili", level * 12);
	me->set("max_jingli", level * 8);
	me->set("jingli", level * 8);
	me->set("food", 500);
	me->set("water", 500);
	me->set("channels", ({}));
	me->set("env/wimpy", 35);
	me->set("env/combatd", 4);
	me->set("startroom", profile["home"]);

	me->set_skill("force", level);
	me->set_skill("dodge", level);
	me->set_skill("parry", level);
	me->set_skill("unarmed", level);
	me->set_skill("sword", level);
}

private void initialize_runtime_state(object me, mapping profile)
{
	int route_index;
	string id;

	if (! objectp(me))
		return;
	if (! stringp(me->query("ai/state/goal")))
	{
		me->set("ai/state/goal", "观察环境");
		me->set("ai/state/goal_since", time());
	}
	if (! me->query("ai/state/initialized"))
	{
		route_index = profile["route_offset"];
		me->set("ai/state/route_index", route_index);
		me->set("ai/state/initialized", 1);
	}
	if (! arrayp(me->query("ai/state/recent_rooms")))
		me->set("ai/state/recent_rooms", ({}));
	if (! mapp(me->query("ai/state/relations")))
		me->set("ai/state/relations", ([]));
	if (me->query("food") > me->max_food_capacity())
		me->set("food", me->max_food_capacity());
	if (me->query("water") > me->max_water_capacity())
		me->set("water", me->max_water_capacity());
	me->set_temp("ai_managed", 1);
	me->set_temp("ai/next_action", time() + 3 + random(8));
	me->set_temp("ai/next_save", time() + AI_SAVE_INTERVAL);
	me->set_temp("ai/next_survival", time() + AI_SURVIVAL_INTERVAL);
	if (! me->query_temp("ai/next_activity"))
		me->set_temp("ai/next_activity",
			time() + AI_ACTIVITY_START_DELAY + random(20));
	id = me->query("ai/profile");
	if (stringp(id))
	{
		metric_bucket(id);
		if (! arrayp(event_queues[id]))
			event_queues[id] = ({});
	}
}

private object load_player(string id, mapping profile)
{
	object login;
	object me;
	object item;
	string room;
	string save_error;
	int new_login;
	int restored;
	int saved;

	me = find_player(id);
	if (objectp(me))
	{
		if (! me->query("ai_player"))
		{
			log_file("ai-player", sprintf("Refusing to replace non-AI player %s.\n", id));
			return 0;
		}
		initialize_runtime_state(me, profile);
		return me;
	}

	login = new(LOGIN_OB);
	login->set("id", id);
	if (! login->restore())
	{
		initialize_login(login, id, profile);
		new_login = 1;
	}
	if (! login->prepare_ai_login())
	{
		log_file("ai-player", sprintf("Unable to prepare login uid for %s.\n", id));
		destruct(login);
		return 0;
	}

	me = LOGIN_D->make_body(login);
	if (! objectp(me))
	{
		destruct(login);
		return 0;
	}
	if (new_login)
	{
		save_error = catch(saved = login->save());
		if (stringp(save_error) || ! saved)
		{
			log_file("ai-player", sprintf("Unable to save initial login for %s.\n", id));
			destruct(me);
			destruct(login);
			return 0;
		}
	}

	restored = me->restore();
	if (! restored)
		initialize_player(me, id, profile);
	else
	{
		me->set("ai_player", 1);
		me->set("ai/profile", id);
	}
	if (! me->prepare_ai_runtime())
	{
		log_file("ai-player", sprintf("Unable to initialize runtime uid for %s.\n", id));
		destruct(me);
		destruct(login);
		return 0;
	}

	room = me->query_full_save_room();
	me->setup();
	initialize_runtime_state(me, profile);

	if (! stringp(room) || file_size(room + ".c") < 0)
		room = profile["home"];
	if (catch(me->move(room)) || ! environment(me))
		me->move(profile["home"]);

	if (! restored)
	{
		item = new("/clone/cloth/cloth");
		if (objectp(item) && item->move(me, 1))
			catch(item->wear());
		me->add_money("silver", 30 + random(50));
	}

	catch(NAME_D->map_name(me->query("name"), id));
	UPDATE_D->check_user(me);
	TOP_D->map_user(me);
	me->save();
	destruct(login);
	return me;
}

private void load_all_players()
{
	string id;
	string error;
	object me;

	foreach (id in keys(profiles))
	{
		error = catch(me = load_player(id, profiles[id]));
		if (stringp(error) && error != "")
			log_file("ai-player", sprintf("%s load %s: %s\n",
				ctime(time()), id, error));
		else
			actors[id] = me;
	}
}

void reload_players()
{
	load_all_players();
}

private void trace_event(object me, string event)
{
	string id;

	if (! objectp(me) || ! stringp(id = me->query("ai/profile")) ||
	    ! tracing[id])
		return;
	log_file("ai-player-trace", sprintf("%s %s %s\n",
		ctime(time()), id, event));
}

private void set_goal(object me, string goal, string intent)
{
	if (me->query("ai/state/goal") != goal)
	{
		me->set("ai/state/goal", goal);
		me->set("ai/state/goal_since", time());
		trace_event(me, "goal=" + goal);
	}
	me->set("ai/state/intent", intent);
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

private string relation_tier(object me, string player_id)
{
	mapping relations;
	mapping relation;
	int familiarity;

	relations = me->query("ai/state/relations");
	if (! mapp(relations) || ! mapp(relation = relations[player_id]))
		return "stranger";
	familiarity = relation["familiarity"];
	if (familiarity >= 30)
		return "friend";
	if (familiarity >= 10)
		return "familiar";
	if (familiarity >= 3)
		return "recognized";
	return "stranger";
}

private mapping gain_familiarity(mapping relation, int day_key)
{
	if (! mapp(relation))
		relation = ([ "familiarity" : 0, "daily_gain" : 0 ]);
	if (relation["day_key"] != day_key)
	{
		relation["day_key"] = day_key;
		relation["daily_gain"] = 0;
	}
	if (relation["familiarity"] < 100 &&
	    relation["daily_gain"] < AI_RELATION_DAILY_GAIN)
	{
		relation["familiarity"] += 1;
		relation["daily_gain"] += 1;
	}
	return relation;
}

private int event_drop_index(mixed *queue, int incoming_priority)
{
	mapping event;
	int lowest;
	int index;
	int i;

	if (! arrayp(queue) || ! sizeof(queue))
		return -1;
	lowest = 1000000;
	index = 0;
	for (i = 0; i < sizeof(queue); i++)
	{
		event = queue[i];
		if (! mapp(event) || event["priority"] < lowest)
		{
			lowest = mapp(event) ? event["priority"] : -1;
			index = i;
		}
	}
	if (incoming_priority <= lowest)
		return -1;
	return index;
}

private int event_pick_index(mixed *queue)
{
	mapping event;
	int best;
	int priority;
	int i;

	best = 0;
	priority = -1;
	for (i = 0; i < sizeof(queue); i++)
	{
		event = queue[i];
		if (mapp(event) && event["priority"] > priority)
		{
			best = i;
			priority = event["priority"];
		}
	}
	return best;
}

private void enqueue_event(object me, string type, object source,
	string source_id, string source_name, string detail, int priority)
{
	mixed *queue;
	mixed *remaining;
	mapping event;
	string id;
	int i;
	int drop_index;

	if (! objectp(me) || ! stringp(id = me->query("ai/profile")) ||
	    ! mapp(profiles[id]) || ! stringp(type))
		return;
	if (objectp(source))
	{
		if (! stringp(source_id))
			source_id = source->query("id");
		if (! stringp(source_name))
			source_name = source->query("name");
	}
	if (! stringp(source_id))
		source_id = "";
	if (! stringp(source_name))
		source_name = "";
	if (! stringp(detail))
		detail = "";
	queue = event_queues[id];
	if (! arrayp(queue))
		queue = ({});
	for (i = sizeof(queue) - 1; i >= 0; i--)
	{
		event = queue[i];
		if (mapp(event) && event["type"] == type &&
		    event["source_id"] == source_id &&
		    event["detail"] == detail && event["at"] + 4 >= time())
			return;
	}
	if (sizeof(queue) >= AI_MAX_EVENTS)
	{
		drop_index = event_drop_index(queue, priority);
		add_metric(id, "events_dropped", 1);
		if (drop_index < 0)
			return;
		remaining = ({});
		for (i = 0; i < sizeof(queue); i++)
			if (i != drop_index)
				remaining += ({ queue[i] });
		queue = remaining;
	}
	event = ([
		"type" : type,
		"source_id" : source_id,
		"source_name" : source_name,
		"detail" : detail,
		"priority" : priority,
		"at" : time(),
		"scenario" : me->query_temp("ai/scenario"),
	]);
	queue += ({ event });
	event_queues[id] = queue;
	add_metric(id, "events_enqueued", 1);
	mark_scenario_event(id, type);
	trace_event(me, sprintf("event=%s source=%s detail=%s",
		type, source_id, detail));
}

private void mark_scenario_event(string id, string type)
{
	mapping scenario;

	if (! mapp(scenario = scenarios[id]) ||
	    (scenario["status"] != "running" &&
	     scenario["status"] != "stopping"))
		return;
	if (type == "self_combat_started")
		scenario["start_event"] = 1;
	else if (type == "self_combat_ended")
		scenario["end_event"] = 1;
	scenarios[id] = scenario;
}

private void clear_scenario_events(string id)
{
	mixed *queue;
	mixed *remaining;
	mapping event;
	int i;

	queue = event_queues[id];
	if (! arrayp(queue) || ! sizeof(queue))
		return;
	remaining = ({});
	for (i = 0; i < sizeof(queue); i++)
	{
		event = queue[i];
		if (! mapp(event) || ! event["scenario"])
			remaining += ({ event });
	}
	event_queues[id] = remaining;
}

private void remember_room(object me)
{
	object room;
	string file;
	string *recent;

	if (! objectp(room = environment(me)))
		return;
	file = base_name(room);
	if (! stringp(file))
		return;
	recent = me->query("ai/state/recent_rooms");
	if (! arrayp(recent))
		recent = ({});
	if (sizeof(recent) && recent[sizeof(recent) - 1] == file)
		return;
	recent += ({ file });
	while (sizeof(recent) > AI_MAX_RECENT_ROOMS)
		recent -= ({ recent[0] });
	me->set("ai/state/recent_rooms", recent);
	me->set("ai/state/last_room", file);
}

private void remember_interaction(object me, object player, string topic)
{
	mapping relations;
	mapping relation;
	string player_id;
	string key;
	string oldest;
	int oldest_time;
	int day_key;

	if (! objectp(player) || ! interactive(player) ||
	    ! stringp(player_id = player->query("id")))
		return;
	relations = me->query("ai/state/relations");
	if (! mapp(relations))
		relations = ([]);
	if (! mapp(relations[player_id]) && sizeof(relations) >= AI_MAX_RELATIONS)
	{
		oldest_time = time();
		foreach (key in keys(relations))
		{
			if (! mapp(relations[key]) ||
			    relations[key]["last_seen"] <= oldest_time)
			{
				oldest = key;
				oldest_time = mapp(relations[key]) ?
					relations[key]["last_seen"] : 0;
			}
		}
		if (stringp(oldest))
			map_delete(relations, oldest);
	}
	relation = relations[player_id];
	if (! mapp(relation))
		relation = ([ "familiarity" : 0, "daily_gain" : 0 ]);
	day_key = time() / 86400;
	relation = gain_familiarity(relation, day_key);
	relation["name"] = player->query("name");
	relation["last_seen"] = time();
	if (stringp(topic) && topic != "")
		relation["last_topic"] = topic;
	relations[player_id] = relation;
	me->set("ai/state/relations", relations);
	me->set("ai/state/last_social", time());
}

private int valid_room_file(string file, string zone)
{
	object room;
	string error;

	if (! stringp(file) || ! stringp(zone) ||
	    strsrch(file, zone) != 0 || file_size(file + ".c") < 0 ||
	    strsrch(file, "/jingji/") != -1 ||
	    strsrch(file, "/fuben/") != -1 ||
	    strsrch(file, "/prison") != -1)
		return 0;
	error = catch(call_other(file, "???"));
	room = find_object(file);
	if (error || ! objectp(room) || ! room->is_room() || clonep(room) ||
	    room->query("close_room") || room->query("out_room") ||
	    room->query("no_save_location") || room->query("no_login"))
		return 0;
	return 1;
}

private string find_route_step(object me, string start, string target, string zone)
{
	string *queue;
	mapping visited;
	mapping depth;
	mapping first_step;
	mapping cached;
	object room;
	mapping exits;
	string current;
	string dir;
	string dest;
	string key;
	string id;
	int i;

	if (start == target || ! valid_room_file(target, zone))
		return 0;
	key = start + "|" + target;
	if (objectp(me))
		id = me->query("ai/profile");
	cached = route_cache[key];
	if (mapp(cached) && cached["expires"] >= time() &&
	    stringp(cached["dir"]) && cached["dir"] != "")
	{
		add_metric(id, "cache_hits", 1);
		if (objectp(me))
			me->set_temp("ai/route_cache_key", key);
		return cached["dir"];
	}
	if (mapp(cached))
		map_delete(route_cache, key);
	add_metric(id, "cache_misses", 1);
	add_metric(id, "path_searches", 1);
	queue = ({ start });
	visited = ([ start : 1 ]);
	depth = ([ start : 0 ]);
	first_step = ([ start : "" ]);
	for (i = 0; i < sizeof(queue) && i < AI_MAX_PATH_NODES; i++)
	{
		add_metric(id, "path_nodes", 1);
		current = queue[i];
		if (depth[current] >= AI_MAX_PATH_DEPTH ||
		    ! valid_room_file(current, zone))
			continue;
		room = find_object(current);
		exits = room->query("exits");
		if (! mapp(exits))
			continue;
		foreach (dir in keys(exits))
		{
			dest = exits[dir];
			if (! stringp(dest) || visited[dest] ||
			    ! valid_room_file(dest, zone))
				continue;
			visited[dest] = 1;
			depth[dest] = depth[current] + 1;
			first_step[dest] = current == start ? dir : first_step[current];
			if (dest == target)
			{
				route_cache[key] = ([
					"dir" : first_step[dest],
					"expires" : time() + AI_ROUTE_CACHE_TTL,
				]);
				if (objectp(me))
					me->set_temp("ai/route_cache_key", key);
				return first_step[dest];
			}
			queue += ({ dest });
		}
	}
	return 0;
}

private void update_survival(object me)
{
	if (! objectp(me) || me->is_ghost() || ! living(me))
		return;
	if (me->query("food") > 0)
		me->add("food", -1);
	if (me->query("water") > 0)
		me->add("water", -1);
}

private int replenish(object me, string need)
{
	object item;
	int result;

	if (need == "food")
	{
		item = new("/clone/food/baozi");
		if (! objectp(item) || ! item->move(me, 1))
			return 0;
		result = run_action(me, "eat", "baozi");
	} else
	if (need == "water")
	{
		item = new("/clone/food/jiudai");
		if (! objectp(item) || ! item->move(me, 1))
			return 0;
		result = run_action(me, "drink", "jiudai");
	} else
		return 0;
	if (objectp(item))
		destruct(item);
	return result;
}

private int inventory_count(object me, string item_id)
{
	object ob;
	int amount;
	int total;

	if (! objectp(me) || ! stringp(item_id))
		return 0;
	total = 0;
	foreach (ob in all_inventory(me))
	{
		if (! objectp(ob) || ! ob->id(item_id))
			continue;
		amount = ob->query_amount();
		total += amount > 0 ? amount : 1;
	}
	return total;
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

private object find_usable_supply(object me, string item_id, string need)
{
	object ob;

	if (! objectp(me) || ! stringp(item_id))
		return 0;
	foreach (ob in all_inventory(me))
	{
		if (! objectp(ob) || ! ob->id(item_id))
			continue;
		if (need == "water" && mapp(ob->query("liquid")) &&
			ob->query("liquid/remaining") > 0)
			return ob;
		if (need == "food" && ob->query("food_remaining") > 0)
			return ob;
	}
	return 0;
}

private int adapter_failure(object me, int result)
{
	string id;

	if (! objectp(me) || ! stringp(id = me->query("ai/profile")))
		return result;
	add_metric(id, "adapter_attempts", 1);
	if (result == ADAPTER_PRECONDITION_FAILED)
		add_metric(id, "adapter_precondition_failures", 1);
	else if (result == ADAPTER_COMMAND_FAILED)
		add_metric(id, "adapter_command_failures", 1);
	else if (result == ADAPTER_POSTCONDITION_FAILED)
		add_metric(id, "adapter_postcondition_failures", 1);
	return result;
}

private int purchase_supply(object me, mapping activity, string need)
{
	string item_id;
	string vendor_id;
	string room_path;
	object vendor;
	int cost;
	int before_count;
	int after_count;
	int before_money;
	int after_money;
	int result;

	if (! objectp(me) || ! mapp(activity) ||
	    ! stringp(item_id = activity[need == "food" ? "food_id" : "water_id"]) ||
	    ! intp(cost = activity[need == "food" ? "food_cost" : "water_cost"]) ||
	    ! stringp(room_path = activity["target"]) ||
	    ! stringp(vendor_id = activity["vendor"]) ||
	    ! objectp(environment(me)) || base_name(environment(me)) != room_path)
		return adapter_failure(me, ADAPTER_PRECONDITION_FAILED);
	vendor = present(vendor_id, environment(me));
	if (! objectp(vendor) || ! vendor->is_vendor() ||
	    carried_money_value(me) < cost)
		return adapter_failure(me, ADAPTER_PRECONDITION_FAILED);
	before_count = inventory_count(me, item_id);
	before_money = carried_money_value(me);
	result = run_action(me, "buy", "1 " + item_id);
	if (! result)
		return adapter_failure(me, ADAPTER_COMMAND_FAILED);
	after_count = inventory_count(me, item_id);
	after_money = carried_money_value(me);
	if (after_count <= before_count || after_money >= before_money ||
	    before_money - after_money != cost)
		return adapter_failure(me, ADAPTER_POSTCONDITION_FAILED);
	add_metric(me->query("ai/profile"), "adapter_attempts", 1);
	add_metric(me->query("ai/profile"), "adapter_successes", 1);
	trace_event(me, "adapter=purchase need=" + need + " item=" + item_id);
	return ADAPTER_SUCCESS;
}

private int consume_supply(object me, mapping activity, string need)
{
	string item_id;
	int before_resource;
	int after_resource;
	int max_resource;
	int result;
	object ob;

	if (! objectp(me) || ! mapp(activity) ||
	    ! stringp(item_id = activity[need == "food" ? "food_id" : "water_id"]) ||
	    ! objectp(ob = find_usable_supply(me, item_id, need)))
		return adapter_failure(me, ADAPTER_PRECONDITION_FAILED);
	before_resource = me->query(need);
	max_resource = need == "food" ? me->max_food_capacity() :
		me->max_water_capacity();
	result = run_action(me, need == "food" ? "eat" : "drink", item_id);
	if (! result)
		return adapter_failure(me, ADAPTER_COMMAND_FAILED);
	after_resource = me->query(need);
	if (after_resource <= before_resource || after_resource > max_resource)
		return adapter_failure(me, ADAPTER_POSTCONDITION_FAILED);
	if (need == "water" && ob->query("liquid/remaining") < 1)
		destruct(ob);
	add_metric(me->query("ai/profile"), "adapter_attempts", 1);
	add_metric(me->query("ai/profile"), "adapter_successes", 1);
	trace_event(me, "adapter=consume need=" + need + " item=" + item_id);
	return ADAPTER_SUCCESS;
}

private int rest_safely(object me, mapping activity)
{
	int qi_before;
	int jing_before;
	int qi_max;
	int jing_max;
	int qi_gain;
	int jing_gain;

	if (! objectp(me) || ! mapp(activity) || me->is_fighting() ||
	    me->is_busy() || ! objectp(environment(me)) ||
	    base_name(environment(me)) != activity["target"])
		return adapter_failure(me, ADAPTER_PRECONDITION_FAILED);
	qi_before = me->query("qi");
	jing_before = me->query("jing");
	qi_max = me->query("eff_qi");
	jing_max = me->query("eff_jing");
	qi_gain = qi_max / 12;
	jing_gain = jing_max / 12;
	if (qi_gain < 1)
		qi_gain = 1;
	if (jing_gain < 1)
		jing_gain = 1;
	if (qi_before < qi_max)
		me->set("qi", qi_before + qi_gain > qi_max ?
			qi_max : qi_before + qi_gain);
	if (jing_before < jing_max)
		me->set("jing", jing_before + jing_gain > jing_max ?
			jing_max : jing_before + jing_gain);
	if (me->query("qi") < qi_before || me->query("qi") > qi_max ||
	    me->query("jing") < jing_before || me->query("jing") > jing_max)
		return adapter_failure(me, ADAPTER_POSTCONDITION_FAILED);
	add_metric(me->query("ai/profile"), "adapter_attempts", 1);
	add_metric(me->query("ai/profile"), "adapter_successes", 1);
	add_metric(me->query("ai/profile"), "rests_completed", 1);
	trace_event(me, sprintf("adapter=rest qi=%d->%d jing=%d->%d",
		qi_before, me->query("qi"), jing_before, me->query("jing")));
	return ADAPTER_SUCCESS;
}

private int run_patrol_activity(object me, mapping profile, mapping activity)
{
	string *stops;
	string target;
	string current;
	int index;

	stops = activity["stops"];
	if (! arrayp(stops) || ! sizeof(stops))
		stops = ({ activity["target"] });
	index = activity["stop_index"];
	if (index < 0 || index >= sizeof(stops))
		index = 0;
	target = stops[index];
	current = base_name(environment(me));
	activity["target"] = target;
	activity["step"] = sprintf("patrol_%d", index);
	me->set("ai/state/activity", activity);
	if (current != target)
	{
		set_goal(me, activity["name"], "巡游前往 " + target);
		move_toward(me, profile, target);
		return 1;
	}
	run_action(me, "look", 0);
	add_metric(me->query("ai/profile"), "patrol_stops", 1);
	index++;
	if (index >= sizeof(stops))
	{
		finish_activity(me, activity, "patrol_completed");
		return 1;
	}
	activity["stop_index"] = index;
	activity["target"] = stops[index];
	activity["step"] = sprintf("patrol_%d", index);
	me->set("ai/state/activity", activity);
	return 1;
}

private int run_social_activity(object me, mapping profile, mapping activity)
{
	object partner;
	mapping partner_activity;
	string partner_id;
	string target;
	string line;

	partner_id = activity["partner"];
	target = activity["target"];
	if (! stringp(partner_id) || ! mapp(profiles[partner_id]) ||
	    ! objectp(partner = actors[partner_id]) || partner == me ||
	    partner->is_fighting() || partner->is_busy())
	{
		finish_activity(me, activity, "partner_unavailable");
		return 1;
	}
	if (activity["action"] != "social_wait" &&
	    ! mapp(partner_activity = partner->query("ai/state/activity")))
	{
		partner_activity = ([
			"id" : "meet_" + me->query("ai/profile"),
			"name" : "与" + me->query("name") + "碰面",
			"target" : target,
			"action" : "social_wait",
			"partner" : me->query("ai/profile"),
			"line" : "正好在这里碰见你，一同说几句话吧。",
			"cooldown" : activity["cooldown"],
			"started_at" : time(),
			"step" : "travel",
			"interrupted" : 0,
		]);
		partner->set("ai/state/activity", partner_activity);
		partner->set_temp("ai/next_action", time() + 2);
		add_metric(partner_id, "activities_started", 1);
		trace_event(partner, "activity_invited=" + partner_activity["id"]);
	}
	if (base_name(environment(me)) != target)
	{
		set_goal(me, activity["name"], "前往会面点 " + target);
		move_toward(me, profile, target);
		return 1;
	}
	if (! objectp(environment(partner)) ||
	    base_name(environment(partner)) != target)
	{
		activity["step"] = "waiting_partner";
		me->set("ai/state/activity", activity);
		set_goal(me, activity["name"], "在会面点等候 " + partner_id);
		return 1;
	}
	line = activity["line"];
	if (! stringp(line) || line == "")
		line = "今日正好碰面，一同走走也好。";
	run_action(me, "say", line);
	add_metric(me->query("ai/profile"), "social_meetings", 1);
	finish_activity(me, activity, "met_" + partner_id);
	return 1;
}

private string route_target(object me, mapping profile)
{
	string *route;
	mapping schedule;
	string period;
	int index;

	period = current_period();
	schedule = profile["schedule"];
	if (mapp(schedule))
		route = schedule[period];
	if (! arrayp(route) || ! sizeof(route))
		route = profile["route"];
	if (! arrayp(route) || ! sizeof(route))
		return profile["home"];
	index = me->query("ai/state/route_index");
	if (index < 0)
		index = 0;
	return route[index % sizeof(route)];
}

private object nearby_human(object me)
{
	object ob;

	if (! objectp(environment(me)))
		return 0;
	foreach (ob in all_inventory(environment(me)))
		if (ob != me && interactive(ob))
			return ob;
	return 0;
}

private mapping nearby_humans(object me)
{
	mapping result;
	object ob;
	string id;

	result = ([]);
	if (! objectp(environment(me)))
		return result;
	foreach (ob in all_inventory(environment(me)))
	{
		if (ob == me || ! interactive(ob) ||
		    ! stringp(id = ob->query("id")))
			continue;
		result[id] = ob->query("name");
	}
	return result;
}

private mapping nearby_fighters(object me)
{
	mapping result;
	object ob;
	string id;

	result = ([]);
	if (! objectp(environment(me)))
		return result;
	foreach (ob in all_inventory(environment(me)))
	{
		if (ob == me || ! living(ob) || ! ob->is_fighting())
			continue;
		id = ob->query("id");
		if (! stringp(id) || id == "")
			id = base_name(ob);
		result[id] = ob->query("name");
	}
	return result;
}

private void perceive_world(object me, mapping profile)
{
	mapping current;
	mapping previous;
	mapping fighting;
	mapping previous_fighting;
	object player;
	string key;
	string period;
	string old_period;
	int was_fighting;
	int is_fighting;

	if (! objectp(me) || ! objectp(environment(me)))
		return;
	current = nearby_humans(me);
	previous = me->query_temp("ai/nearby_humans");
	if (! mapp(previous))
		previous = ([]);
	foreach (key in keys(current))
	{
		if (undefinedp(previous[key]))
		{
			player = find_player(key);
			if (objectp(player))
				remember_interaction(me, player, "enter");
			enqueue_event(me, "player_enter", player, key,
				current[key], base_name(environment(me)), 45);
		}
	}
	foreach (key in keys(previous))
		if (undefinedp(current[key]))
			enqueue_event(me, "player_leave", 0, key,
				previous[key], base_name(environment(me)), 20);
	me->set_temp("ai/nearby_humans", current);

	fighting = nearby_fighters(me);
	previous_fighting = me->query_temp("ai/nearby_fighters");
	if (! mapp(previous_fighting))
		previous_fighting = ([]);
	foreach (key in keys(fighting))
		if (undefinedp(previous_fighting[key]))
			enqueue_event(me, "nearby_combat_started", 0, key,
				fighting[key], base_name(environment(me)), 75);
	foreach (key in keys(previous_fighting))
		if (undefinedp(fighting[key]))
			enqueue_event(me, "nearby_combat_ended", 0, key,
				previous_fighting[key], base_name(environment(me)), 35);
	me->set_temp("ai/nearby_fighters", fighting);

	was_fighting = me->query_temp("ai/was_fighting") ? 1 : 0;
	is_fighting = me->is_fighting() ? 1 : 0;
	if (is_fighting != was_fighting)
		enqueue_event(me, is_fighting ? "self_combat_started" :
			"self_combat_ended", me, me->query("id"),
			me->query("name"), base_name(environment(me)),
			is_fighting ? 100 : 80);
	me->set_temp("ai/was_fighting", is_fighting);

	period = current_period();
	old_period = me->query_temp("ai/period");
	if (stringp(old_period) && old_period != period)
		enqueue_event(me, "time_period_changed", 0, "", "",
			old_period + "->" + period, 30);
	me->set_temp("ai/period", period);
}

private void respawn_player(object me, mapping profile)
{
	string id;

	if (! objectp(me))
		return;
	id = me->query("ai/profile");
	me->remove_all_killer();
	me->reincarnate();
	me->set("eff_qi", me->query("max_qi") + me->query_gain_qi());
	me->set("qi", me->query("eff_qi"));
	me->set("eff_jing", me->query("max_jing"));
	me->set("jing", me->query("max_jing"));
	me->set("neili", me->query("max_neili"));
	me->move(profile["home"]);
	me->save();
	add_metric(id, "respawns", 1);
}

private int run_action(object me, string verb, string arg)
{
	string command;
	string before_room;
	string after_room;
	int result;

	if (! objectp(me) || ! me->query("ai_player"))
		return 0;

	switch (verb)
	{
	case "go":
	case "say":
	case "look":
	case "nod":
	case "sigh":
	case "eat":
	case "drink":
	case "buy":
		break;
	default:
		return 0;
	}

	before_room = objectp(environment(me)) ? base_name(environment(me)) : "";
	command = (! stringp(arg) || arg == "") ? verb : verb + " " + arg;
	add_metric(me->query("ai/profile"), "actions", 1);
	result = me->force_me(command);
	if (! objectp(me))
		return result;
	after_room = objectp(environment(me)) ? base_name(environment(me)) : "";
	me->set("ai/state/last_action", command);
	me->set("ai/state/last_action_at", time());
	trace_event(me, sprintf("action=%s result=%d room=%s",
		command, result, after_room));
	if (verb == "go" && before_room != after_room)
		remember_room(me);
	else if (! result || (verb == "go" && before_room == after_room))
	{
		add_metric(me->query("ai/profile"), "action_failures", 1);
		if (verb == "go" &&
		    stringp(me->query_temp("ai/route_cache_key")))
			map_delete(route_cache, me->query_temp("ai/route_cache_key"));
	}
	me->delete_temp("ai/route_cache_key");
	return result;
}

private void say_profile_line(object me, mapping profile)
{
	string *lines;
	string line;
	string previous;
	int index;

	lines = profile["lines"];
	if (! arrayp(lines) || ! sizeof(lines))
		return;
	index = random(sizeof(lines));
	line = lines[index];
	previous = me->query("ai/state/last_speech");
	if (sizeof(lines) > 1 && line == previous)
		line = lines[(index + 1) % sizeof(lines)];
	me->set("ai/state/last_speech", line);
	run_action(me, "say", line);
}

private void say_context_line(object me, mapping profile)
{
	mapping period_lines;
	mapping room_lines;
	string line;
	string room;

	room = objectp(environment(me)) ? base_name(environment(me)) : "";
	room_lines = profile["room_lines"];
	if (mapp(room_lines) && stringp(line = room_lines[room]) &&
	    line != "" && random(100) < 65)
	{
		me->set("ai/state/last_speech", line);
		run_action(me, "say", line);
		return;
	}
	period_lines = profile["period_lines"];
	if (! mapp(period_lines) ||
	    ! stringp(line = period_lines[current_period()]) || line == "")
	{
		say_profile_line(me, profile);
		return;
	}
	me->set("ai/state/last_speech", line);
	run_action(me, "say", line);
}

private string contextual_greeting(object me, mapping profile,
	string player_name, string tier)
{
	mapping greeting_lines;
	mapping room_lines;
	string context;
	string room;

	greeting_lines = profile["greeting_lines"];
	if (mapp(greeting_lines))
		context = greeting_lines[current_period()];
	room = objectp(environment(me)) ? base_name(environment(me)) : "";
	room_lines = profile["room_lines"];
	if (mapp(room_lines) && stringp(room_lines[room]) && random(100) < 40)
		context = room_lines[room];
	if (! stringp(context) || context == "")
		context = "今日在这里相逢，也算有缘。";
	if (tier == "friend")
		return player_name + "，又见面了。" + context;
	if (tier == "familiar")
		return player_name + "，今日也来这里走走？" + context;
	if (tier == "recognized")
		return player_name + "，我们又碰面了。" + context;
	return context;
}

private void recover_route_failure(object me, mapping profile)
{
	mapping activity;
	int failures;
	string id;

	if (! objectp(me))
		return;
	id = me->query("ai/profile");
	failures = me->query_temp("ai/route_failures") + 1;
	me->set_temp("ai/route_failures", failures);
	add_metric(id, "route_failures", 1);
	if (failures < 3)
		return;
	me->delete_temp("ai/route_failures");
	activity = me->query("ai/state/activity");
	if (mapp(activity))
	{
		interrupt_activity(me, "route_failed");
		me->delete("ai/state/activity");
		me->set_temp("ai/next_activity", time() + 120);
	}
	if (catch(me->move(profile["home"])) || ! environment(me))
		return;
	add_metric(id, "relocations", 1);
	set_goal(me, "路径恢复", "回到安全常驻点后重新安排活动");
	remember_room(me);
	me->set_temp("ai/next_action", time() + 10);
	trace_event(me, "relocate=route_failure home=" + profile["home"]);
}

private void move_locally(object me, mapping profile)
{
	mapping exits;
	string *dirs;
	string *allowed;
	string dir;
	string dest;
	string zone;
	string *recent;
	string previous;

	if (! objectp(environment(me)) ||
	    ! mapp(exits = environment(me)->query("exits")))
		return;

	zone = profile["zone"];
	recent = me->query("ai/state/recent_rooms");
	if (arrayp(recent) && sizeof(recent) > 1)
		previous = recent[sizeof(recent) - 2];
	allowed = ({});
	dirs = keys(exits);
	foreach (dir in dirs)
	{
		dest = exits[dir];
		if (stringp(dest) && dest != previous &&
		    valid_room_file(dest, zone))
			allowed += ({ dir });
	}
	if (! sizeof(allowed))
	{
		foreach (dir in dirs)
		{
			dest = exits[dir];
			if (stringp(dest) && valid_room_file(dest, zone))
				allowed += ({ dir });
		}
	}

	if (sizeof(allowed) && ! run_action(me, "go",
		allowed[random(sizeof(allowed))]))
		recover_route_failure(me, profile);
}

private int move_toward(object me, mapping profile, string target)
{
	string current;
	string dir;

	if (! objectp(environment(me)))
		return 0;
	current = base_name(environment(me));
	dir = find_route_step(me, current, target, profile["zone"]);
	if (! stringp(dir) || dir == "")
	{
		move_locally(me, profile);
		return 0;
	}
	if (! run_action(me, "go", dir))
	{
		recover_route_failure(me, profile);
		return 0;
	}
	me->delete_temp("ai/route_failures");
	return 1;
}

private void answer_question(object me, mapping profile,
	string asker_id, string topic)
{
	object asker;
	string *answers;
	string tier;

	if (! stringp(asker_id))
		asker_id = me->query_temp("ask_you");
	if (! stringp(asker_id))
		return;
	me->delete_temp("ask_you");
	asker = find_player(asker_id);
	if (! objectp(asker) || environment(asker) != environment(me))
		return;

	tier = relation_tier(me, asker_id);
	if (tier == "friend")
		answers = ({
			"既然是你来问，我会替你留心这件事。",
			"我们也算熟人了；有确切消息，我一定告诉你。",
		});
	else if (tier == "familiar")
		answers = ({
			"这件事我有些印象，不过还不敢说得太满。",
			"我替你记下了，路上若听见消息会多留意。",
		});
	else
		answers = ({
			"这件事我也只是略有耳闻，未必说得准。",
			"我暂时还没有头绪，不如去问问附近的人。",
			"江湖传闻真真假假，我可不敢随便下结论。",
		});
	set_goal(me, "回应问询", "回答 " + asker_id);
	remember_interaction(me, asker,
		stringp(topic) && topic != "" ? topic : "ask");
	run_action(me, "say", answers[random(sizeof(answers))]);
}

private mapping choose_activity(object me, mapping profile, string period)
{
	mixed *activities;
	mapping cooldowns;
	mapping activity;
	string *periods;
	int i;

	activities = profile["activities"];
	cooldowns = me->query("ai/state/activity_cooldowns");
	if (! mapp(cooldowns))
		cooldowns = ([]);
	if (! arrayp(activities) || ! sizeof(activities))
		return 0;
	for (i = 0; i < sizeof(activities); i++)
	{
		activity = activities[i];
		if (! mapp(activity) || ! stringp(activity["id"]) ||
		    ! stringp(activity["target"]) ||
		    ! stringp(activity["action"]))
			continue;
		periods = activity["periods"];
		if (arrayp(periods) && member_array(period, periods) == -1)
			continue;
		if (cooldowns[activity["id"]] > time())
			continue;
		return activity;
	}
	return 0;
}

private void finish_activity(object me, mapping activity, string outcome)
{
	mapping cooldowns;
	string id;

	if (! objectp(me) || ! mapp(activity) ||
	    ! stringp(id = me->query("ai/profile")))
		return;
	cooldowns = me->query("ai/state/activity_cooldowns");
	if (! mapp(cooldowns))
		cooldowns = ([]);
	cooldowns[activity["id"]] = time() + activity["cooldown"];
	me->set("ai/state/activity_cooldowns", cooldowns);
	me->set("ai/state/last_activity", ([
		"id" : activity["id"],
		"name" : activity["name"],
		"outcome" : outcome,
		"at" : time(),
	]));
	me->delete("ai/state/activity");
	me->delete_temp("ai/force_activity");
	me->set_temp("ai/next_activity", time() + 35 + random(30));
	add_metric(id, "activities_completed", 1);
	trace_event(me, "activity_done=" + activity["id"] +
		" outcome=" + outcome);
}

private void interrupt_activity(object me, string reason)
{
	mapping activity;
	string id;

	if (! objectp(me) || ! mapp(activity = me->query("ai/state/activity")) ||
		activity["interrupted"])
		return;
	activity["interrupted"] = 1;
	activity["interrupt_reason"] = reason;
	me->set("ai/state/activity", activity);
	id = me->query("ai/profile");
	add_metric(id, "activities_interrupted", 1);
	trace_event(me, "activity_interrupt=" + activity["id"] +
		" reason=" + reason);
}

private int run_activity(object me, mapping profile)
{
	mapping activity;
	string period;
	string current;
	string target;
	string action;
	string line;
	int started_at;

	if (! objectp(me) || ! objectp(environment(me)))
		return 0;
	activity = me->query("ai/state/activity");
	period = current_period();
	if (! mapp(activity))
	{
		if (period == "night" || me->query_temp("ai/next_activity") > time())
			return 0;
		activity = choose_activity(me, profile, period);
		if (! mapp(activity))
		{
			me->set_temp("ai/next_activity", time() + 60);
			return 0;
		}
		activity = copy(activity);
		activity["step"] = "travel";
		activity["started_at"] = time();
		activity["interrupted"] = 0;
		me->set("ai/state/activity", activity);
		add_metric(me->query("ai/profile"), "activities_started", 1);
		trace_event(me, "activity_start=" + activity["id"]);
	}
	if (activity["interrupted"])
	{
		activity["interrupted"] = 0;
		activity["step"] = "resume";
		me->set("ai/state/activity", activity);
		add_metric(me->query("ai/profile"), "activities_resumed", 1);
		trace_event(me, "activity_resume=" + activity["id"]);
	}
	started_at = activity["started_at"];
	if (started_at + 900 < time())
	{
		finish_activity(me, activity, "timeout");
		return 1;
	}
	action = activity["action"];
	if (action == "patrol")
		return run_patrol_activity(me, profile, activity);
	if (action == "social" || action == "social_wait")
		return run_social_activity(me, profile, activity);
	target = activity["target"];
	current = base_name(environment(me));
	if (current != target)
	{
		set_goal(me, activity["name"], "活动前往 " + target);
		move_toward(me, profile, target);
		return 1;
	}
	activity["step"] = "action";
	me->set("ai/state/activity", activity);
	set_goal(me, activity["name"], "在活动地点执行 " + action);
	if (action == "supplies")
	{
		if (stringp(activity["vendor"]))
		{
			if (me->query("water") * 100 < me->max_water_capacity() * 75)
			{
				if (consume_supply(me, activity, "water") == ADAPTER_SUCCESS)
					return 1;
				if (purchase_supply(me, activity, "water") == ADAPTER_SUCCESS)
					return 1;
			}
			if (me->query("food") * 100 < me->max_food_capacity() * 75)
			{
				if (consume_supply(me, activity, "food") == ADAPTER_SUCCESS)
					return 1;
				if (purchase_supply(me, activity, "food") == ADAPTER_SUCCESS)
					return 1;
			}
		}
		run_action(me, "look", 0);
		finish_activity(me, activity,
			stringp(activity["vendor"]) ? "supplies_adapted" :
			"supplies_checked");
		return 1;
	}
	if (action == "reflect")
	{
		line = activity["line"];
		if (stringp(line) && line != "")
			run_action(me, "say", line);
		else
			run_action(me, "look", 0);
		finish_activity(me, activity, "reflection");
		return 1;
	}
	if (action == "rest")
	{
		if (! activity["rest_until"])
		{
			line = activity["line"];
			if (stringp(line) && line != "")
				run_action(me, "say", line);
			else
				run_action(me, "sigh", 0);
			activity["rest_until"] = time() + activity["duration"];
			activity["step"] = "resting";
			me->set("ai/state/activity", activity);
			return 1;
		}
		if (time() < activity["rest_until"])
		{
			set_goal(me, activity["name"], "在安全地点安静休息");
			return 1;
		}
		if (rest_safely(me, activity) == ADAPTER_SUCCESS)
			finish_activity(me, activity, "rested");
		else
			finish_activity(me, activity, "rest_failed");
		return 1;
	}
	run_action(me, "look", 0);
	finish_activity(me, activity, "observed");
	return 1;
}

private int handle_next_event(object me, mapping profile)
{
	mixed *queue;
	mixed *remaining;
	mapping event;
	object source;
	string id;
	string type;
	string source_id;
	string tier;
	string name;
	int best;
	int i;
	int chance;

	id = me->query("ai/profile");
	queue = event_queues[id];
	if (! arrayp(queue) || ! sizeof(queue))
		return 0;
	best = event_pick_index(queue);
	event = queue[best];
	remaining = ({});
	for (i = 0; i < sizeof(queue); i++)
		if (i != best)
			remaining += ({ queue[i] });
	event_queues[id] = remaining;
	add_metric(id, "events_handled", 1);
	if (! mapp(event) || event["at"] + 300 < time())
		return 1;

	type = event["type"];
	if (event["scenario"] && type != "self_combat_started" &&
	    type != "self_combat_ended")
		return 1;
	if (event["priority"] >= 45 && ! event["scenario"])
		interrupt_activity(me, type);
	source_id = event["source_id"];
	name = event["source_name"];
	if (stringp(source_id) && source_id != "")
		source = find_player(source_id);

	if (type == "player_enter")
	{
		if (! objectp(source) || environment(source) != environment(me) ||
		    me->query_temp("ai/next_greeting") > time())
			return 1;
		tier = relation_tier(me, source_id);
		me->set_temp("ai/next_greeting", time() + 90 + random(60));
		set_goal(me, "留意来客", "观察 " + source_id);
		if (tier != "stranger")
			run_action(me, "say",
				contextual_greeting(me, profile, name, tier));
		else if (random(100) < 25)
			run_action(me, "say",
				contextual_greeting(me, profile, name, tier));
		else
			run_action(me, "nod", 0);
		return 1;
	}
	if (type == "ask")
	{
		answer_question(me, profile, source_id, event["detail"]);
		return 1;
	}
	if (type == "say")
	{
		if (! objectp(source) || environment(source) != environment(me) ||
		    me->query_temp("ai/next_reply") > time())
			return 1;
		tier = relation_tier(me, source_id);
		chance = strsrch(event["detail"], me->query("name")) == -1 ?
			20 : 80;
		if (tier == "friend")
			chance += 15;
		else if (tier == "familiar")
			chance += 8;
		if (random(100) >= chance)
			return 1;
		me->set_temp("ai/next_reply", time() + 60 + random(60));
		call_out("reply_to_say", 2 + random(4), me, source);
		return 1;
	}
	if (type == "nearby_combat_started")
	{
		if (profile["courage"] < 60)
		{
			set_goal(me, "避开争斗", "远离附近战斗");
			move_locally(me, profile);
		} else
		{
			set_goal(me, "观察争斗", "留意附近战况");
			run_action(me, "look", 0);
		}
		return 1;
	}
	if (type == "self_combat_ended")
	{
		set_goal(me, "战后调整", "检查自身状态");
		if (random(100) < 50)
			run_action(me, "sigh", 0);
		return 1;
	}
	if (type == "time_period_changed")
	{
		set_goal(me, "调整日程", "按当前时段行动");
		say_context_line(me, profile);
		return 1;
	}
	return 1;
}

private void think(object me, mapping profile)
{
	int roll;
	int qi_max;
	int jing_max;
	int dwell;
	string current;
	string target;
	string period;
	object human;

	if (! objectp(me))
		return;

	me->set_temp("ai/next_action",
		time() + AI_MIN_ACTION_DELAY + random(AI_ACTION_JITTER));
	if (me->query_temp("ai/scenario"))
	{
		set_goal(me, "隔离测试", "执行受控战斗场景");
		return;
	}
	if (me->query_temp("ai/force_activity"))
	{
		if (run_activity(me, profile))
			return;
		me->delete_temp("ai/force_activity");
	}

	if (! objectp(environment(me)))
	{
		set_goal(me, "返回常驻点", "恢复丢失的环境");
		me->move(profile["home"]);
		remember_room(me);
		return;
	}

	if (me->is_ghost())
	{
		set_goal(me, "死后恢复", "返回常驻点");
		respawn_player(me, profile);
		return;
	}

	if (! living(me))
	{
		set_goal(me, "恢复意识", "等待自然恢复");
		return;
	}
	if (me->is_fighting())
	{
		interrupt_activity(me, "self_combat");
		set_goal(me, "自卫", "交由玩家战斗系统处理");
		return;
	}
	if (me->is_busy())
	{
		set_goal(me, "完成当前动作", "等待忙碌结束");
		return;
	}
	if (handle_next_event(me, profile))
		return;

	if (stringp(me->query_temp("ask_you")))
	{
		answer_question(me, profile, 0, 0);
		return;
	}

	current = base_name(environment(me));
	if (! valid_room_file(current, profile["zone"]))
	{
		set_goal(me, "返回常驻点", "离开不安全房间");
		me->move(profile["home"]);
		remember_room(me);
		return;
	}
	remember_room(me);

	if (me->query("water") * 100 < me->max_water_capacity() * 35)
	{
		set_goal(me, "补充饮水", "饮用随身水袋");
		replenish(me, "water");
		return;
	}
	if (me->query("food") * 100 < me->max_food_capacity() * 35)
	{
		set_goal(me, "补充食物", "食用随身干粮");
		replenish(me, "food");
		return;
	}

	qi_max = me->query("max_qi") + me->query_gain_qi();
	jing_max = me->query("max_jing");
	if ((qi_max > 0 && me->query("qi") * 100 < qi_max * 45) ||
	    (jing_max > 0 && me->query("jing") * 100 < jing_max * 45))
	{
		set_goal(me, "休息恢复", "留在安全房间恢复状态");
		if (random(100) < 8)
			run_action(me, "sigh", 0);
		return;
	}
	if (run_activity(me, profile))
		return;

	target = route_target(me, profile);
	period = current_period();
	if (current != target)
	{
		me->delete_temp("ai/dwell_until");
		set_goal(me, period == "night" ? "夜间归家" : "按时巡游",
			"前往 " + target);
		move_toward(me, profile, target);
		return;
	}

	if (period == "night")
	{
		set_goal(me, "夜间休息", "留在常驻点");
		return;
	}
	dwell = me->query_temp("ai/dwell_until");
	if (! dwell)
	{
		dwell = time() + profile["dwell"] + random(31);
		me->set_temp("ai/dwell_until", dwell);
	}
	if (time() >= dwell)
	{
		me->add("ai/state/route_index", 1);
		me->delete_temp("ai/dwell_until");
		target = route_target(me, profile);
		set_goal(me, "日常巡游", "前往 " + target);
		move_toward(me, profile, target);
		return;
	}

	set_goal(me, "驻足观察", "在兴趣点短暂停留");
	human = nearby_human(me);
	if (objectp(human) &&
	    me->query("ai/state/last_social") + 180 < time() &&
	    random(100) < profile["talk_chance"])
	{
		remember_interaction(me, human, "nearby");
		say_profile_line(me, profile);
		return;
	}
	roll = random(100);
	if (roll < profile["talk_chance"])
	{
		if (random(100) < 35)
			say_context_line(me, profile);
		else
			say_profile_line(me, profile);
	}
	else if (roll < 18)
		run_action(me, "look", 0);
	else if (roll < 22)
		run_action(me, "nod", 0);
}

void hear_say(object listener, object speaker, string msg)
{
	if (paused || ! objectp(listener) || ! objectp(speaker) ||
	    listener == speaker || ! interactive(speaker) ||
	    environment(listener) != environment(speaker))
		return;

	remember_interaction(listener, speaker, "say");
	enqueue_event(listener, "say", speaker, 0, 0, msg, 70);
}

void hear_ask(object listener, object asker, string topic)
{
	if (paused || ! objectp(listener) || ! objectp(asker) ||
	    ! listener->query("ai_player") || ! interactive(asker) ||
	    environment(listener) != environment(asker))
		return;
	remember_interaction(listener, asker, topic);
	enqueue_event(listener, "ask", asker, 0, 0, topic, 90);
}

void reply_to_say(object listener, object speaker)
{
	string *replies;
	string tier;

	if (! objectp(listener) || ! objectp(speaker) ||
	    environment(listener) != environment(speaker) ||
	    listener->is_busy() || listener->is_fighting())
		return;

	tier = relation_tier(listener, speaker->query("id"));
	if (tier == "friend")
		replies = ({
			"你这话我听进去了，回头再细谈。",
			"还是你看得仔细，我也正有这个想法。",
		});
	else if (tier == "familiar")
		replies = ({
			"有道理，我记下了。",
			"你这么一说，我倒想起些线索。",
		});
	else
		replies = ({
			"这话倒也不能说错。",
			"嗯，我再想想。",
			"江湖上的事，很难只看一面。",
		});
	set_goal(listener, "回应交谈", "回应附近玩家");
	remember_interaction(listener, speaker, "reply");
	run_action(listener, "say", replies[random(sizeof(replies))]);
}

void receive_message(object me, string msgclass, string msg)
{
	// Decisions use structured world state; UI text is intentionally ignored.
}

string *validate_profiles()
{
	string *issues;
	string *locations;
	string *route;
	string id;
	string period;
	string location;
	mapping profile;
	mapping schedule;
	mapping activity_config;
	mapping greeting_lines;
	mapping room_lines;
	mapping seen;
	string action;
	string partner;
	string *stops;
	int i;

	issues = ({});
	foreach (id in keys(profiles))
	{
		profile = profiles[id];
		if (! valid_room_file(profile["home"], profile["zone"]))
			issues += ({ id + ": invalid home " + profile["home"] });
		locations = ({});
		route = profile["route"];
		if (arrayp(route))
			locations += route;
		greeting_lines = profile["greeting_lines"];
		room_lines = profile["room_lines"];
		if (! mapp(greeting_lines) || sizeof(keys(greeting_lines)) != 4)
			issues += ({ id + ": incomplete greeting context" });
		if (! mapp(room_lines) || ! sizeof(keys(room_lines)))
			issues += ({ id + ": missing room context" });
		if (arrayp(profile["activities"]))
			foreach (activity_config in profile["activities"])
				if (mapp(activity_config))
				{
					action = activity_config["action"];
					if (member_array(action, ({ "observe", "reflect", "supplies",
					    "patrol", "rest", "social" })) == -1)
						issues += ({ id + ": unsupported activity action " + action });
					if (stringp(activity_config["target"]))
						locations += ({ activity_config["target"] });
					stops = activity_config["stops"];
					if (arrayp(stops))
						locations += stops;
					partner = activity_config["partner"];
					if (action == "social" &&
					    (! stringp(partner) || ! mapp(profiles[partner])))
						issues += ({ id + ": invalid social partner" });
				}
		schedule = profile["schedule"];
		foreach (period in ({ "morning", "day", "evening", "night" }))
		{
			if (! mapp(schedule) || ! arrayp(schedule[period]) ||
			    ! sizeof(schedule[period]))
			{
				issues += ({ id + ": missing schedule " + period });
				continue;
			}
			locations += schedule[period];
		}
		seen = ([]);
		for (i = 0; i < sizeof(locations); i++)
		{
			location = locations[i];
			if (seen[location])
				continue;
			seen[location] = 1;
			if (! valid_room_file(location, profile["zone"]))
			{
				issues += ({ id + ": invalid destination " + location });
				continue;
			}
			if (location != profile["home"] &&
			    ! stringp(find_route_step(0, profile["home"], location,
				profile["zone"])))
				issues += ({ id + ": unreachable from home " + location });
			if (location != profile["home"] &&
			    ! stringp(find_route_step(0, location, profile["home"],
				profile["zone"])))
				issues += ({ id + ": cannot return home from " + location });
		}
	}
	return issues;
}

mapping selftest_player(string id)
{
	mapping result;
	mapping profile;
	mapping relation;
	mapping old_cache;
	mapping activity;
	mixed *queue;
	mixed *activities;
	string *issues;
	string target;
	string key;
	string first;
	string second;
	int i;
	int patrols;
	int rests;
	int social;

	result = ([ "id" : id, "passed" : 0, "checks" : ([]),
		"issues" : ({}) ]);
	if (! mapp(profile = profiles[id]))
	{
		result["issues"] = ({ "unknown AI player" });
		return result;
	}
	issues = validate_profiles();
	foreach (key in issues)
		if (strsrch(key, id + ":") == 0)
			result["issues"] += ({ key });
	result["checks"]["schedule_and_routes"] =
		! sizeof(result["issues"]);
	activities = profile["activities"];
	result["checks"]["activities"] = arrayp(activities) &&
		sizeof(activities) >= 3;
	patrols = 0;
	rests = 0;
	social = 0;
	if (arrayp(activities))
		foreach (activity in activities)
		{
			if (activity["action"] == "patrol" &&
			    arrayp(activity["stops"]) && sizeof(activity["stops"]) >= 2)
				patrols++;
			else if (activity["action"] == "rest" && activity["duration"] > 0)
				rests++;
			else if (activity["action"] == "social" &&
				 mapp(profiles[activity["partner"]]))
				social++;
		}
	result["checks"]["contextual_dialogue"] =
		mapp(profile["greeting_lines"]) &&
		sizeof(keys(profile["greeting_lines"])) == 4 &&
		mapp(profile["room_lines"]) && sizeof(keys(profile["room_lines"]));
	result["checks"]["patrol_activity"] = patrols > 0;
	result["checks"]["rest_activity"] = rests > 0;
	result["checks"]["social_activity"] =
		(id == "ai_qingfeng" || id == "ai_wantang") ? social > 0 : 1;
	queue = ({});
	for (i = 0; i < AI_MAX_EVENTS; i++)
		queue += ({ ([ "priority" : 20, "type" : "test" ]) });
	result["checks"]["queue_priority"] =
		event_drop_index(queue, 90) == 0 &&
		event_drop_index(queue, 20) == -1;
	relation = ([ "familiarity" : 0, "daily_gain" : 0 ]);
	for (i = 0; i < AI_RELATION_DAILY_GAIN + 2; i++)
		relation = gain_familiarity(relation, 12345);
	result["checks"]["relation_daily_cap"] =
		relation["familiarity"] == AI_RELATION_DAILY_GAIN &&
		relation["daily_gain"] == AI_RELATION_DAILY_GAIN;
	if (arrayp(activities) && sizeof(activities))
	{
		activity = activities[0];
		target = activity["target"];
		if (target == profile["home"] && sizeof(activities) > 1)
			target = activities[1]["target"];
		key = profile["home"] + "|" + target;
		old_cache = route_cache[key];
		first = find_route_step(0, profile["home"], target,
			profile["zone"]);
		second = find_route_step(0, profile["home"], target,
			profile["zone"]);
		result["checks"]["route_cache"] = stringp(first) &&
			first == second && mapp(route_cache[key]);
		if (mapp(old_cache))
			route_cache[key] = old_cache;
		else
			map_delete(route_cache, key);
	}
	else
		result["checks"]["route_cache"] = 0;
	foreach (key in keys(result["checks"]))
		if (! result["checks"][key])
			result["issues"] += ({ key + " check failed" });
	result["passed"] = ! sizeof(result["issues"]);
	return result;
}

void save_all_players()
{
	object me;

	foreach (me in query_ai_players())
		if (objectp(me))
			catch(me->save());
}

void mud_shutdown()
{
	save_all_players();
}

void remove()
{
	save_all_players();
}

private void process_player(string id)
{
	object me;
	mapping profile;
	string error;

	profile = profiles[id];
	me = actors[id];
	if (! objectp(me))
	{
		if (intp(next_load_attempt[id]) && next_load_attempt[id] > time())
			return;
		next_load_attempt[id] = time() + 30;
		actors[id] = load_player(id, profile);
		if (objectp(actors[id]))
			map_delete(next_load_attempt, id);
		return;
	}
	if (! objectp(environment(me)))
	{
		me->move(profile["home"]);
		remember_room(me);
	}
	if (me->is_ghost())
		respawn_player(me, profile);

	if (! paused && me->query_temp("ai/next_survival") <= time())
	{
		update_survival(me);
		me->set_temp("ai/next_survival",
			time() + AI_SURVIVAL_INTERVAL);
	}
	if (! paused)
		perceive_world(me, profile);
	if (me->query_temp("ai/next_save") <= time())
	{
		error = catch(me->save());
		if (stringp(error) && error != "")
		{
			add_metric(id, "errors", 1);
			log_file("ai-player", sprintf("%s save %s: %s\n",
				ctime(time()), id, error));
		}
		me->set_temp("ai/next_save", time() + AI_SAVE_INTERVAL);
	}
	if (! paused && me->query_temp("ai/next_action") <= time())
		think(me, profile);
}

protected void heart_beat()
{
	string id;
	string error;

	foreach (id in keys(profiles))
	{
		error = catch(process_player(id));
		if (stringp(error) && error != "")
		{
			add_metric(id, "errors", 1);
			log_file("ai-player", sprintf("%s %s: %s\n",
				ctime(time()), id, error));
		}
	}
}
