// Chapter three: Silent Crossing.

#include <ansi.h>

#define PLOT_ID "silent_crossing_03"
#define ROOT "plot/silent_crossing_03"
#define DECK "/d/plot/silent_crossing/boat_deck"
#define FOREDECK "/d/plot/silent_crossing/foredeck"
#define STERN "/d/plot/silent_crossing/stern"
#define PASSAGE "/d/plot/silent_crossing/narrow_passage"
#define HOLD "/d/plot/silent_crossing/cargo_hold"
#define BILGE "/d/plot/silent_crossing/bilge"
#define LIU_DASHAO "/clone/plot/silent_crossing/liu_dashao"
#define A_HE "/clone/plot/silent_crossing/a_he"
#define LUO_QINIANG "/clone/plot/silent_crossing/luo_qiniang"
#define STUB "/clone/plot/silent_crossing/relabelled_stub"

string render_log(object me);
void instance_left(object me);

private string stage(object me)
{
	return me->query(ROOT + "/stage");
}

private string status(object me)
{
	return me->query(ROOT + "/status");
}

private int in_instance(object me)
{
	string room;

	if (! objectp(environment(me))) return 0;
	room = base_name(environment(me));
	return environment(me)->query("plot_owner") == me->query("id") &&
		(room == DECK || room == FOREDECK || room == STERN ||
		 room == PASSAGE || room == HOLD || room == BILGE);
}

private object query_deck(object me)
{
	return PLOT_D->query_instance(me, PLOT_ID);
}

private object query_hold(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "cargo_hold");
}

private object query_foredeck(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "foredeck");
}

private object query_stern(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "stern");
}

private object query_passage(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "narrow_passage");
}

private object query_bilge(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "bilge");
}

private object ensure_liu(object me, object room)
{
	object liu;

	liu = present("liu dashao", room);
	if (objectp(liu))
	{
		room->set("plot_spawned/liu_dashao", 1);
		return liu;
	}
	if (room->query("plot_spawned/liu_dashao")) return 0;
	liu = new(LIU_DASHAO);
	if (! objectp(liu)) return 0;
	liu->set("plot_owner", me->query("id"));
	liu->move(room);
	room->set("plot_spawned/liu_dashao", 1);
	return liu;
}

private object ensure_a_he(object me, object room)
{
	object ahe;
	string condition;

	ahe = present("a he", room);
	condition = PLOT_D->query_flag(me, PLOT_ID, "a_he_condition");
	if (objectp(ahe))
	{
		room->set("plot_spawned/a_he", 1);
		if (stringp(condition)) ahe->restore_plot_condition(condition);
		return ahe;
	}
	if (room->query("plot_spawned/a_he")) return 0;
	ahe = new(A_HE);
	if (! objectp(ahe)) return 0;
	ahe->set("plot_owner", me->query("id"));
	ahe->move(room);
	room->set("plot_spawned/a_he", 1);
	if (stringp(condition)) ahe->restore_plot_condition(condition);
	return ahe;
}

private object ensure_luo(object me, object room)
{
	object luo;

	if (PLOT_D->query_flag(me, PLOT_ID, "luo_resolved")) return 0;
	luo = present("luo qiniang", room);
	if (objectp(luo) && living(luo))
	{
		room->set("plot_spawned/luo_qiniang", 1);
		PLOT_COMBAT_D->restore_enemy(luo);
		return luo;
	}
	if (room->query("plot_spawned/luo_qiniang")) return 0;
	luo = new(LUO_QINIANG);
	if (! objectp(luo)) return 0;
	luo->set("plot_owner", me->query("id"));
	if (! mapp(PLOT_COMBAT_D->configure_enemy(luo, me, "ferry_matron", 0)))
	{
		destruct(luo);
		return 0;
	}
	luo->move(room);
	room->set("plot_spawned/luo_qiniang", 1);
	return luo;
}

private object create_deck(object me)
{
	object room;

	room = query_deck(me);
	if (! objectp(room))
	{
		room = PLOT_D->open_instance(me, PLOT_ID, DECK, "/d/city/beimen");
		if (! objectp(room)) return 0;
	}
	ensure_liu(me, room);
	return room;
}

private object create_hold(object me)
{
	object room;

	room = query_hold(me);
	if (! objectp(room))
	{
		room = PLOT_D->open_instance_room(me, PLOT_ID, "cargo_hold", HOLD);
		if (! objectp(room)) return 0;
		room->set("exits/out", "/d/city/beimen");
	}
	ensure_a_he(me, room);
	if (stage(me) == "stop_burning_records") ensure_luo(me, room);
	return room;
}

private object create_side_room(object me, string slot, string blueprint)
{
	object room;

	room = PLOT_D->query_instance_room(me, PLOT_ID, slot);
	if (objectp(room)) return room;
	room = PLOT_D->open_instance_room(me, PLOT_ID, slot, blueprint);
	if (! objectp(room)) return 0;
	room->set("exits/out", "/d/city/beimen");
	return room;
}

private string enter_deck(object me)
{
	object room;

	room = create_deck(me);
	if (! objectp(room)) return "青芦渡的夜船入口暂时无法打开，请稍后再试。";
	me->move(room);
	return "你踏过湿滑跳板，登上没有船名的乌篷夜船。";
}

private string enter_hold(object me)
{
	object room;

	room = create_hold(me);
	if (! objectp(room)) return "夜船货舱暂时无法打开，请稍后再试。";
	me->move(room);
	return "你沿着窄梯钻入货舱，潮湿木板下传来绳索绷紧的声响。";
}

private string enter_foredeck(object me)
{
	object room;

	room = create_side_room(me, "foredeck", FOREDECK);
	if (! objectp(room)) return "夜船船首暂时无法打开，请稍后再试。";
	me->move(room);
	return "你截住缆绳跃上船首，守梯押运人立刻用湿缆封住通道。";
}

private string enter_stern(object me)
{
	object room;

	room = create_side_room(me, "stern", STERN);
	if (! objectp(room)) return "夜船船尾暂时无法打开，请稍后再试。";
	me->move(room);
	return "你借船尾暗影翻过低舷，避开了甲板中央的巡夜人。";
}

private string enter_passage(object me)
{
	object room;

	room = create_side_room(me, "narrow_passage", PASSAGE);
	if (! objectp(room)) return "狭舱通道暂时无法打开，请稍后再试。";
	me->move(room);
	return "三条登船路线都汇入乌篷下的狭舱通道。前方木梯直落货舱。";
}

private string enter_bilge(object me)
{
	object room;

	room = create_side_room(me, "bilge", BILGE);
	if (! objectp(room)) return "舱底暂时无法进入，请稍后再试。";
	me->move(room);
	return "你掀开检修板落到舱底，浑浊河水正从船缝间缓慢渗入。";
}

private string begin_watch(object me)
{
	string here;

	if (status(me) == "available")
	{
		if (! PLOT_D->begin_chapter(me, PLOT_ID)) return "第三章暂时无法接取，请稍后再试。";
	}
	if (status(me) != "active" || stage(me) != "watch_green_reed_ferry") return render_log(me);
	here = base_name(environment(me));
	if (here != "/d/city/beimen" && here != "/d/city/postofficer")
		return "青芦渡在扬州北门水路外。先到北门，再按回浪印辨认夜船。";
	PLOT_D->set_flag(me, PLOT_ID, "safe_return", here);
	PLOT_D->set_flag(me, PLOT_ID, "ferry_scene_seen", 1);
	return ZJOBLONG "夜渡无声\n\n"
		"青芦渡没有固定码头名，只有三艘乌篷船在暗水里轮换灯号。回浪凭据写着二潮、二十三号船和七号仓，\n"
		"而渡口潮牌说明第三盏灯才代表救济货。选错不会错过夜船，但必须指出哪组灯号同时吻合。\n\n" + render_log(me);
}

private string choose_signal(object me, string value)
{
	if (stage(me) != "watch_green_reed_ferry") return render_log(me);
	if (value == "17")
		return "十七号灯船在一潮前就解缆，早于回浪凭据记录的放行时刻。";
	if (value == "9")
		return "九号灯船第三盏是客货灯，不是凭据所记的救济货灯。";
	if (value != "23") return "没有这组灯号。";
	PLOT_D->set_flag(me, PLOT_ID, "night_boat_known", 1);
	PLOT_D->set_flag(me, PLOT_ID, "signal_ship", "23");
	PLOT_D->advance_stage(me, PLOT_ID, "watch_green_reed_ferry", "board_boat");
	return "二十三号灯船在二潮前亮起第三盏货灯，正与双半印吻合。你已经锁定真正的夜船。\n\n" + render_log(me);
}

private string board_boat(object me, string method)
{
	string text;

	if (stage(me) != "board_boat") return render_log(me);
	if (method == "token")
	{
		if (! me->query("plot/returning_mark_02/flags/full_token_recovered"))
			return "你没有完整回浪凭据，船主不会凭一句话放下跳板。可以潜行观察或正面截船。";
		text = "你将完整回浪凭据压在船签旁，柳大艄沉默片刻，放下了跳板。";
	}
	else if (method == "stealth")
		text = "你借装卸麻袋的阴影贴近船舷，从系缆处翻上甲板；这条路不检查轻功门槛。";
	else if (method == "force")
	{
		text = "你在岸上亮明改签粮案，截住缆绳登船。押运人立刻收紧阵脚，后续冲突压力更高。";
		PLOT_D->set_flag(me, PLOT_ID, "boarding_pressure", 1);
		PLOT_D->set_flag(me, PLOT_ID, "deck_blocked", 1);
	}
	else return "没有这种登船方式。";
	PLOT_D->set_flag(me, PLOT_ID, "boarding_method", method);
	PLOT_D->advance_stage(me, PLOT_ID, "board_boat", "reach_cargo_hold");
	if (method == "token") enter_deck(me);
	else if (method == "stealth") enter_stern(me);
	else enter_foredeck(me);
	return text + "\n\n" + render_log(me);
}

private string reach_hold(object me)
{
	if (stage(me) != "reach_cargo_hold") return render_log(me);
	if (! in_instance(me)) return "先按选定的方式重新登上夜船。";
	if (PLOT_D->query_flag(me, PLOT_ID, "deck_blocked") &&
		! PLOT_D->query_flag(me, PLOT_ID, "deck_secured"))
		return "正面截船惊动了押运人，他们用缆绳封住窄梯。先夺下系缆柱，清出通往货舱的路。";
	if (base_name(environment(me)) != PASSAGE)
	{
		PLOT_D->set_flag(me, PLOT_ID, "passage_reached", 1);
		return enter_passage(me) + "\n\n继续沿木梯下到货舱。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "cargo_hold_reached", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "reach_cargo_hold", "save_a_he");
	return enter_hold(me) + "\n\n压舱板突然断裂，阿禾被绳索和药箱压在舱角。先把人救出来。\n";
}

private string rescue_a_he(object me, string method)
{
	string text;

	if (stage(me) != "save_a_he" || ! in_instance(me) || base_name(environment(me)) != HOLD)
		return "必须先进入夜船货舱找到阿禾。";
	if (method == "crates")
		text = "你和柳大艄依次卸掉上层粮袋，再抬开药箱，把阿禾从断板下拖了出来。";
	else if (method == "rope")
		text = "你先固定主缆，再割断缠住阿禾脚踝的废绳，压舱货没有继续滑落。";
	else if (method == "guard")
		text = "你撞开还想护货不救人的押运者，以舱柱作支点掀开箱角，救出了阿禾。";
	else return "没有这种救援办法。";
	PLOT_D->set_flag(me, PLOT_ID, "a_he_condition", "rescued");
	PLOT_D->set_flag(me, PLOT_ID, "a_he_rescue_method", method);
	PLOT_D->set_flag(me, PLOT_ID, "a_he_testimony", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "save_a_he", "inspect_three_cargos");
	return text + "\n\n阿禾喘匀气后指出：药签上那个被登记为亡者的姓名，属于仍然活着的亲人。\n\n" + render_log(me);
}

void record_a_he_injury(object ahe)
{
	object me;
	string owner;

	if (! objectp(ahe) || base_name(ahe) != A_HE) return;
	owner = ahe->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || environment(me) != environment(ahe) || stage(me) != "save_a_he") return;
	PLOT_D->set_flag(me, PLOT_ID, "a_he_condition", "injured");
	PLOT_D->set_flag(me, PLOT_ID, "a_he_testimony", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "save_a_he", "inspect_three_cargos");
}

private string inspect_cargo(object me, string kind)
{
	int count;
	string text;
	object room;

	if (stage(me) != "inspect_three_cargos" || ! in_instance(me) || base_name(environment(me)) != HOLD)
		return "三类货物都在夜船货舱里。";
	if (kind == "grain")
	{
		PLOT_D->set_flag(me, PLOT_ID, "cargo_grain_seen", 1);
		text = "改签粮袋沿针脚藏着义仓旧号，正是第一章亏空中的一部分。";
	}
	else if (kind == "medicine")
	{
		PLOT_D->set_flag(me, PLOT_ID, "cargo_medicine_seen", 1);
		text = "药材由船户自筹，药签却夹着被改写生死状态的姓名和家属指印。";
	}
	else if (kind == "relabeled")
	{
		PLOT_D->set_flag(me, PLOT_ID, "cargo_relabeled_seen", 1);
		text = "无册灾粮的来源混杂，但收货村仍在受灾；夹层原始货单的付款人只写“沈先生”，潮纹还缺了一笔。";
	}
	else return "没有这类货物。";
	count = (PLOT_D->query_flag(me, PLOT_ID, "cargo_grain_seen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "cargo_medicine_seen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "cargo_relabeled_seen") ? 1 : 0);
	PLOT_D->set_flag(me, PLOT_ID, "cargo_inspected", count);
	if (count >= 3)
	{
		PLOT_D->set_flag(me, PLOT_ID, "cargo_truth_known", 1);
		PLOT_D->advance_stage(me, PLOT_ID, "inspect_three_cargos", "stop_burning_records");
		room = environment(me);
		ensure_luo(me, room);
		text += "\n\n罗七娘扯下改签存根，火折已经逼近舱灯。核心存根必须保住。";
	}
	return text + "\n\n" + render_log(me);
}

private int settle_records(object me, string method, string fate, int saved)
{
	object luo;

	PLOT_D->set_flag(me, PLOT_ID, "record_method", method);
	PLOT_D->set_flag(me, PLOT_ID, "luo_fate", fate);
	PLOT_D->set_flag(me, PLOT_ID, "luo_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "records_saved", saved);
	luo = present("luo qiniang", environment(me));
	if (objectp(luo)) destruct(luo);
	return PLOT_D->advance_stage(me, PLOT_ID, "stop_burning_records", "settle_the_boat");
}

void record_luo_defeat(object luo)
{
	object me;
	object room;
	string owner;

	if (! objectp(luo) || base_name(luo) != LUO_QINIANG) return;
	room = environment(luo);
	owner = luo->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || ! objectp(room) || environment(me) != room ||
		room->query("plot_owner") != owner || stage(me) != "stop_burning_records") return;
	PLOT_D->set_flag(me, PLOT_ID, "record_method", "fight");
	PLOT_D->set_flag(me, PLOT_ID, "luo_fate", "defeated");
	PLOT_D->set_flag(me, PLOT_ID, "luo_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "records_saved", 2);
	PLOT_D->advance_stage(me, PLOT_ID, "stop_burning_records", "settle_the_boat");
}

private string resolve_records(object me, string method)
{
	object luo;

	if (stage(me) != "stop_burning_records" || ! in_instance(me) || base_name(environment(me)) != HOLD)
		return "必须先查清三类货物并留在货舱现场。";
	luo = present("luo qiniang", environment(me));
	if (method == "fight")
	{
		if (! objectp(luo) || ! living(luo)) return render_log(me);
		me->kill_ob(luo);
		luo->kill_ob(me);
		return "罗七娘拔出短刀守住火折。击败她后，核心改签存根会先写入剧情事实。";
	}
	if (method == "water")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "water_bucket_ready"))
			return "舱内没有现成水桶。先从舱底汲水，把水放到火折与存根之间。";
		settle_records(me, "water", "retreated", 2);
		return "你把舱底水泼向火折和湿麻布，罗七娘只能弃火退到船尾。核心存根和一份附签被保住。\n\n" + render_log(me);
	}
	if (method == "turn")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "relabelled_tag_shown"))
			return "船工只听见争执，还没看到无面局怎样改写活人的姓名。先把换名药签摊开。";
		settle_records(me, "turn", "restrained", 3);
		return "阿禾认出药签姓名，船工当场按住火折。罗七娘被迫交出全部三份存根。\n\n" + render_log(me);
	}
	return "没有这种保全存根的方法。";
}

private string settle_boat(object me, string method)
{
	int relation;
	string guarantor;
	string text;

	if (stage(me) != "settle_the_boat" || ! PLOT_D->query_flag(me, PLOT_ID, "records_saved"))
		return render_log(me);
	if (method == "seal")
	{
		relation = 1;
		text = "你封存改签义仓粮，让药材和无册灾粮继续上路，并把存根带回追索。";
	}
	else if (method == "aid_first")
	{
		relation = 2;
		text = "你让柳大艄先卸救命粮，再带改签存根回扬州补缴；船户愿意留下完整航路证词。";
	}
	else if (method == "guarantor")
	{
		int hall;
		int yamen;
		int grain;
		hall = me->query("plot/arc/hometown_letters_01/relations/hometown_hall");
		yamen = me->query("plot/arc/hometown_letters_01/relations/yamen");
		grain = me->query("plot/arc/hometown_letters_01/relations/grain_house");
		guarantor = hall >= yamen && hall >= grain ? "hometown_hall" :
			(yamen >= grain ? "yamen" : "grain_house");
		PLOT_D->set_flag(me, PLOT_ID, "guarantor", guarantor);
		relation = 1;
		text = "你让关系最稳的一方作有限担保：只追索改签粮，不扣押药材和救命粮。";
	}
	else return "没有这种当夜处置。";
	PLOT_D->set_flag(me, PLOT_ID, "night_settlement", method);
	PLOT_D->set_relation(me, PLOT_ID, "boatmen", relation);
	PLOT_D->set_arc_relation(me, "boatmen", relation);
	PLOT_D->advance_stage(me, PLOT_ID, "settle_the_boat", "chapter_close");
	return text + "\n\n" + render_log(me);
}

private string close_chapter(object me)
{
	object stub;

	if (stage(me) != "chapter_close") return render_log(me);
	if (in_instance(me))
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/beimen";
		me->move(safe);
	}
	if (! PLOT_D->complete_chapter(me, PLOT_ID, "chapter_close"))
		return "夜船事实已经查清，但章节完成状态暂时无法保存，请再试一次。";
	if (! PLOT_D->query_flag(me, PLOT_ID, "reward_claimed"))
	{
		if (present("relabelled stub", me))
			PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
		else
		{
			stub = new(STUB);
			if (objectp(stub) && stub->move(me, 1))
				PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
			else if (objectp(stub)) destruct(stub);
		}
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "score_claimed"))
	{
		me->add("score", 15);
		PLOT_D->set_flag(me, PLOT_ID, "score_claimed", 1);
	}
	PLOT_D->set_arc_flag(me, "baichuan_known", 1);
	PLOT_D->set_arc_flag(me, "verified_relabelled_stub", 1);
	PLOT_D->set_arc_flag(me, "home_letters_surge_known", 1);
	PLOT_D->set_arc_value(me, "current_chapter", 4);
	instance_left(me);
	return ZJOBLONG "夜渡无声\n\n"
		"你保住了改签存根，也没有让药材和救命粮被粗暴销毁。百川会曾在官仓失效时救过人，\n"
		"但闻守拙已经把旧日恩义变成欠据和改名的权力。柳大艄还交出近期船信：有人正以你的故乡名义大量寄信。\n\n"
		"第三章「夜渡无声」完成。你获得少量阅历和一份无属性优势的改签存根纪念副本。\n";
}

private string claim_stub(object me)
{
	object stub;

	if (status(me) != "completed") return "第三章尚未完成。";
	if (present("relabelled stub", me)) return "改签存根纪念副本还在你身上。";
	stub = new(STUB);
	if (! objectp(stub) || ! stub->move(me, 1))
	{
		if (objectp(stub)) destruct(stub);
		return "你身上没有空位，清出一格后再补领改签存根。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	return "杜宽根据剧情日志替你补出一份改签存根纪念副本。";
}

void instance_left(object me)
{
	object room;
	object foredeck;
	object stern;
	object passage;
	object hold;
	object bilge;
	object ob;

	if (! objectp(me)) return;
	room = query_deck(me);
	foredeck = query_foredeck(me);
	stern = query_stern(me);
	passage = query_passage(me);
	hold = query_hold(me);
	bilge = query_bilge(me);
	if (objectp(environment(me)) &&
		(base_name(environment(me)) == DECK || base_name(environment(me)) == FOREDECK ||
		 base_name(environment(me)) == STERN || base_name(environment(me)) == PASSAGE ||
		 base_name(environment(me)) == HOLD || base_name(environment(me)) == BILGE)) return;
	foreach (ob in (objectp(room) ? all_inventory(room) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(foredeck) ? all_inventory(foredeck) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(stern) ? all_inventory(stern) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(passage) ? all_inventory(passage) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(hold) ? all_inventory(hold) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(bilge) ? all_inventory(bilge) : ({})))
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
	if (status(me) == "available")
		return "回浪印已经指向青芦渡。到扬州北门观察三艘夜船的灯号。\n";
	if (status(me) == "completed")
		return "百川会与改签存根已经记入乡书篇。可向杜宽补领纪念副本。\n"
			"补领：plot act silent_crossing_03 claim_stub\n";
	if (current == "watch_green_reed_ferry")
		return "夜船灯号：可核对 17、9、23 号船；提示不会消耗机会。\n";
	if (current == "board_boat")
		return "登船方式：完整凭据、通用潜行或正面截船。三路都会在货舱合流。\n";
	if (current == "reach_cargo_hold")
	{
		if (PLOT_D->query_flag(me, PLOT_ID, "deck_blocked") &&
			! PLOT_D->query_flag(me, PLOT_ID, "deck_secured"))
			return "正面截船触发甲板封锁。先夺下系缆柱清开窄梯，再进入货舱合流。\n";
		return "你已登上夜船。目标：沿窄梯进入货舱，三种登船路线在此合流。\n";
	}
	if (current == "save_a_he")
		return "阿禾被压舱货困住：可逐层搬箱、固定后割绳，或撞开阻拦者救人。\n";
	if (current == "inspect_three_cargos")
	{
		count = PLOT_D->query_flag(me, PLOT_ID, "cargo_inspected");
		return sprintf("已核验货物：%d/3。需要分别查看改签义仓粮、船户药材和无册灾粮。\n", count);
	}
	if (current == "stop_burning_records")
		return "罗七娘准备焚毁存根：可正面交手；先汲舱底水再灭火；或先展示换名药签让船工倒戈。\n";
	if (current == "settle_the_boat")
		return "当夜处置：封存改签粮、先卸救命粮再补缴，或请关系最稳的一方有限担保。\n";
	if (current == "chapter_close")
		return "人命和存根都已保住。返回日志完成第三章结算。\n";
	return "目标：确认青芦渡夜船与百川会暗粮路的真实用途。\n";
}

string ask_liu(object me)
{
	if (! objectp(me)) return "柳大艄没有认出你。";
	return "柳大艄低声道：船上三类货不能一刀切。改签粮要追，药和救命粮不能先毁。\n" + render_log(me);
}

string ask_a_he(object me)
{
	if (! objectp(me)) return "阿禾没有回应。";
	return "阿禾指着换名药签：这名字是我亲人，他还活着，却已经被写成亡者。\n";
}

mixed handle_action(object me, string action)
{
	if (! objectp(me) || ! userp(me)) return "无效的剧情参与者。";
	PLOT_D->ensure_player_state(me);
	if (action == "begin" || action == "watch_green_reed_ferry") return begin_watch(me);
	if (action == "signal_17") return choose_signal(me, "17");
	if (action == "signal_9") return choose_signal(me, "9");
	if (action == "signal_23") return choose_signal(me, "23");
	if (action == "signal_hint") return "提示：二潮、二十三号船和第三盏救济货灯必须同时成立。";
	if (action == "board_token") return board_boat(me, "token");
	if (action == "board_stealth") return board_boat(me, "stealth");
	if (action == "board_force") return board_boat(me, "force");
	if (action == "secure_deck")
	{
		if (stage(me) != "reach_cargo_hold" || ! in_instance(me) ||
			base_name(environment(me)) != FOREDECK) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "deck_secured", 1);
		return "你借湿滑船板逼退守梯押运人，夺下系缆柱，把封住窄梯的缆绳解开。";
	}
	if (action == "reach_cargo_hold") return reach_hold(me);
	if (action == "rescue_crates") return rescue_a_he(me, "crates");
	if (action == "rescue_rope") return rescue_a_he(me, "rope");
	if (action == "rescue_guard") return rescue_a_he(me, "guard");
	if (action == "inspect_grain") return inspect_cargo(me, "grain");
	if (action == "inspect_medicine") return inspect_cargo(me, "medicine");
	if (action == "inspect_relabelled") return inspect_cargo(me, "relabeled");
	if (action == "fight_luo") return resolve_records(me, "fight");
	if (action == "draw_water")
	{
		if (stage(me) != "stop_burning_records" || ! in_instance(me) ||
			base_name(environment(me)) != HOLD) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "water_bucket_ready", 1);
		return enter_bilge(me) + "\n\n你从积水处汲起一桶河水。带回货舱后即可挡住火折。";
	}
	if (action == "return_hold")
	{
		if (stage(me) != "stop_burning_records" || ! in_instance(me) ||
			base_name(environment(me)) != BILGE) return render_log(me);
		return enter_hold(me) + "\n\n水桶已经放到火折与存根之间。";
	}
	if (action == "douse_records") return resolve_records(me, "water");
	if (action == "show_relabelled_tag")
	{
		if (stage(me) != "stop_burning_records" || ! in_instance(me)) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "relabelled_tag_shown", 1);
		return "你把写着活人姓名的换名药签摊在船灯下，阿禾当场认出家人的指印。";
	}
	if (action == "turn_shipworkers") return resolve_records(me, "turn");
	if (action == "settle_seal") return settle_boat(me, "seal");
	if (action == "settle_aid_first") return settle_boat(me, "aid_first");
	if (action == "settle_guarantor") return settle_boat(me, "guarantor");
	if (action == "chapter_close" || action == "aftermath") return close_chapter(me);
	if (action == "claim_stub") return claim_stub(me);
	if (action == "enter")
	{
		if (stage(me) == "reach_cargo_hold")
		{
			string method;
			if (PLOT_D->query_flag(me, PLOT_ID, "passage_reached")) return enter_passage(me);
			method = PLOT_D->query_flag(me, PLOT_ID, "boarding_method");
			if (method == "stealth") return enter_stern(me);
			if (method == "force") return enter_foredeck(me);
			return enter_deck(me);
		}
		if (stage(me) == "save_a_he" || stage(me) == "inspect_three_cargos" ||
			stage(me) == "stop_burning_records" || stage(me) == "settle_the_boat")
			return enter_hold(me);
		return render_log(me);
	}
	if (action == "leave")
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/beimen";
		if (in_instance(me)) me->move(safe);
		instance_left(me);
		return "你离开夜船，回到青芦渡安全处。";
	}
	return "这项剧情操作不存在。";
}
