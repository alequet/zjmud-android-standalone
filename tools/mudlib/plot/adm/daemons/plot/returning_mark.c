// Chapter two: Old Mark, New Trace.

#include <ansi.h>

#define PLOT_ID "returning_mark_02"
#define ROOT "plot/returning_mark_02"
#define ARC "hometown_letters_01"
#define STOREHOUSE "/d/plot/returning_mark/old_storehouse"
#define MARK_CHAMBER "/d/plot/returning_mark/mark_chamber"
#define MENG_SI "/clone/plot/returning_mark/meng_si"
#define RUBBING "/clone/plot/returning_mark/wave_mark_rubbing"

string render_log(object me);
void instance_left(object me);
private string reconstruct_route(object me);

private string stage(object me)
{
	return me->query(ROOT + "/stage");
}

private string status(object me)
{
	return me->query(ROOT + "/status");
}

private string branch(object me)
{
	string value;

	value = me->query(ROOT + "/flags/branch");
	return stringp(value) ? value : "compat";
}

private string branch_label(string value)
{
	if (value == "hall") return "会馆公开";
	if (value == "yamen") return "衙门官断";
	if (value == "grain") return "粮行私了";
	return "杜宽兼容入口";
}

private int in_instance(object me)
{
	string room;

	if (! objectp(environment(me))) return 0;
	room = base_name(environment(me));
	return environment(me)->query("plot_owner") == me->query("id") &&
		(room == STOREHOUSE || room == MARK_CHAMBER);
}

private object query_storehouse(object me)
{
	return PLOT_D->query_instance(me, PLOT_ID);
}

private object query_mark_chamber(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "mark_chamber");
}

private object create_storehouse(object me)
{
	object room;
	object meng;
	object qing;
	string meng_condition;

	room = query_storehouse(me);
	if (! objectp(room))
	{
		room = PLOT_D->open_instance(me, PLOT_ID, STOREHOUSE, "/d/city/beimen");
		if (! objectp(room)) return 0;
	}
	meng = present("meng si", room);
	if (objectp(meng))
		room->set("plot_spawned/meng_si", 1);
	else if (! room->query("plot_spawned/meng_si"))
	{
		meng = new(MENG_SI);
		meng->set("plot_owner", me->query("id"));
		meng->move(room);
		room->set("plot_spawned/meng_si", 1);
	}
	meng_condition = PLOT_D->query_flag(me, PLOT_ID, "meng_injured") ? "injured" : "guarding";
	if (objectp(meng)) meng->restore_plot_condition(meng_condition);
	if (PLOT_D->query_flag(me, PLOT_ID, "full_token_recovered")) return room;
	qing = present("qing peng ke", room);
	if (objectp(qing) && living(qing))
	{
		room->set("plot_spawned/qing_peng_ke", 1);
		PLOT_COMBAT_D->restore_enemy(qing);
		return room;
	}
	if (PLOT_D->query_flag(me, PLOT_ID, "qing_resolved")) return room;
	if (room->query("plot_spawned/qing_peng_ke")) return room;
	qing = new("/clone/plot/returning_mark/qing_peng_ke");
	qing->set("plot_owner", me->query("id"));
	if (! mapp(PLOT_COMBAT_D->configure_enemy(qing, me, "green_hood", 0)))
	{
		destruct(qing);
		PLOT_D->close_instance(me, PLOT_ID);
		return 0;
	}
	qing->move(room);
	room->set("plot_spawned/qing_peng_ke", 1);
	return room;
}

private object create_mark_chamber(object me)
{
	object room;

	room = query_mark_chamber(me);
	if (objectp(room)) return room;
	room = PLOT_D->open_instance_room(me, PLOT_ID, "mark_chamber", MARK_CHAMBER);
	if (! objectp(room)) return 0;
	room->set("exits/out", "/d/city/beimen");
	return room;
}

private string enter_storehouse(object me)
{
	object room;

	room = create_storehouse(me);
	if (! objectp(room)) return "旧仓入口一时无法打开，请稍后再试。";
	me->move(room);
	return "你循着半张凭据背面的仓号，推开旧仓沉重的木门。";
}

private string enter_mark_chamber(object me)
{
	object room;

	room = create_mark_chamber(me);
	if (! objectp(room)) return "旧仓后室暂时无法打开，请稍后再试。";
	me->move(room);
	return "你绕过堆满空箱的偏廊，来到旧仓后室的潮印案台。";
}

private int sync_first_chapter(object me)
{
	string choice;
	int value;
	string key;

	choice = me->query("plot/origin_letter_01/choices/ledger_custodian");
	if (! stringp(choice) || choice == "") choice = "compat";
	if (choice != "hall" && choice != "yamen" && choice != "grain") choice = "compat";
	if (! stringp(me->query(ROOT + "/flags/branch")))
	{
		PLOT_D->set_flag(me, PLOT_ID, "branch", choice);
		PLOT_D->set_flag(me, PLOT_ID, "branch_contact", choice);
	}
	foreach (key in ({ "hometown_hall", "yamen", "grain_house" }))
	{
		value = me->query("plot/origin_letter_01/relations/" + key);
		if (! intp(value)) value = 0;
		PLOT_D->set_arc_relation(me, key, value);
	}
	return 1;
}

private string begin_branch(object me)
{
	if (status(me) == "available")
	{
		if (! PLOT_D->begin_chapter(me, PLOT_ID)) return "第二章暂时无法接取，请稍后再试。";
	}
	if (status(me) != "active" || stage(me) != "branch_hook") return render_log(me);
	sync_first_chapter(me);
	PLOT_D->set_flag(me, PLOT_ID, "branch_hook_done", 1);
	PLOT_D->set_flag(me, PLOT_ID, "record_" + branch(me), 1);
	PLOT_D->set_flag(me, PLOT_ID, "records_seen", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "branch_hook", "compare_marks");
	return ZJOBLONG "旧印新痕\n\n"
		"你在第一章选择了" + branch_label(branch(me)) + "。这条路留下的后果先找上门来，但同一枚首尾相接的水纹印，也在另外两处物证上出现。\n\n"
		"会馆、衙门和粮行各自只保留了回浪印的一部分。先比对至少两份记录，再去找印记背后的旧仓。\n\n" + render_log(me);
}

private string inspect_record(object me, string source)
{
	string key;
	string text;
	int count;

	if (stage(me) != "compare_marks") return render_log(me);
	if (source == "hall")
	{
		key = "record_hall";
		text = "会馆保存的是收货人和作保人姓名，回浪印落在纸角，潮纹从左向右收尾。";
	}
	else if (source == "yamen")
	{
		key = "record_yamen";
		text = "衙门卷宗保存的是放行日期，缺页边缘仍有半枚水纹，最后一笔向下压。";
	}
	else if (source == "grain")
	{
		key = "record_grain";
		text = "粮行货单保存的是船号，印泥混着细盐，说明凭据曾在水边旧仓重新盖过。";
	}
	else return "没有这份记录。";
	PLOT_D->set_flag(me, PLOT_ID, key, 1);
	count = (PLOT_D->query_flag(me, PLOT_ID, "record_hall") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "record_yamen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "record_grain") ? 1 : 0);
	PLOT_D->set_flag(me, PLOT_ID, "records_seen", count);
	if (count >= 2)
	{
		PLOT_D->set_flag(me, PLOT_ID, "returning_mark_known", 1);
		PLOT_D->set_arc_flag(me, "returning_mark_known", 1);
		PLOT_D->advance_stage(me, PLOT_ID, "compare_marks", "find_old_storehouse");
	}
	return ZJOBLONG + text + "\n\n" + render_log(me);
}

private string find_old_storehouse(object me)
{
	string here;

	if (stage(me) != "find_old_storehouse") return render_log(me);
	here = base_name(environment(me));
	if (here != "/d/city/beimen" && here != "/d/city/postofficer")
		return "旧仓在扬州北门外。先回到北门，或向杜宽询问仓号。";
	PLOT_D->set_flag(me, PLOT_ID, "safe_return", here);
	if (! PLOT_D->advance_stage(me, PLOT_ID, "find_old_storehouse", "protect_meng_si"))
		return "旧仓线索暂时无法确认。";
	return enter_storehouse(me);
}

void record_qing_death(object qing)
{
	object me;
	object room;
	string owner;

	if (! objectp(qing) || base_name(qing) != "/clone/plot/returning_mark/qing_peng_ke") return;
	room = environment(qing);
	owner = qing->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || ! objectp(room) || environment(me) != room ||
		room->query("plot_owner") != owner || stage(me) != "protect_meng_si") return;
	PLOT_D->set_flag(me, PLOT_ID, "qing_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "qing_fate", "killed");
	PLOT_D->set_flag(me, PLOT_ID, "full_token_recovered", 1);
}

void record_meng_injury(object meng)
{
	object me;
	string owner;

	if (! objectp(meng) || base_name(meng) != MENG_SI) return;
	owner = meng->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || environment(me) != environment(meng) ||
		stage(me) != "protect_meng_si") return;
	PLOT_D->set_flag(me, PLOT_ID, "meng_injured", 1);
}

private string resolve_qing(object me, string method)
{
	object qing;
	object room;

	if (stage(me) != "protect_meng_si" || ! in_instance(me)) return "你必须先在旧仓现场找到孟四。";
	room = environment(me);
	qing = present("qing peng ke", room);
	if (method == "fight")
	{
		if (! objectp(qing) || ! living(qing)) return reconstruct_route(me);
		me->kill_ob(qing);
		qing->kill_ob(me);
		return "青篷客翻过箱垛，已经把退路让给了你。击退他后，再检查孟四手里的半张凭据。";
	}
	if (method == "intercept")
	{
		if (! objectp(qing)) return "青篷客已经不在仓里。";
		if (! PLOT_D->query_flag(me, PLOT_ID, "door_closed"))
			return "仓门仍然敞着。青篷客一旦察觉不对就能翻门逃走，先把门闩落下。";
		PLOT_D->set_flag(me, PLOT_ID, "qing_resolved", 1);
		PLOT_D->set_flag(me, PLOT_ID, "qing_fate", "intercepted");
		PLOT_D->set_flag(me, PLOT_ID, "full_token_recovered", 1);
		destruct(qing);
		return reconstruct_route(me);
	}
	if (method == "persuade")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "returning_mark_known"))
			return "你还没有拿出足够的两份印记记录，青篷客不会相信雇主已经放弃这张旧票。";
		if (! PLOT_D->query_flag(me, PLOT_ID, "half_token_shown"))
			return "青篷客只看见你口说无凭。先把已知的半张回浪凭据摊给他看。";
		PLOT_D->set_flag(me, PLOT_ID, "qing_resolved", 1);
		PLOT_D->set_flag(me, PLOT_ID, "qing_fate", "persuaded");
		PLOT_D->set_flag(me, PLOT_ID, "full_token_recovered", 1);
		if (objectp(qing)) destruct(qing);
		return reconstruct_route(me);
	}
	return "没有这个处置办法。";
}

private string reconstruct_route(object me)
{
	object room;

	if (stage(me) == "protect_meng_si")
	{
		PLOT_D->advance_stage(me, PLOT_ID, "protect_meng_si", "reconstruct_route");
		room = create_mark_chamber(me);
		if (objectp(room)) me->move(room);
	}
	if (stage(me) != "reconstruct_route") return render_log(me);
	return "孟四把另一半凭据压在潮印案台上。潮次、船号和仓号各只能使用一次，先看清选项，再确认水路。\n\n" + render_log(me);
}

private string choose_route(object me, string kind, string value)
{
	if (stage(me) != "reconstruct_route") return render_log(me);
	if (kind != "tide" && kind != "ship" && kind != "warehouse") return "没有这个路线字段。";
	PLOT_D->set_pending_choice(me, PLOT_ID, "route_" + kind, value);
	return "你把" + (kind == "tide" ? "潮次" : (kind == "ship" ? "船号" : "仓号")) +
		"记为「" + value + "」。三项都选好后再确认，错配不会销毁凭据。\n";
}

private string confirm_route(object me)
{
	string tide;
	string ship;
	string warehouse;

	if (stage(me) != "reconstruct_route") return render_log(me);
	tide = me->query(ROOT + "/choices/route_tide");
	ship = me->query(ROOT + "/choices/route_ship");
	warehouse = me->query(ROOT + "/choices/route_warehouse");
	if (! stringp(tide) || ! stringp(ship) || ! stringp(warehouse))
		return "潮次、船号和仓号还没有选齐。";
	if (tide != "2" || ship != "23" || warehouse != "7")
		return "双半印在这里断开：这组潮次、船号和仓号无法同时解释第一章多出的七袋粮。请保留已知记录，换一项重排。";
	PLOT_D->set_flag(me, PLOT_ID, "route_reconstructed", 1);
	PLOT_D->set_flag(me, PLOT_ID, "green_reed_ferry_known", 1);
	PLOT_D->set_arc_flag(me, "green_reed_ferry_known", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "reconstruct_route", "chapter_close");
	return "双半印首尾相接，指向「青芦渡」。你已经知道这不是三桩孤立的旧案，而是一条水路网络。\n\n" + render_log(me);
}

private string close_chapter(object me)
{
	object rubbing;

	if (stage(me) != "chapter_close") return render_log(me);
	if (in_instance(me))
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/beimen";
		me->move(safe);
	}
	if (! PLOT_D->complete_chapter(me, PLOT_ID, "chapter_close"))
		return "回浪印已经拼合，但章节完成状态暂时无法保存，请再试一次。";
	if (! PLOT_D->query_flag(me, PLOT_ID, "reward_claimed"))
	{
		if (present("wave mark rubbing", me))
			PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
		else
		{
			rubbing = new(RUBBING);
			if (objectp(rubbing) && rubbing->move(me, 1))
				PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
			else if (objectp(rubbing)) destruct(rubbing);
		}
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "score_claimed"))
	{
		me->add("score", 12);
		PLOT_D->set_flag(me, PLOT_ID, "score_claimed", 1);
	}
	PLOT_D->set_arc_value(me, "current_chapter", 3);
	instance_left(me);
	return ZJOBLONG "你把三份记录摊在一起，向会馆、衙门和粮行说明：他们各自遇到的麻烦，都来自同一套被拆开的回浪凭据。\n\n"
		"第二章「旧印新痕」完成。旧驿木牌上多了一枚印记拓样，水路尽头指向青芦渡。你获得少量阅历，但没有得到任何强力装备。\n";
}

private string claim_rubbing(object me)
{
	object rubbing;

	if (status(me) != "completed") return "第二章尚未完成。";
	if (present("wave mark rubbing", me)) return "回浪印拓样还在你身上。";
	rubbing = new(RUBBING);
	if (! objectp(rubbing) || ! rubbing->move(me, 1))
	{
		if (objectp(rubbing)) destruct(rubbing);
		return "你身上没有空位，清出一格后再补领回浪印拓样。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	return "杜宽替你从旧件箱中补出一张回浪印拓样。";
}

void instance_left(object me)
{
	object room;
	object chamber;
	object ob;

	if (! objectp(me)) return;
	room = query_storehouse(me);
	chamber = query_mark_chamber(me);
	if (objectp(environment(me)) &&
		(base_name(environment(me)) == STOREHOUSE || base_name(environment(me)) == MARK_CHAMBER)) return;
	foreach (ob in (objectp(room) ? all_inventory(room) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(chamber) ? all_inventory(chamber) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	PLOT_D->close_instance_rooms(me, PLOT_ID);
	PLOT_D->close_instance(me, PLOT_ID);
}

string render_log(object me)
{
	string current;
	int count;

	PLOT_D->ensure_player_state(me);
	current = stage(me);
	count = PLOT_D->query_flag(me, PLOT_ID, "records_seen");
	if (status(me) == "available")
		return "杜宽、许三娘、谭友纪或周守义那里，都有一封与你第一章选择有关的无署名补信。\n";
	if (status(me) == "completed")
		return "回浪印已知。青芦渡已记入乡书篇线索，回浪印拓样可向杜宽补领。\n" +
			"补领：plot act returning_mark_02 claim_rubbing\n";
	if (current == "compare_marks")
		return sprintf("已比对记录：%d/3。至少再查一份，第三份只提供额外细节。\n%s", count,
			"可查：plot act returning_mark_02 inspect_hall / inspect_yamen / inspect_grain\n");
	if (current == "find_old_storehouse")
		return "两份印记已经对上。目标：到扬州北门，根据凭据背面的仓号找到旧仓。\n";
	if (current == "protect_meng_si")
		return "孟四正在旧仓里护住另一半凭据。可正面战斗；也可先落下门闩再截断出口；或先展示半印再说服青篷客。\n";
	if (current == "reconstruct_route")
		return "双半印谜题：潮次可选 2/4，船号可选 23/17，仓号可选 7/4。确认组合后前往青芦渡。\n";
	return "目标：向三方说明回浪印的共同来处。\n";
}

string npc_notice(object me)
{
	if (! objectp(me)) return "";
	if (status(me) == "available" || status(me) == "active") return begin_branch(me);
	return render_log(me);
}

mixed handle_action(object me, string action)
{
	string value;

	if (! objectp(me) || ! userp(me)) return "无效的剧情参与者。";
	PLOT_D->ensure_player_state(me);
	if (action == "branch_hook" || action == "begin") return begin_branch(me);
	if (action == "inspect_hall") return inspect_record(me, "hall");
	if (action == "inspect_yamen") return inspect_record(me, "yamen");
	if (action == "inspect_grain") return inspect_record(me, "grain");
	if (action == "hint_storehouse") return "背面仓号属于扬州北门外旧水路仓，不在常驻地图上。";
	if (action == "find_old_storehouse") return find_old_storehouse(me);
	if (action == "enter")
	{
		if (stage(me) == "protect_meng_si") return enter_storehouse(me);
		if (stage(me) == "reconstruct_route") return enter_mark_chamber(me);
		return render_log(me);
	}
	if (action == "fight_qing") return resolve_qing(me, "fight");
	if (action == "close_door")
	{
		if (stage(me) != "protect_meng_si" || ! in_instance(me)) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "door_closed", 1);
		return "你把旧仓木门合上，落下生锈的门闩。青篷客的退路已经被截断。";
	}
	if (action == "show_half_token")
	{
		if (stage(me) != "protect_meng_si" || ! in_instance(me)) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "half_token_shown", 1);
		return "你把两份记录对应的半枚回浪印摊开，指出这张旧票已经失去替雇主保密的价值。";
	}
	if (action == "intercept_qing") return resolve_qing(me, "intercept");
	if (action == "persuade_qing") return resolve_qing(me, "persuade");
	if (sscanf(action, "route_tide_%s", value) == 1) return choose_route(me, "tide", value);
	if (sscanf(action, "route_ship_%s", value) == 1) return choose_route(me, "ship", value);
	if (sscanf(action, "route_warehouse_%s", value) == 1) return choose_route(me, "warehouse", value);
	if (action == "route_hint") return "提示：会馆收货日排除早潮，衙门放行页的尾笔与 23 号船签相合，仓号 7 的旧印边缘正好缺一笔。\n";
	if (action == "confirm_route") return confirm_route(me);
	if (action == "chapter_close" || action == "aftermath") return close_chapter(me);
	if (action == "claim_rubbing") return claim_rubbing(me);
	if (action == "leave")
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/beimen";
		if (in_instance(me)) me->move(safe);
		instance_left(me);
		return "你离开旧仓，回到安全处。";
	}
	return "这项剧情操作不存在。";
}
