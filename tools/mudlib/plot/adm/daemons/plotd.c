// Persistent chapter plot coordinator for the Android standalone runtime.

#include <ansi.h>

inherit F_DBASE;

#define PLOT_SCHEMA_VERSION 2
#define HOMETOWN_ARC "hometown_letters_01"

private mapping chapters = ([
	"origin_letter_01" : ([
		"name" : "一纸乡书",
		"chapter" : 1,
		"controller" : "/adm/daemons/plot/origin_letter",
		"initial_stage" : "notice",
		"final_stage" : "aftermath",
		"stages" : ({ "notice", "accept_letter", "hear_three_sides",
			"trace_warehouse", "face_pei_jiu", "inspect_ledger",
			"protect_evidence", "choose_custodian", "aftermath" }),
		"implemented" : 1,
	]),
	"returning_mark_02" : ([
		"name" : "旧印新痕",
		"chapter" : 2,
		"controller" : "/adm/daemons/plot/returning_mark",
		"initial_stage" : "branch_hook",
		"final_stage" : "chapter_close",
		"stages" : ({ "branch_hook", "compare_marks", "find_old_storehouse",
			"protect_meng_si", "reconstruct_route", "chapter_close" }),
		"prerequisite" : "origin_letter_01",
		"implemented" : 1,
	]),
	"silent_crossing_03" : ([
		"name" : "夜渡无声",
		"chapter" : 3,
		"controller" : "/adm/daemons/plot/silent_crossing",
		"initial_stage" : "watch_green_reed_ferry",
		"final_stage" : "chapter_close",
		"stages" : ({ "watch_green_reed_ferry", "board_boat",
			"reach_cargo_hold", "save_a_he", "inspect_three_cargos",
			"stop_burning_records", "settle_the_boat", "chapter_close" }),
		"prerequisite" : "returning_mark_02",
		"implemented" : 1,
	]),
	"visitors_from_home_04" : ([
		"name" : "故园来客",
		"chapter" : 4,
		"controller" : "/adm/daemons/plot/visitors_from_home",
		"initial_stage" : "letters_arrive",
		"final_stage" : "chapter_close",
		"stages" : ({ "letters_arrive", "meet_lu_xiaoshuan",
			"verify_letters", "choose_immediate_action", "catch_hua_yaozi",
			"decode_invitation", "prepare_for_meeting", "chapter_close" }),
		"prerequisite" : "silent_crossing_03",
		"implemented" : 1,
	]),
	"where_rivers_end_05" : ([
		"name" : "百川归处",
		"chapter" : 5,
		"controller" : "/adm/daemons/plot/where_rivers_end",
		"initial_stage" : "enter_salt_storehouse",
		"final_stage" : "arc_aftermath",
		"stages" : ({ "enter_salt_storehouse", "hear_the_assembly",
			"rescue_real_lu", "break_single_control", "save_people_and_records",
			"separate_two_hands", "confront_wen_shouzhuo",
			"choose_archive_disposition", "arc_aftermath" }),
		"prerequisite" : "visitors_from_home_04",
		"implemented" : 1,
	]),
]);

void create()
{
	seteuid(ROOT_UID);
	set("channel_id", "剧情精灵");
}

private mapping copy_flat_mapping(mapping source)
{
	mapping result;
	mixed key;

	result = ([]);
	if (! mapp(source))
		return result;

	foreach (key in keys(source))
		result[key] = source[key];
	return result;
}

private int valid_plot_id(string plot_id)
{
	return stringp(plot_id) && mapp(chapters[plot_id]);
}

private string chapter_root(string plot_id)
{
	return "plot/" + plot_id;
}

private string safe_key(string key)
{
	if (! stringp(key) || key == "" ||
		strsrch(key, "/") >= 0 || strsrch(key, "\\") >= 0)
		return 0;
	return key;
}

private int completed(object me, string plot_id)
{
	return objectp(me) &&
		me->query(chapter_root(plot_id) + "/status") == "completed";
}

private int valid_stage(string plot_id, mixed stage)
{
	return valid_plot_id(plot_id) && stringp(stage) &&
		member_array(stage, chapters[plot_id]["stages"]) >= 0;
}

private int valid_status(mixed status)
{
	return stringp(status) &&
		member_array(status, ({ "locked", "available", "active", "completed" })) >= 0;
}

private int caller_is_controller(string plot_id, object caller)
{
	string expected;

	if (! valid_plot_id(plot_id))
		return 0;
	expected = chapters[plot_id]["controller"];
	return objectp(caller) && base_name(caller) == expected;
}

private int caller_is_registered_controller(object caller)
{
	string plot_id;

	if (! objectp(caller))
		return 0;
	foreach (plot_id in keys(chapters))
		if (base_name(caller) == chapters[plot_id]["controller"])
			return 1;
	return 0;
}

private int save_player(object me)
{
	mixed error;

	if (! objectp(me) || ! userp(me))
		return 0;
	error = catch(me->save());
	if (error)
	{
		log_file("plot-error", sprintf("Unable to save %s: %O\n",
			me->query("id"), error));
		return 0;
	}
	return 1;
}

private string derived_status(object me, string plot_id)
{
	string prerequisite;
	string startroom;

	if (! chapters[plot_id]["implemented"])
		return "locked";
	/* Historical world characters can predate the birth-field schema. */
	startroom = me->query("startroom");
	if (! me->query("born") &&
		(! me->query("registered") || ! stringp(startroom) || startroom == "" ||
		 strsrch(startroom, "/d/death/") == 0))
		return "locked";
	prerequisite = chapters[plot_id]["prerequisite"];
	if (stringp(prerequisite) && ! completed(me, prerequisite))
		return "locked";
	return "available";
}

private int ensure_arc(object me)
{
	string root;
	string status;
	string expected_status;
	int changed;
	int expected_chapter;
	mixed version;

	root = "plot/arc/" + HOMETOWN_ARC;
	version = me->query(root + "/version");
	if (intp(version) && version > PLOT_SCHEMA_VERSION)
		return -1;

	changed = 0;
	if (! intp(version) || version < PLOT_SCHEMA_VERSION)
	{
		me->set(root + "/version", PLOT_SCHEMA_VERSION);
		changed = 1;
	}
	expected_status = completed(me, "where_rivers_end_05") ? "completed" : "available";
	expected_chapter = 1;
	if (completed(me, "origin_letter_01")) expected_chapter = 2;
	if (completed(me, "returning_mark_02")) expected_chapter = 3;
	if (completed(me, "silent_crossing_03")) expected_chapter = 4;
	if (completed(me, "visitors_from_home_04")) expected_chapter = 5;
	status = me->query(root + "/status");
	if (status != expected_status)
	{
		me->set(root + "/status", expected_status);
		changed = 1;
	}
	if (me->query(root + "/current_chapter") != expected_chapter)
	{
		me->set(root + "/current_chapter", expected_chapter);
		changed = 1;
	}
	return changed;
}

private void reset_chapter_state(object me, string plot_id)
{
	string root;

	root = chapter_root(plot_id);
	me->delete(root);
	me->set(root + "/version", PLOT_SCHEMA_VERSION);
	me->set(root + "/status", derived_status(me, plot_id));
	me->set(root + "/stage", chapters[plot_id]["initial_stage"]);
}

private int compatible_player_state(object me)
{
	string plot_id;
	mixed version;

	version = me->query("plot/arc/" + HOMETOWN_ARC + "/version");
	if (intp(version) && version > PLOT_SCHEMA_VERSION)
		return 0;
	foreach (plot_id in keys(chapters))
	{
		version = me->query(chapter_root(plot_id) + "/version");
		if (intp(version) && version > PLOT_SCHEMA_VERSION)
			return 0;
	}
	return 1;
}

private int ensure_chapter(object me, string plot_id)
{
	string root;
	string status;
	string current_stage;
	int changed;
	mixed version;

	root = chapter_root(plot_id);
	version = me->query(root + "/version");
	if (intp(version) && version > PLOT_SCHEMA_VERSION)
		return -1;
	status = me->query(root + "/status");
	current_stage = me->query(root + "/stage");
	if (! intp(version) || version < 1)
	{
		if (status == "completed" && intp(me->query(root + "/completed_at")) &&
			me->query(root + "/completed_at") > 0)
		{
			me->set(root + "/version", PLOT_SCHEMA_VERSION);
			if (! valid_stage(plot_id, current_stage))
				me->set(root + "/stage", chapters[plot_id]["final_stage"]);
			return 1;
		}
		reset_chapter_state(me, plot_id);
		return 1;
	}

	changed = 0;
	if (version < PLOT_SCHEMA_VERSION)
	{
		me->set(root + "/version", PLOT_SCHEMA_VERSION);
		changed = 1;
	}
	if (! valid_status(status))
	{
		reset_chapter_state(me, plot_id);
		return 1;
	}
	if (! valid_stage(plot_id, current_stage))
	{
		if (status == "completed")
			me->set(root + "/stage", chapters[plot_id]["final_stage"]);
		else
			reset_chapter_state(me, plot_id);
		return 1;
	}
	if ((status == "locked" || status == "available") &&
		current_stage != chapters[plot_id]["initial_stage"])
	{
		reset_chapter_state(me, plot_id);
		return 1;
	}
	if ((status == "locked" || status == "available") &&
		status != derived_status(me, plot_id))
	{
		me->set(root + "/status", derived_status(me, plot_id));
		changed = 1;
	}
	return changed;
}

int ensure_player_state(object me)
{
	string plot_id;
	int result;
	int changed;

	if (! objectp(me) || ! userp(me))
		return 0;
	if (! compatible_player_state(me))
		return 0;
	result = ensure_arc(me);
	if (result < 0)
		return 0;
	changed = result;
	foreach (plot_id in keys(chapters))
	{
		result = ensure_chapter(me, plot_id);
		if (result < 0)
			return 0;
		changed += result;
	}
	if (changed)
		save_player(me);
	return 1;
}

mapping query_chapter(string plot_id)
{
	if (! valid_plot_id(plot_id))
		return 0;
	return copy_flat_mapping(chapters[plot_id]);
}

string *query_plot_ids()
{
	return sort_array(keys(chapters), (: chapters[$1]["chapter"] - chapters[$2]["chapter"] :));
}

mapping query_player_chapter(object me, string plot_id)
{
	mapping result;
	string root;

	if (! objectp(me) || ! valid_plot_id(plot_id))
		return 0;
	result = copy_flat_mapping(chapters[plot_id]);
	if (! ensure_player_state(me))
	{
		result["status"] = "incompatible";
		result["stage"] = "";
		return result;
	}
	root = chapter_root(plot_id);
	result["status"] = me->query(root + "/status");
	result["stage"] = me->query(root + "/stage");
	result["updated_at"] = me->query(root + "/updated_at");
	result["completed_at"] = me->query(root + "/completed_at");
	return result;
}

int begin_chapter(object me, string plot_id)
{
	string root;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "available")
		return 0;

	me->set(root + "/status", "active");
	me->set(root + "/started_at", time());
	me->set(root + "/updated_at", time());
	return save_player(me);
}

int advance_stage(object me, string plot_id, string expected, string next)
{
	string root;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! valid_stage(plot_id, expected) || ! valid_stage(plot_id, next))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "active" ||
		me->query(root + "/stage") != expected)
		return 0;

	me->set(root + "/stage", next);
	me->set(root + "/updated_at", time());
	return save_player(me);
}

int record_choice(object me, string plot_id, string key, string value)
{
	string path;
	string root;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! stringp(safe_key(key)) || ! stringp(value) || value == "")
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "active")
		return 0;
	path = root + "/choices/" + key;
	if (stringp(me->query(path)))
		return me->query(path) == value;

	me->set(path, value);
	me->set(root + "/updated_at", time());
	return save_player(me);
}

int set_pending_choice(object me, string plot_id, string key, string value)
{
	string root;
	string path;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! stringp(safe_key(key)) || ! stringp(value) || value == "")
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "active") return 0;
	path = root + "/choices/" + key;
	me->set(path, value);
	me->set(root + "/updated_at", time());
	return save_player(me);
}

int set_flag(object me, string plot_id, string key, mixed value)
{
	string root;
	string path;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "active" &&
		me->query(root + "/status") != "available" &&
		me->query(root + "/status") != "completed")
		return 0;
	path = root + "/flags/" + key;
	if (! undefinedp(me->query(path)) && me->query(path) == value)
		return 1;
	me->set(path, value);
	me->set(root + "/updated_at", time());
	return save_player(me);
}

mixed query_flag(object me, string plot_id, string key)
{
	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	return me->query(chapter_root(plot_id) + "/flags/" + key);
}

private object find_owned_instance(object me, string plot_id)
{
	string path;
	object room;

	if (! objectp(me) || ! valid_plot_id(plot_id))
		return 0;
	path = me->query_temp("plot/" + plot_id + "/instance");
	if (! stringp(path))
		return 0;
	room = find_object(path);
	if (! objectp(room) || room->query("plot_owner") != me->query("id"))
		return 0;
	return room;
}

object query_instance(object me, string plot_id)
{
	if (! caller_is_controller(plot_id, previous_object()))
		return 0;
	return find_owned_instance(me, plot_id);
}

private object find_owned_instance_room(object me, string plot_id, string slot)
{
	string path;
	object room;

	if (! objectp(me) || ! valid_plot_id(plot_id) || ! stringp(safe_key(slot))) return 0;
	path = me->query_temp("plot/" + plot_id + "/instance_rooms/" + slot);
	if (! stringp(path)) return 0;
	room = find_object(path);
	if (! objectp(room) || room->query("plot_owner") != me->query("id") ||
		room->query("plot_id") != plot_id)
		return 0;
	return room;
}

object query_instance_room(object me, string plot_id, string slot)
{
	if (! caller_is_controller(plot_id, previous_object())) return 0;
	return find_owned_instance_room(me, plot_id, slot);
}

object open_instance_room(object me, string plot_id, string slot, string blueprint)
{
	object room;

	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! caller_is_controller(plot_id, previous_object()) ||
		! stringp(safe_key(slot)) || ! stringp(blueprint) ||
		strsrch(blueprint, "/d/plot/") != 0 || file_size(blueprint + ".c") < 0)
		return 0;
	room = find_owned_instance_room(me, plot_id, slot);
	if (objectp(room)) return room;
	room = clone_object(blueprint);
	if (! objectp(room)) return 0;
	room->set("plot_owner", me->query("id"));
	room->set("plot_id", plot_id);
	room->set("no_save_location", 1);
	me->set_temp("plot/" + plot_id + "/instance_rooms/" + slot, file_name(room));
	return room;
}

int close_instance_rooms(object me, string plot_id)
{
	mapping rooms;
	object room;
	string slot;

	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! caller_is_controller(plot_id, previous_object())) return 0;
	rooms = me->query_temp("plot/" + plot_id + "/instance_rooms");
	if (! mapp(rooms)) return 1;
	foreach (slot in keys(rooms))
	{
		room = find_owned_instance_room(me, plot_id, slot);
		if (objectp(room) && sizeof(all_inventory(room)) > 0) return 0;
	}
	foreach (slot in keys(rooms))
	{
		room = find_owned_instance_room(me, plot_id, slot);
		if (objectp(room)) destruct(room);
	}
	me->delete_temp("plot/" + plot_id + "/instance_rooms");
	return 1;
}

object open_instance(object me, string plot_id, string blueprint, string return_room)
{
	object room;

	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! caller_is_controller(plot_id, previous_object()) ||
		! stringp(blueprint) || strsrch(blueprint, "/") != 0 ||
		! stringp(return_room) || strsrch(return_room, "/d/") != 0 ||
		file_size(blueprint + ".c") < 0 || file_size(return_room + ".c") < 0)
		return 0;
	room = find_owned_instance(me, plot_id);
	if (objectp(room))
		return room;
	room = clone_object(blueprint);
	if (! objectp(room))
		return 0;
	room->set("plot_owner", me->query("id"));
	room->set("plot_id", plot_id);
	room->set("no_save_location", 1);
	room->set("exits/out", return_room);
	me->set_temp("plot/" + plot_id + "/instance", file_name(room));
	return room;
}

int close_instance(object me, string plot_id)
{
	object room;

	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! caller_is_controller(plot_id, previous_object()))
		return 0;
	room = find_owned_instance(me, plot_id);
	if (objectp(room) && sizeof(all_inventory(room)) > 0)
		return 0;
	if (objectp(room))
		destruct(room);
	me->delete_temp("plot/" + plot_id + "/instance");
	return 1;
}

int set_relation(object me, string plot_id, string key, int value)
{
	string root;
	string path;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") != "active")
		return 0;
	path = root + "/relations/" + key;
	me->set(path, value);
	me->set(root + "/updated_at", time());
	return save_player(me);
}

int set_arc_value(object me, string key, mixed value)
{
	string root;
	string path;

	if (! objectp(me) || ! caller_is_registered_controller(previous_object()) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me)) return 0;
	root = "plot/arc/" + HOMETOWN_ARC;
	path = root + "/" + key;
	me->set(path, value);
	return save_player(me);
}

int set_arc_flag(object me, string key, mixed value)
{
	string root;
	string path;

	if (! objectp(me) || ! caller_is_registered_controller(previous_object()) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me)) return 0;
	root = "plot/arc/" + HOMETOWN_ARC;
	path = root + "/flags/" + key;
	me->set(path, value);
	return save_player(me);
}

int set_arc_choice(object me, string key, string value)
{
	string root;
	string path;
	mixed current;

	if (! objectp(me) || ! caller_is_registered_controller(previous_object()) ||
		! stringp(safe_key(key)) || ! stringp(value) || value == "")
		return 0;
	if (! ensure_player_state(me)) return 0;
	root = "plot/arc/" + HOMETOWN_ARC;
	path = root + "/choices/" + key;
	current = me->query(path);
	if (stringp(current)) return current == value;
	me->set(path, value);
	return save_player(me);
}

int set_arc_relation(object me, string key, int value)
{
	string root;
	string path;

	if (! objectp(me) || ! caller_is_registered_controller(previous_object()) ||
		! stringp(safe_key(key)))
		return 0;
	if (! ensure_player_state(me)) return 0;
	root = "plot/arc/" + HOMETOWN_ARC;
	path = root + "/relations/" + key;
	me->set(path, value);
	return save_player(me);
}

mixed dispatch_action(object me, string plot_id, string action)
{
	mapping chapter;
	mixed result;
	mixed error;

	if (! objectp(me) || ! valid_plot_id(plot_id) ||
		! stringp(action) || action == "" || strsrch(action, ";") >= 0 ||
		strsrch(action, "|") >= 0)
		return "无效的剧情操作。";
	if (! ensure_player_state(me))
		return "剧情存档来自更新版本，当前程序不能安全读取。";
	chapter = chapters[plot_id];
	if (! chapter["implemented"])
		return "本章内容尚未开放。";
	error = catch(result = call_other(chapter["controller"],
		"handle_action", me, action));
	if (error)
	{
		log_file("plot-error", sprintf("Action %s/%s failed for %s: %O\n",
			plot_id, action, me->query("id"), error));
		return "剧情暂时无法推进，请稍后重试。";
	}
	return result;
}

int complete_chapter(object me, string plot_id, string expected_stage)
{
	string root;

	if (! caller_is_controller(plot_id, previous_object()) || ! objectp(me) ||
		! valid_stage(plot_id, expected_stage))
		return 0;
	if (! ensure_player_state(me))
		return 0;
	root = chapter_root(plot_id);
	if (me->query(root + "/status") == "completed")
		return 1;
	if (me->query(root + "/status") != "active" ||
		me->query(root + "/stage") != expected_stage)
		return 0;

	me->set(root + "/status", "completed");
	me->set(root + "/completed_at", time());
	me->set(root + "/updated_at", time());
	return save_player(me);
}

mapping selftest()
{
	mapping result;
	string *ids;
	string plot_id;
	int expected;

	result = ([ "ok" : 1, "errors" : ({}) ]);
	ids = query_plot_ids();
	if (sizeof(ids) != 5)
		result["errors"] += ({ "chapter_count" });

	expected = 1;
	foreach (plot_id in ids)
	{
		mapping chapter;

		chapter = chapters[plot_id];
		if (chapter["chapter"] != expected)
			result["errors"] += ({ "chapter_order:" + plot_id });
		if (! stringp(chapter["controller"]) ||
			strsrch(chapter["controller"], "/adm/daemons/plot/") != 0)
			result["errors"] += ({ "controller:" + plot_id });
		if (! stringp(chapter["initial_stage"]) || chapter["initial_stage"] == "")
			result["errors"] += ({ "stage:" + plot_id });
		if (! pointerp(chapter["stages"]) || sizeof(chapter["stages"]) < 2 ||
			member_array(chapter["initial_stage"], chapter["stages"]) < 0 ||
			member_array(chapter["final_stage"], chapter["stages"]) < 0)
			result["errors"] += ({ "stage_registry:" + plot_id });
		expected++;
	}
	result["ok"] = sizeof(result["errors"]) == 0;
	result["schema"] = PLOT_SCHEMA_VERSION;
	result["arc"] = HOMETOWN_ARC;
	return result;
}
