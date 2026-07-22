// Chapter one: A Letter From Home.

#include <ansi.h>

#define PLOT_ID "origin_letter_01"
#define ROOT "plot/origin_letter_01"
#define WAREHOUSE "/d/plot/origin_letter/warehouse"
#define PEI_JIU "/clone/plot/origin_letter/pei_jiu"
#define TOKEN "/clone/plot/origin_letter/old_post_token"

string render_log(object me);
private string hear_clue(object me, string source);
void instance_left(object me);

private string stage(object me)
{
	return me->query(ROOT + "/stage");
}

private string status(object me)
{
	return me->query(ROOT + "/status");
}

private string current_family(object me)
{
	string family;

	family = me->query("family/family_name");
	return stringp(family) ? family : "";
}

private string origin_label(object me)
{
	string label;

	label = me->query("born_family");
	if (stringp(label) && label != "" && label != "没有")
		return label;
	label = me->query("born");
	if (stringp(label) && label != "")
		return label;
	return "籍贯无考";
}

private string origin_group(string label)
{
	if (label == "籍贯无考") return "compat";
	if (strsrch(label, "世家") >= 0 || strsrch(label, "皇族") >= 0 ||
		strsrch(label, "胡家") >= 0) return "family";
	if (strsrch(label, "关外") >= 0 || strsrch(label, "燕赵") >= 0 ||
		strsrch(label, "齐鲁") >= 0 || strsrch(label, "秦晋") >= 0) return "north";
	if (strsrch(label, "中原") >= 0 || strsrch(label, "荆州") >= 0) return "central";
	if (strsrch(label, "扬州") >= 0 || strsrch(label, "苏州") >= 0 ||
		strsrch(label, "杭州") >= 0) return "south";
	if (strsrch(label, "福建") >= 0 || strsrch(label, "两广") >= 0) return "southeast";
	if (strsrch(label, "巴蜀") >= 0 || strsrch(label, "云南") >= 0 ||
		strsrch(label, "大理") >= 0) return "southwest";
	if (strsrch(label, "西域") >= 0) return "west";
	return "central";
}

private string clue_flavor(object me)
{
	string group;

	group = PLOT_D->query_flag(me, PLOT_ID, "origin_group");
	if (group == "north") return "关隘印记和陆路货签";
	if (group == "south") return "水路平码和潮次记号";
	if (group == "southeast") return "商帮双结货签";
	if (group == "southwest") return "药材包上的山道节点";
	if (group == "west") return "从右侧起笔的异文货签";
	if (group == "family") return "只刻一半的旧驿纹";
	if (group == "compat") return "杜宽收件簿上的纸张水印";
	return "官道与渡口并列的义仓编号";
}

private int at_room(object me, string room)
{
	return objectp(environment(me)) && base_name(environment(me)) == room;
}

private int in_yangzhou_entry(object me)
{
	return at_room(me, "/d/city/postofficer") ||
		at_room(me, "/d/city/wumiao") || at_room(me, "/d/city/kedian");
}

private int has_world_identity(object me)
{
	string startroom;

	if (me->query("born")) return 1;
	startroom = me->query("startroom");
	return me->query("registered") && stringp(startroom) && startroom != "" &&
		strsrch(startroom, "/d/death/") != 0;
}

private int clue_count(object me)
{
	return (PLOT_D->query_flag(me, PLOT_ID, "clue_hall") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "clue_yamen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "clue_grain") ? 1 : 0);
}

private string method_label(string value)
{
	if (value == "battle") return "正面击败裴九";
	if (value == "persuade") return "以证词说服裴九";
	if (value == "ransom") return "以银两赎回账簿";
	return "尚未取得";
}

private string custodian_label(string value)
{
	if (value == "hall") return "同乡会馆公开";
	if (value == "yamen") return "衙门密封";
	if (value == "grain") return "粮行私了";
	return "尚未决定";
}

private object query_instance(object me)
{
	return PLOT_D->query_instance(me, PLOT_ID);
}

private object create_instance(object me)
{
	object room;
	object pei;
	string return_room;

	room = query_instance(me);
	if (! objectp(room))
	{
		return_room = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(return_room) || file_size(return_room + ".c") < 0)
			return_room = "/d/city/beimen";
		room = PLOT_D->open_instance(me, PLOT_ID, WAREHOUSE, return_room);
		if (! objectp(room)) return 0;
	}
	pei = present("pei jiu", room);
	if (objectp(pei) && living(pei))
	{
		room->set("plot_spawned/pei_jiu", 1);
		PLOT_COMBAT_D->restore_enemy(pei);
		return room;
	}
	if (PLOT_D->query_flag(me, PLOT_ID, "pei_battle_won")) return room;
	if (room->query("plot_spawned/pei_jiu")) return room;
	pei = new(PEI_JIU);
	pei->set("plot_owner", me->query("id"));
	if (! mapp(PLOT_COMBAT_D->configure_enemy(pei, me, "street_blade", 0)))
	{
		destruct(pei);
		PLOT_D->close_instance(me, PLOT_ID);
		return 0;
	}
	pei->move(room);
	room->set("plot_spawned/pei_jiu", 1);
	return room;
}

private string enter_instance(object me)
{
	object room;

	room = create_instance(me);
	if (! objectp(room))
		return "废仓入口一时无法打开，请稍后再试。";
	me->move(room);
	return "你循着货签上的旧记号，推开一扇半朽的仓门。";
}

private string notice(object me)
{
	if (! has_world_identity(me))
		return "你尚未完成投胎，此时还没有属于自己的乡书。";
	if (status(me) == "completed")
		return render_log(me);
	PLOT_D->set_flag(me, PLOT_ID, "notice_seen", 1);
	return ZJOBLONG "一封边角发黄的短笺\n\n"
		"杜宽托人送来消息：扬州驿站压着一封从你来处寄来的旧信，寄信人只求有人查清一册义仓簿的下落。\n\n"
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
		"前往扬州驿站:plot origin_letter_01" ZJSEP
		"查看剧情日志:plot origin_letter_01\n";
}

string notify_entry(object me)
{
	if (! objectp(me)) return "";
	PLOT_D->ensure_player_state(me);
	if (status(me) != "available" ||
		PLOT_D->query_flag(me, PLOT_ID, "notice_seen"))
		return "";
	PLOT_D->set_flag(me, PLOT_ID, "notice_seen", 1);
	return "杜宽托人送来一封同乡旧信：扬州驿站压着一桩与你来处有关的求助。到驿站问‘乡书’，或在武庙查看旧闻。";
}

string yamen_inquiry(object me)
{
	if (! objectp(me)) return "";
	if (status(me) == "available") return notice(me);
	if (stage(me) == "hear_three_sides" || stage(me) == "trace_warehouse")
		return hear_clue(me, "yamen");
	return render_log(me);
}

private string accept_letter(object me)
{
	string label;
	string family;

	if (! in_yangzhou_entry(me))
		return "杜宽把信留在扬州驿站。请先到驿站、武庙或宝昌客栈查看。";
	if (status(me) == "available")
	{
		if (! PLOT_D->begin_chapter(me, PLOT_ID))
			return "乡书暂时无法接取，请稍后再试。";
	}
	if (stage(me) != "notice")
		return render_log(me);
	label = origin_label(me);
	family = current_family(me);
	PLOT_D->set_flag(me, PLOT_ID, "origin_label", label);
	PLOT_D->set_flag(me, PLOT_ID, "origin_group", origin_group(label));
	PLOT_D->set_flag(me, PLOT_ID, "family_at_start", family);
	if (! PLOT_D->advance_stage(me, PLOT_ID, "notice", "accept_letter"))
		return "杜宽正要取信，驿站里却忽然忙乱起来。请再问一次。";
	return ZJOBLONG "杜宽从箱底翻出一封旧信\n\n"
		"“信是从「" + label + "」那一路来的。有人护送义仓簿到扬州，城外遇袭，账簿和领头人都失了踪。”\n\n" +
		(family != "" ? "杜宽又道：“你如今已有师承，但这件事只问你的来处，不问门墙。”\n\n" :
		"杜宽道：“这事不问师承，接与不接，只看你自己。”\n\n") +
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) + "开始调查:plot act origin_letter_01 begin_inquiry\n";
}

private string begin_inquiry(object me)
{
	if (stage(me) == "accept_letter")
		PLOT_D->advance_stage(me, PLOT_ID, "accept_letter", "hear_three_sides");
	if (stage(me) != "hear_three_sides" && stage(me) != "trace_warehouse")
		return render_log(me);
	return ZJOBLONG "三方说法\n\n"
		"许三娘主张公开账目救急；谭友纪希望密封取证；周守义愿补足粮食换回账簿。至少听取两方说法，才能追到废仓。\n\n" +
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
		"前往同乡会馆:plot act origin_letter_01 go_hall" ZJSEP
		"前往衙门正厅:plot act origin_letter_01 go_yamen" ZJSEP
		"前往粮行账房:plot act origin_letter_01 go_grain\n";
}

private string hear_clue(object me, string source)
{
	string key;
	string text;
	int count;

	if (stage(me) != "hear_three_sides" && stage(me) != "trace_warehouse")
		return "眼下还不到听取这份证词的时候。";
	if (source == "hall")
	{
		key = "clue_hall";
		text = "许三娘出示会馆登记：二十三户乡民留下姓名，却有三十袋粮从粮行出库。公开账目能立刻施压，也会暴露作保人。";
	}
	else if (source == "yamen")
	{
		key = "clue_yamen";
		text = "谭友纪查到二十三枚签收手印。衙门可以保全证据，但立案与放粮不会同时完成。";
	}
	else
	{
		key = "clue_grain";
		text = "周守义承认粮行发出三十袋粮，愿立即补粮私了，却不肯说明多出的七袋为何没有领粮人姓名。";
	}
	PLOT_D->set_flag(me, PLOT_ID, key, 1);
	count = clue_count(me);
	PLOT_D->set_flag(me, PLOT_ID, "clues_heard", count);
	if (count >= 2 && stage(me) == "hear_three_sides")
		PLOT_D->advance_stage(me, PLOT_ID, "hear_three_sides", "trace_warehouse");
	return ZJOBLONG + text + "\n\n你又认出物证上的" + clue_flavor(me) + "。\n\n" + render_log(me);
}

private string go_room(object me, string room, string prompt)
{
	if (stage(me) != "hear_three_sides" && stage(me) != "trace_warehouse")
		return render_log(me);
	me->move(room);
	return prompt;
}

private string trace_warehouse(object me)
{
	string here;

	if (stage(me) == "face_pei_jiu")
		return enter_instance(me);
	if (stage(me) != "trace_warehouse")
		return render_log(me);
	if (clue_count(me) < 2)
		return "至少还要核实两方说法。";
	here = base_name(environment(me));
	if (here != "/d/city/beimen" && here != "/d/city/postofficer")
		return "线索指向扬州北门外，请先回到北门或驿站。";
	PLOT_D->set_flag(me, PLOT_ID, "safe_return", here);
	if (! PLOT_D->advance_stage(me, PLOT_ID, "trace_warehouse", "face_pei_jiu"))
		return "废仓线索暂时无法确认。";
	return enter_instance(me);
}

private int in_warehouse(object me)
{
	return objectp(environment(me)) &&
		environment(me)->query("plot_owner") == me->query("id") &&
		base_name(environment(me)) == WAREHOUSE;
}

void record_pei_death(object pei)
{
	object me;
	object room;
	string owner;

	if (! objectp(pei) || base_name(pei) != PEI_JIU) return;
	room = environment(pei);
	owner = pei->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || ! objectp(room) || environment(me) != room ||
		room->query("plot_owner") != owner || stage(me) != "face_pei_jiu")
		return;
	PLOT_D->set_flag(me, PLOT_ID, "pei_battle_won", 1);
	PLOT_D->set_flag(me, PLOT_ID, "pei_jiu_fate", "killed");
}

private string recover_ledger(object me, string method)
{
	if (stage(me) != "face_pei_jiu" || ! in_warehouse(me))
		return "你必须先在自己的废仓实例中面对裴九。";
	PLOT_D->set_flag(me, PLOT_ID, "ledger_method", method);
	PLOT_D->set_flag(me, PLOT_ID, "ledger_recovered", 1);
	PLOT_D->set_flag(me, PLOT_ID, "pei_jiu_fate",
		method == "battle" ? "killed" : "released");
	if (! PLOT_D->advance_stage(me, PLOT_ID, "face_pei_jiu", "inspect_ledger"))
		return "账簿已经到手，但状态未能保存，请稍后重试。";
	return ZJOBLONG "你取回了义仓簿\n\n"
		"簿中粮数、户数和手印本应一一对应，装订边却有整列被裁去。裴九留下的半张名单上，正好还有七个姓名。\n\n" +
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
		"缺的是姓名列:plot act origin_letter_01 puzzle_names" ZJSEP
		"缺的是粮数:plot act origin_letter_01 puzzle_grain" ZJSEP
		"缺的是日期:plot act origin_letter_01 puzzle_date" ZJSEP
		"缺的是金额:plot act origin_letter_01 puzzle_money\n";
}

private string fight_pei(object me)
{
	object pei;

	if (stage(me) != "face_pei_jiu" || ! in_warehouse(me))
		return "裴九不在这里。";
	pei = present("pei jiu", environment(me));
	if (! objectp(pei) || ! living(pei))
		return recover_ledger(me, "battle");
	me->kill_ob(pei);
	pei->fight_ob(me);
	return "裴九横刀挡在账簿前。击败他后，再从剧情日志中取回账簿。";
}

private string claim_battle(object me)
{
	object pei;

	if (! in_warehouse(me)) return "你不在废仓中。";
	if (PLOT_D->query_flag(me, PLOT_ID, "pei_battle_won"))
		return recover_ledger(me, "battle");
	pei = present("pei jiu", environment(me));
	if (objectp(pei) && living(pei))
		return "裴九仍守着账簿，战斗尚未结束。";
	return recover_ledger(me, "battle");
}

private string persuade_pei(object me)
{
	if (clue_count(me) < 2)
		return "你掌握的论据还不足以让裴九相信雇主已经弃他。";
	return recover_ledger(me, "persuade");
}

private string ransom_ledger(object me)
{
	int cost;

	if (! in_warehouse(me) || stage(me) != "face_pei_jiu")
		return "这里没有可以赎回的账簿。";
	cost = me->query("combat_exp") < 10000 ? 300 :
		(me->query("combat_exp") < 100000 ? 600 : 1000);
	if (! MONEY_D->player_pay(me, cost))
		return "裴九只求" + chinese_number(cost / 100) + "两白银作盘缠。你若无钱，也可说服或战胜他。";
	return recover_ledger(me, "ransom");
}

private string puzzle_hint(object me, string kind)
{
	int level;

	if (stage(me) != "inspect_ledger") return render_log(me);
	level = PLOT_D->query_flag(me, PLOT_ID, "hint_level");
	if (level < 1) level = 0;
	level++;
	PLOT_D->set_flag(me, PLOT_ID, "hint_level", level);
	if (kind == "number")
		return "提示一：二十三户、三十袋粮、二十三枚手印，先把能互相对上的数字划掉，剩下的差额是多少？";
	return "提示二：装订边被整齐裁去的不是一行数字，而是一整列。裴九留下的七个姓名正好填入那道空缺。";
}

private string solve_puzzle(object me, string answer)
{
	if (stage(me) != "inspect_ledger")
		return render_log(me);
	if (answer != "names")
	{
		if (answer == "grain")
			return "若缺的是粮数，便无法解释粮行明确记载的三十袋出库。会馆只有二十三户，差额仍在。";
		if (answer == "date")
			return "日期缺失不能解释为什么三十袋粮只对应二十三枚手印。";
		return "金额并不参与户数、粮袋和手印的守恒关系。再看看被整列裁去的装订边。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "ledger_column_identified", 1);
	PLOT_D->set_flag(me, PLOT_ID, "returning_mark_seen", 1);
	if (! PLOT_D->advance_stage(me, PLOT_ID, "inspect_ledger", "protect_evidence"))
		return "你已经看出答案，但日志暂时没有更新。";
	me->move("/d/city/postofficer");
	return ZJOBLONG "被裁去的是姓名列\n\n"
		"二十三户、三十袋粮、二十三枚手印，多出的七袋粮对应七个被抹去的领粮人。页脚还压着一枚缺一笔的潮纹水印，显然并非普通地方账簿。\n\n" +
		"你刚合上账簿，门外便传来脚步声：花鹞子的人要抢走半张名单，还想放火毁掉‘普通劫匪’的假象。先保住证据，再决定账簿交给谁。\n\n" +
		render_log(me);
}

private string protect_evidence(object me, string method)
{
	if (stage(me) != "protect_evidence") return render_log(me);
	if (method != "guard" && method != "quench") return "没有这个应对办法。";
	PLOT_D->set_flag(me, PLOT_ID, "counteraction", method);
	PLOT_D->set_flag(me, PLOT_ID, "evidence_protected", 1);
	if (! PLOT_D->advance_stage(me, PLOT_ID, "protect_evidence", "choose_custodian"))
		return "证据状态暂时无法保存，请再试一次。";
	return (method == "guard" ? "你护住半张名单，逼退了抢证的人。" :
		"你先扑灭门边火头，再把半张名单压在账簿下。") +
		"证据没有散失，现在可以决定账簿的去向。\n\n" + render_log(me);
}

private string choose_custodian(object me, string choice, int confirmed)
{
	string prompt;

	if (stage(me) != "choose_custodian")
		return render_log(me);
	if (choice == "hall")
		prompt = "交给会馆公开，可以最快逼粮行放粮，却会暴露作保人。";
	else if (choice == "yamen")
		prompt = "密封交给衙门，可以保留完整证据，却无法保证立刻放粮。";
	else if (choice == "grain")
		prompt = "接受粮行私了，可以立即补粮，却会让责任人暂时保住名声。";
	else return "没有这个保管选择。";
	if (! confirmed)
		return ZJOBLONG + prompt + "\n\n这项选择确认后不可改写。\n\n" +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"确认选择:plot act origin_letter_01 confirm_" + choice + ZJSEP +
			"返回日志:plot origin_letter_01\n";
	if (! PLOT_D->record_choice(me, PLOT_ID, "ledger_custodian", choice))
		return "账簿已经有了不同的去向，不能再次改写。";
	if (choice == "hall")
	{
		PLOT_D->set_relation(me, PLOT_ID, "hometown_hall", 2);
		PLOT_D->set_relation(me, PLOT_ID, "yamen", 0);
		PLOT_D->set_relation(me, PLOT_ID, "grain_house", -1);
	}
	else if (choice == "yamen")
	{
		PLOT_D->set_relation(me, PLOT_ID, "hometown_hall", 0);
		PLOT_D->set_relation(me, PLOT_ID, "yamen", 2);
		PLOT_D->set_relation(me, PLOT_ID, "grain_house", 0);
	}
	else
	{
		PLOT_D->set_relation(me, PLOT_ID, "hometown_hall", -1);
		PLOT_D->set_relation(me, PLOT_ID, "yamen", 0);
		PLOT_D->set_relation(me, PLOT_ID, "grain_house", 2);
	}
	PLOT_D->set_flag(me, PLOT_ID, "family_at_completion", current_family(me));
	PLOT_D->advance_stage(me, PLOT_ID, "choose_custodian", "aftermath");
	return prompt + "\n\n你的决定已经写入剧情日志。" +
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
		"查看江湖回响:plot act origin_letter_01 aftermath\n";
}

private string finish_chapter(object me)
{
	object token;
	string choice;
	string result;

	if (status(me) == "completed")
		return render_log(me);
	if (stage(me) != "aftermath")
		return render_log(me);
	choice = me->query(ROOT + "/choices/ledger_custodian");
	if (choice == "hall") result = "会馆借公开账目迫使粮行开仓，乡人先得了粮，作保人的姓名也承受了压力。";
	else if (choice == "yamen") result = "谭友纪封存账簿立案，证据和部分姓名得到保护，救济却还要等候。";
	else result = "粮行当夜补足粮食，眼前危机暂解，真正责任仍埋在未公开的账页中。";
	if (! PLOT_D->query_flag(me, PLOT_ID, "score_claimed"))
	{
		me->add("score", 20);
		PLOT_D->set_flag(me, PLOT_ID, "score_claimed", 1);
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "reward_claimed"))
	{
		if (! present("old post token", me))
		{
			token = new(TOKEN);
			if (objectp(token) && token->move(me, 1))
				PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
		}
		else PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	}
	if (! PLOT_D->complete_chapter(me, PLOT_ID, "aftermath"))
		return "结算已经发生，但完成状态暂时无法保存，请再次查看回响。";
	instance_left(me);
	PLOT_D->set_arc_value(me, "current_chapter", 2);
	return ZJOBLONG + result + "\n\n"
		"你合上旧驿木牌，忽然明白：出身只说明你从哪里来，师门也只说明谁曾教你。真正传到江湖上的，终究是你亲手做下的选择。\n\n"
		HIG "第一章「一纸乡书」完成。你获得二十点江湖阅历和旧驿木牌。" NOR "\n";
}

private string claim_token(object me)
{
	object token;

	if (status(me) != "completed") return "第一章尚未完成，暂时不能补领旧驿木牌。";
	if (present("old post token", me)) return "旧驿木牌还在你身上。";
	token = new(TOKEN);
	if (! objectp(token) || ! token->move(me, 1))
	{
		if (objectp(token)) destruct(token);
		return "你身上没有空位。清出一格后，再用‘补领旧驿木牌’即可。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	return "杜宽替你从旧件箱中补出一块旧驿木牌。";
}

void instance_left(object me)
{
	object room;
	object ob;

	if (! objectp(me)) return;
	room = query_instance(me);
	if (! objectp(room) || environment(me) == room) return;
	foreach (ob in all_inventory(room))
		if (objectp(ob) && ob != me) destruct(ob);
	PLOT_D->close_instance(me, PLOT_ID);
}

string render_log(object me)
{
	string msg;
	string current;
	int count;

	if (! objectp(me)) return "";
	PLOT_D->ensure_player_state(me);
	current = stage(me);
	count = clue_count(me);
	msg = "来处：" + (stringp(PLOT_D->query_flag(me, PLOT_ID, "origin_label")) ?
		PLOT_D->query_flag(me, PLOT_ID, "origin_label") : "尚未记录") + "\n";
	if (status(me) == "available")
		return msg + "杜宽处有一封同乡旧信。\n" + ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"查看旧信:plot act origin_letter_01 notice" ZJSEP
			"接下调查:plot act origin_letter_01 accept\n";
	if (status(me) == "completed")
		return msg + "取得账簿：" + method_label(me->query(ROOT + "/flags/ledger_method")) +
			"\n账簿去向：" + custodian_label(me->query(ROOT + "/choices/ledger_custodian")) +
			"\n" + (present("old post token", me) ? "纪念物：旧驿木牌在身上。" : "纪念物：可向杜宽补领旧驿木牌。") +
			"\n" + ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"补领旧驿木牌:plot act origin_letter_01 claim_token\n";
	if (current == "accept_letter" || current == "hear_three_sides")
		return msg + sprintf("已核实证词：%d/3\n", count) + begin_inquiry(me);
	if (current == "trace_warehouse")
		return msg + sprintf("已核实证词：%d/3。目标：前往扬州北门追踪废仓。\n", count) +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) + "追踪废仓:plot act origin_letter_01 trace\n";
	if (current == "face_pei_jiu")
		return msg + "目标：在废仓中从裴九手里取回账簿。\n" +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"进入废仓:plot act origin_letter_01 enter" ZJSEP
			"正面交手:plot act origin_letter_01 fight" ZJSEP
			"战后取簿:plot act origin_letter_01 claim_battle" ZJSEP
			"以证词说服:plot act origin_letter_01 persuade" ZJSEP
			"小额赎回:plot act origin_letter_01 ransom" ZJSEP
			"离开废仓:plot act origin_letter_01 leave\n";
	if (current == "inspect_ledger")
		return msg + "谜题：二十三户、三十袋粮、二十三枚手印，账簿被裁去哪一列？\n" +
			ZJOBACTS2 + ZJMENUF(2, 2, 8, 30) +
			"姓名列:plot act origin_letter_01 puzzle_names" ZJSEP
			"粮数:plot act origin_letter_01 puzzle_grain" ZJSEP
			"日期:plot act origin_letter_01 puzzle_date" ZJSEP
			"金额:plot act origin_letter_01 puzzle_money" ZJSEP
			"数字提示:plot act origin_letter_01 hint_number" ZJSEP
			"装订提示:plot act origin_letter_01 hint_column\n";
	if (current == "protect_evidence")
		return msg + "花鹞子的人正在抢夺半张名单并纵火毁证。\n" +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"护住姓名:plot act origin_letter_01 guard_names" ZJSEP
			"先行灭火:plot act origin_letter_01 quench_fire\n";
	if (current == "choose_custodian")
		return msg + "目标：决定由谁保管义仓簿。\n" +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"会馆公开:plot act origin_letter_01 choose_hall" ZJSEP
			"衙门封存:plot act origin_letter_01 choose_yamen" ZJSEP
			"粮行私了:plot act origin_letter_01 choose_grain\n";
	return msg + "目标：查看账簿去向造成的江湖回响。\n" +
		ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) + "查看回响:plot act origin_letter_01 aftermath\n";
}

string npc_notice(object me)
{
	if (status(me) == "available")
		return notice(me);
	return render_log(me);
}

mixed handle_action(object me, string action)
{
	if (! objectp(me) || ! userp(me)) return "无效的剧情参与者。";
	PLOT_D->ensure_player_state(me);
	if (action == "notice") return notice(me);
	if (action == "accept") return accept_letter(me);
	if (action == "begin_inquiry") return begin_inquiry(me);
	if (action == "go_hall") return go_room(me, "/d/plot/origin_letter/hall", "你来到同乡会馆，许三娘正等着说明账簿之事。");
	if (action == "go_yamen") return go_room(me, "/d/city/ymzhengting", "你来到衙门正厅，谭友纪已经调出封存卷宗。");
	if (action == "go_grain") return go_room(me, "/d/plot/origin_letter/grain_house", "你来到粮行账房，周守义让人关上了门。");
	if (action == "hear_hall") return hear_clue(me, "hall");
	if (action == "hear_yamen") return hear_clue(me, "yamen");
	if (action == "hear_grain") return hear_clue(me, "grain");
	if (action == "trace") return trace_warehouse(me);
	if (action == "enter") return enter_instance(me);
	if (action == "fight") return fight_pei(me);
	if (action == "claim_battle") return claim_battle(me);
	if (action == "persuade") return persuade_pei(me);
	if (action == "ransom") return ransom_ledger(me);
	if (action == "puzzle_names") return solve_puzzle(me, "names");
	if (action == "puzzle_grain") return solve_puzzle(me, "grain");
	if (action == "puzzle_date") return solve_puzzle(me, "date");
	if (action == "puzzle_money") return solve_puzzle(me, "money");
	if (action == "hint_number") return puzzle_hint(me, "number");
	if (action == "hint_column") return puzzle_hint(me, "column");
	if (action == "guard_names") return protect_evidence(me, "guard");
	if (action == "quench_fire") return protect_evidence(me, "quench");
	if (action == "choose_hall") return choose_custodian(me, "hall", 0);
	if (action == "choose_yamen") return choose_custodian(me, "yamen", 0);
	if (action == "choose_grain") return choose_custodian(me, "grain", 0);
	if (action == "confirm_hall") return choose_custodian(me, "hall", 1);
	if (action == "confirm_yamen") return choose_custodian(me, "yamen", 1);
	if (action == "confirm_grain") return choose_custodian(me, "grain", 1);
	if (action == "aftermath") return finish_chapter(me);
	if (action == "claim_token") return claim_token(me);
	if (action == "leave")
	{
		string safe;

		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/beimen";
		if (in_warehouse(me)) me->move(safe);
		instance_left(me);
		return "你离开废仓，回到安全处。";
	}
	return "这项剧情操作不存在。";
}
