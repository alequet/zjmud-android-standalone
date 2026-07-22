// Chapter four: Visitors From Home.

#include <ansi.h>

#define PLOT_ID "visitors_from_home_04"
#define ROOT "plot/visitors_from_home_04"
#define RELAY "/d/plot/visitors_from_home/relay_court"
#define ROAD "/d/plot/visitors_from_home/post_road"
#define STONE "/clone/plot/visitors_from_home/stone_seven"
#define HUA "/clone/plot/visitors_from_home/hua_yaozi"
#define INVITATION "/clone/plot/visitors_from_home/salt_invitation"

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

private string origin_group(object me)
{
	string group;

	group = me->query("plot/origin_letter_01/flags/origin_group");
	if (! stringp(group) || group == "") group = "compat";
	return group;
}

private string local_proof(string group)
{
	if (group == "north") return "关隘印应压在折角内侧，伪信却盖在封口外。";
	if (group == "central") return "义仓编号应按渡口顺序，伪信却照官道排序。";
	if (group == "south") return "水路平码必须连着潮次，伪信只有平码。";
	if (group == "southeast") return "商帮货签一向留双结，伪信只打了单结。";
	if (group == "southwest") return "药材包记录山道节点，伪信误写成里程。";
	if (group == "west") return "异文货签应从右侧起笔，伪信照中原格式抄写。";
	if (group == "family") return "旧驿纹只刻半纹，伪信上的完整家纹反而是假。";
	return "杜宽收件簿证明纸张入库晚于信上日期。";
}

private int in_instance(object me)
{
	string room;

	if (! objectp(environment(me))) return 0;
	room = base_name(environment(me));
	return environment(me)->query("plot_owner") == me->query("id") &&
		(room == RELAY || room == ROAD);
}

private object query_relay(object me)
{
	return PLOT_D->query_instance(me, PLOT_ID);
}

private object query_road(object me)
{
	return PLOT_D->query_instance_room(me, PLOT_ID, "post_road");
}

private object ensure_stone(object me, object room)
{
	object stone;
	string condition;

	stone = present("lu xiaoshuan", room);
	condition = PLOT_D->query_flag(me, PLOT_ID, "stone_condition");
	if (objectp(stone))
	{
		room->set("plot_spawned/stone_seven", 1);
		if (stringp(condition)) stone->restore_plot_condition(condition);
		return stone;
	}
	if (room->query("plot_spawned/stone_seven")) return 0;
	stone = new(STONE);
	if (! objectp(stone)) return 0;
	stone->set("plot_owner", me->query("id"));
	stone->move(room);
	room->set("plot_spawned/stone_seven", 1);
	if (stringp(condition)) stone->restore_plot_condition(condition);
	return stone;
}

private object ensure_hua(object me, object room)
{
	object hua;

	if (PLOT_D->query_flag(me, PLOT_ID, "hua_resolved")) return 0;
	hua = present("hua yaozi", room);
	if (objectp(hua) && living(hua))
	{
		room->set("plot_spawned/hua_yaozi", 1);
		PLOT_COMBAT_D->restore_enemy(hua);
		return hua;
	}
	if (room->query("plot_spawned/hua_yaozi")) return 0;
	hua = new(HUA);
	if (! objectp(hua)) return 0;
	hua->set("plot_owner", me->query("id"));
	if (! mapp(PLOT_COMBAT_D->configure_enemy(hua, me, "courier_scout", 0)))
	{
		destruct(hua);
		return 0;
	}
	hua->move(room);
	room->set("plot_spawned/hua_yaozi", 1);
	return hua;
}

private object create_relay(object me)
{
	object room;

	room = query_relay(me);
	if (! objectp(room))
	{
		room = PLOT_D->open_instance(me, PLOT_ID, RELAY, "/d/city/postofficer");
		if (! objectp(room)) return 0;
	}
	ensure_stone(me, room);
	return room;
}

private object create_road(object me)
{
	object room;

	room = query_road(me);
	if (! objectp(room))
	{
		room = PLOT_D->open_instance_room(me, PLOT_ID, "post_road", ROAD);
		if (! objectp(room)) return 0;
		room->set("exits/out", "/d/city/beimen");
	}
	if (stage(me) == "catch_hua_yaozi") ensure_hua(me, room);
	return room;
}

private string enter_relay(object me)
{
	object room;

	room = create_relay(me);
	if (! objectp(room)) return "驿站后院暂时无法打开，请稍后再试。";
	me->move(room);
	return "杜宽领你绕过信柜，来到堆着三只信匣的驿站后院。";
}

private string enter_road(object me)
{
	object room;

	room = create_road(me);
	if (! objectp(room)) return "北门外旧驿路暂时无法打开，请稍后再试。";
	me->move(room);
	return "你循着被换下的信封追到北门外，花鹞子正在旧驿路界碑前拆毁请帖。";
}

private string begin_letters(object me)
{
	string here;

	if (status(me) == "available")
	{
		if (! PLOT_D->begin_chapter(me, PLOT_ID)) return "第四章暂时无法接取，请稍后再试。";
	}
	if (status(me) != "active" || stage(me) != "letters_arrive") return render_log(me);
	here = objectp(environment(me)) ? base_name(environment(me)) : "/d/city/postofficer";
	if (! PLOT_D->query_flag(me, PLOT_ID, "safe_return"))
		PLOT_D->set_flag(me, PLOT_ID, "safe_return", here);
	PLOT_D->set_flag(me, PLOT_ID, "origin_group", origin_group(me));
	return ZJOBLONG "故园来客\n\n"
		"同一天送到扬州的三封乡书都借你的名义作保：一封催粮，一封要求释放船户，一封声称已有故乡来客抵达。"
		"三封信使用相似格式，日期、纸张和寄路却不能同时成立。先逐封查看。\n\n" + render_log(me);
}

private string inspect_letter(object me, string source)
{
	int count;
	string text;

	if (stage(me) != "letters_arrive") return render_log(me);
	if (source == "relief")
	{
		PLOT_D->set_flag(me, PLOT_ID, "letter_relief_seen", 1);
		text = "催粮信使用了正确的地方格式，却写在三天后才入库的杭州纸上。";
	}
	else if (source == "boatmen")
	{
		PLOT_D->set_flag(me, PLOT_ID, "letter_boatmen_seen", 1);
		text = "释放船户的信盖着回浪印，寄出日却早于二十三号夜船靠岸。";
	}
	else if (source == "visitor")
	{
		PLOT_D->set_flag(me, PLOT_ID, "letter_visitor_seen", 1);
		text = "来客信封夹层藏着七字短句，装订缺口与第一章被裁去的七个人名相合。";
	}
	else return "没有这封乡书。";
	count = (PLOT_D->query_flag(me, PLOT_ID, "letter_relief_seen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "letter_boatmen_seen") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "letter_visitor_seen") ? 1 : 0);
	PLOT_D->set_flag(me, PLOT_ID, "letters_seen", count);
	if (count >= 3)
		PLOT_D->advance_stage(me, PLOT_ID, "letters_arrive", "meet_lu_xiaoshuan");
	return text + "\n\n" + (count >= 3 ? "三封信都在借你的来处说话。现在去后院见那名来客。\n\n" : "") + render_log(me);
}

string question_visitor(object me)
{
	if (! objectp(me) || stage(me) != "meet_lu_xiaoshuan" || ! in_instance(me) ||
		base_name(environment(me)) != RELAY) return render_log(me);
	PLOT_D->set_flag(me, PLOT_ID, "visitor_asks_evidence", 1);
	PLOT_D->set_flag(me, PLOT_ID, "hangzhou_sand_seen", 1);
	PLOT_D->set_flag(me, PLOT_ID, "visitor_statement", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "meet_lu_xiaoshuan", "verify_letters");
	return "“陆小栓”说得出正确的乡书格式，却只追问你保存了哪些账簿和存根；他鞋底的细白沙来自杭州塔边，"
		"并非信上所写的北门泥路。\n\n" + render_log(me);
}

void record_stone_injury(object stone)
{
	object me;
	string owner;

	if (! objectp(stone) || base_name(stone) != STONE) return;
	owner = stone->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || environment(me) != environment(stone)) return;
	PLOT_D->set_flag(me, PLOT_ID, "stone_condition", "injured");
	PLOT_D->set_flag(me, PLOT_ID, "hangzhou_wrapper_seen", 1);
}

private string verify_fact(object me, string fact)
{
	int count;
	string text;

	if (stage(me) != "verify_letters") return render_log(me);
	if (fact == "timeline")
	{
		PLOT_D->set_flag(me, PLOT_ID, "proof_timeline", 1);
		text = "你按寄出、入库和留宿时间重排三封信：来客信寄出时，真正陆小栓仍登记在沿途驿站。";
	}
	else if (fact == "travel")
	{
		PLOT_D->set_flag(me, PLOT_ID, "proof_travel", 1);
		text = "你把鞋底细沙与封套纸行对上：眼前来客最近从杭州来，不是从信中自称的来处赶来。";
	}
	else if (fact == "format")
		return "格式可以照旧信临摹，不能单独证明送信人身份。";
	else if (fact == "accent")
		return "口误可能来自紧张或长途跋涉，不能解释纸张时间和鞋底来路。";
	else return "没有这项核验事实。";
	count = (PLOT_D->query_flag(me, PLOT_ID, "proof_timeline") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "proof_travel") ? 1 : 0);
	if (count >= 2)
	{
		PLOT_D->set_flag(me, PLOT_ID, "forgery_known", 1);
		PLOT_D->set_flag(me, PLOT_ID, "stone_identity", "stone_seven");
		PLOT_D->advance_stage(me, PLOT_ID, "verify_letters", "choose_immediate_action");
		text += "\n\n两条独立事实同时成立：所谓陆小栓是无面局换名人石七。花鹞子已经带着旧盐仓请帖逃向北门。";
	}
	return text + "\n\n" + render_log(me);
}

private string choose_action(object me, string method)
{
	string text;

	if (stage(me) != "choose_immediate_action") return render_log(me);
	if (method == "protect")
	{
		PLOT_D->set_flag(me, PLOT_ID, "evidence_testimony", 1);
		text = "你先让许三娘封住后院，保全三封信和石七的完整口供，再从信封去向追赶花鹞子。";
	}
	else if (method == "chase")
	{
		PLOT_D->set_flag(me, PLOT_ID, "road_observed", 1);
		PLOT_D->set_flag(me, PLOT_ID, "evidence_full_invitation", 1);
		text = "你立刻追出北门，提前看清花鹞子在两股驿路间预留的退路。许三娘负责看住后院。";
	}
	else if (method == "decoy")
	{
		PLOT_D->set_flag(me, PLOT_ID, "decoy_ready", 1);
		PLOT_D->set_flag(me, PLOT_ID, "evidence_employer_word", 1);
		text = "你让杜宽放出假回信，声称愿用存根换人。花鹞子果然改口提到“沈先生”的杭州纸行。";
	}
	else return "没有这种优先行动。";
	PLOT_D->set_flag(me, PLOT_ID, "immediate_action", method);
	PLOT_D->advance_stage(me, PLOT_ID, "choose_immediate_action", "catch_hua_yaozi");
	return text + "\n\n" + enter_road(me) + "\n\n" + render_log(me);
}

private int settle_hua(object me, string method, string fate, int quality)
{
	object hua;

	PLOT_D->set_flag(me, PLOT_ID, "hua_method", method);
	PLOT_D->set_flag(me, PLOT_ID, "hua_fate", fate);
	PLOT_D->set_flag(me, PLOT_ID, "hua_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "invitation_quality", quality);
	hua = present("hua yaozi", environment(me));
	if (objectp(hua)) destruct(hua);
	return PLOT_D->advance_stage(me, PLOT_ID, "catch_hua_yaozi", "decode_invitation");
}

void record_hua_defeat(object hua)
{
	object me;
	object room;
	string owner;

	if (! objectp(hua) || base_name(hua) != HUA) return;
	room = environment(hua);
	owner = hua->query("plot_owner");
	if (! stringp(owner) || owner == "") return;
	me = find_player(owner);
	if (! objectp(me) || ! objectp(room) || environment(me) != room ||
		room->query("plot_owner") != owner || stage(me) != "catch_hua_yaozi") return;
	PLOT_D->set_flag(me, PLOT_ID, "hua_method", "fight");
	PLOT_D->set_flag(me, PLOT_ID, "hua_fate", "defeated");
	PLOT_D->set_flag(me, PLOT_ID, "hua_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "invitation_quality", 2);
	PLOT_D->advance_stage(me, PLOT_ID, "catch_hua_yaozi", "decode_invitation");
}

private string resolve_hua(object me, string method)
{
	object hua;

	if (stage(me) != "catch_hua_yaozi") return render_log(me);
	if (! in_instance(me) || base_name(environment(me)) != ROAD)
		return "必须先追到北门外旧驿路。";
	hua = present("hua yaozi", environment(me));
	if (method == "fight")
	{
		if (! objectp(hua) || ! living(hua)) return render_log(me);
		me->kill_ob(hua);
		hua->kill_ob(me);
		return "花鹞子踏上界碑拔身欲走。击败他后，请帖事实会先写入剧情状态。";
	}
	if (method == "block")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "road_observed"))
			return "两股驿路都有退路。先查看浅沟、废亭和界碑留下的脚印。";
		settle_hua(me, "block", "restrained", 2);
		return "你用浅沟截马、废亭横梁封墙，谭友纪带人从官道合围。花鹞子只得交出完整请帖。\n\n" + render_log(me);
	}
	if (method == "lure")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "decoy_ready"))
			return "花鹞子不会凭一句空话回头。先让杜宽准备带错批号的假回信。";
		settle_hua(me, "lure", "exposed", 3);
		PLOT_D->set_flag(me, PLOT_ID, "evidence_employer_word", 1);
		return "假回信故意写错一个批号，花鹞子脱口纠正并说出杭州纸行的交割口风，随后被堵在界碑前。\n\n" + render_log(me);
	}
	return "没有这种截帖办法。";
}

private string decode_invitation(object me, string field, string value)
{
	if (stage(me) != "decode_invitation") return render_log(me);
	if (field == "place")
	{
		if (value != "salt") return "请帖边缘有盐霜和七号仓旧钉孔，不是粮行新仓。";
		PLOT_D->set_flag(me, PLOT_ID, "decoded_place", "old_salt_storehouse");
	}
	else if (field == "tide")
	{
		if (value != "second") return "回浪印末笔向下，必须对应二潮，不是早潮。";
		PLOT_D->set_flag(me, PLOT_ID, "decoded_tide", "second_tide");
	}
	else return "没有这项请帖字段。";
	if (PLOT_D->query_flag(me, PLOT_ID, "decoded_place") &&
		PLOT_D->query_flag(me, PLOT_ID, "decoded_tide"))
	{
		PLOT_D->set_flag(me, PLOT_ID, "meeting_place_known", 1);
		PLOT_D->set_arc_flag(me, "meeting_place_known", 1);
		PLOT_D->set_arc_flag(me, "real_lu_captive_known", 1);
		PLOT_D->advance_stage(me, PLOT_ID, "decode_invitation", "prepare_for_meeting");
		return "七号旧盐仓、二潮后开侧门。请帖背面还写着“百川总簿”，石七终于承认真陆小栓被关在仓内。\n\n" + render_log(me);
	}
	return "这一项已经对上，再核验另一项。\n\n" + render_log(me);
}

private int evidence_allowed(object me, string evidence)
{
	if (evidence == "letters" || evidence == "invitation" || evidence == "stub") return 1;
	if (evidence == "testimony") return PLOT_D->query_flag(me, PLOT_ID, "evidence_testimony");
	if (evidence == "employer") return PLOT_D->query_flag(me, PLOT_ID, "evidence_employer_word");
	return 0;
}

private string prepare_evidence(object me, string evidence)
{
	int count;

	if (stage(me) != "prepare_for_meeting") return render_log(me);
	if (! evidence_allowed(me, evidence)) return "你目前没有这项可核验的证据。";
	if (PLOT_D->query_flag(me, PLOT_ID, "prepared_" + evidence))
		return "这项证据已经放入会面卷袋。";
	count = PLOT_D->query_flag(me, PLOT_ID, "evidence_prepared");
	if (count >= 2) return "卷袋只准备两项展示证据。先按当前两项进入会面；它们不锁定第五章路线。";
	PLOT_D->set_flag(me, PLOT_ID, "prepared_" + evidence, 1);
	PLOT_D->set_flag(me, PLOT_ID, "evidence_prepared", count + 1);
	return "你把这项证据放入会面卷袋。已准备 " + sprintf("%d/2", count + 1) + "。\n";
}

private string confirm_preparation(object me)
{
	if (stage(me) != "prepare_for_meeting") return render_log(me);
	if (PLOT_D->query_flag(me, PLOT_ID, "evidence_prepared") < 2)
		return "至少准备两项证据，避免旧盐仓开场只剩单方说法。";
	PLOT_D->set_flag(me, PLOT_ID, "preparation_confirmed", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "prepare_for_meeting", "chapter_close");
	return "两项证据已经封入卷袋。它们只改变第五章开场便利，不形成永久路线。\n\n" + render_log(me);
}

private string close_chapter(object me)
{
	object invitation;

	if (stage(me) != "chapter_close") return render_log(me);
	if (in_instance(me))
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/postofficer";
		me->move(safe);
	}
	if (! PLOT_D->complete_chapter(me, PLOT_ID, "chapter_close"))
		return "旧盐仓会面已经查明，但章节完成状态暂时无法保存，请再试一次。";
	if (! PLOT_D->query_flag(me, PLOT_ID, "reward_claimed"))
	{
		if (present("salt invitation", me))
			PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
		else
		{
			invitation = new(INVITATION);
			if (objectp(invitation) && invitation->move(me, 1))
				PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
			else if (objectp(invitation)) destruct(invitation);
		}
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "score_claimed"))
	{
		me->add("score", 18);
		PLOT_D->set_flag(me, PLOT_ID, "score_claimed", 1);
	}
	PLOT_D->set_arc_flag(me, "meeting_place_known", 1);
	PLOT_D->set_arc_flag(me, "real_lu_captive_known", 1);
	PLOT_D->set_arc_flag(me, "identify_home_letters", 1);
	PLOT_D->set_arc_value(me, "current_chapter", 5);
	instance_left(me);
	return ZJOBLONG "故园来客\n\n"
		"三封乡书的地方格式只是可复制的外壳，时间、纸张和独立行程才证明送信人是谁。石七冒用了陆小栓的身份，"
		"花鹞子则把旧盐仓请帖送给仍想控制百川总簿的人。\n\n"
		"第四章「故园来客」完成。真正陆小栓仍在七号旧盐仓，二潮后的会面已记入日志。\n";
}

private string claim_invitation(object me)
{
	object invitation;

	if (status(me) != "completed") return "第四章尚未完成。";
	if (present("salt invitation", me)) return "旧盐仓请帖残件还在你身上。";
	invitation = new(INVITATION);
	if (! objectp(invitation) || ! invitation->move(me, 1))
	{
		if (objectp(invitation)) destruct(invitation);
		return "你身上没有空位，清出一格后再补领请帖残件。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	return "杜宽根据核验记录替你补出一张旧盐仓请帖残件。";
}

void instance_left(object me)
{
	object relay;
	object road;
	object ob;

	if (! objectp(me)) return;
	if (in_instance(me)) return;
	relay = query_relay(me);
	road = query_road(me);
	foreach (ob in (objectp(relay) ? all_inventory(relay) : ({})))
		if (objectp(ob) && ob != me) destruct(ob);
	foreach (ob in (objectp(road) ? all_inventory(road) : ({})))
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
		return "杜宽处同日压着三封互相矛盾的乡书。开始：plot act visitors_from_home_04 begin\n";
	if (status(me) == "completed")
		return "假陆小栓已识破，七号旧盐仓与二潮会面已确认。补领：plot act visitors_from_home_04 claim_invitation\n";
	if (current == "letters_arrive")
	{
		count = PLOT_D->query_flag(me, PLOT_ID, "letters_seen");
		return sprintf("已查看乡书：%d/3。分别核对催粮、船户和来客三封信。\n", count);
	}
	if (current == "meet_lu_xiaoshuan") return "三封信不能同时为真。进入驿站后院，询问自称陆小栓的来客。\n";
	if (current == "verify_letters") return "辨伪必须取得两条独立事实。地方依据：" + local_proof(origin_group(me)) + "\n";
	if (current == "choose_immediate_action") return "石七身份已经识破。选择先保护后院、立即追信使或布置假回信；其余目标由三方暂时承担。\n";
	if (current == "catch_hua_yaozi") return "花鹞子在旧驿路毁帖。可正面交手、观察退路后围堵，或准备假回信诱捕。\n";
	if (current == "decode_invitation") return "请帖需要核对地点与潮时：七号旧盐仓还是粮行新仓，二潮还是早潮。\n";
	if (current == "prepare_for_meeting")
		return sprintf("已准备证据：%d/2。可选乡书时间轴、请帖、改签存根，以及本路线取得的口供。\n",
			PLOT_D->query_flag(me, PLOT_ID, "evidence_prepared"));
	if (current == "chapter_close") return "证据卷袋已经封好。返回剧情日志完成第四章结算。\n";
	return "目标：识破借用你故乡名义的假信使。\n";
}

string npc_notice(object me)
{
	if (! objectp(me)) return "";
	if (status(me) == "available" || (status(me) == "active" && stage(me) == "letters_arrive"))
		return begin_letters(me);
	return render_log(me);
}

mixed handle_action(object me, string action)
{
	if (! objectp(me) || ! userp(me)) return "无效的剧情参与者。";
	PLOT_D->ensure_player_state(me);
	if (action == "begin" || action == "letters_arrive") return begin_letters(me);
	if (action == "inspect_relief") return inspect_letter(me, "relief");
	if (action == "inspect_boatmen") return inspect_letter(me, "boatmen");
	if (action == "inspect_visitor") return inspect_letter(me, "visitor");
	if (action == "meet_visitor") return enter_relay(me);
	if (action == "question_visitor") return question_visitor(me);
	if (action == "verify_timeline") return verify_fact(me, "timeline");
	if (action == "verify_travel") return verify_fact(me, "travel");
	if (action == "verify_format") return verify_fact(me, "format");
	if (action == "verify_accent") return verify_fact(me, "accent");
	if (action == "choose_protect") return choose_action(me, "protect");
	if (action == "choose_chase") return choose_action(me, "chase");
	if (action == "choose_decoy") return choose_action(me, "decoy");
	if (action == "observe_road")
	{
		if (stage(me) != "catch_hua_yaozi" || ! in_instance(me)) return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "road_observed", 1);
		return "你看清浅沟能截马，废亭横梁能封住翻墙退路，界碑后只剩官道出口。";
	}
	if (action == "prepare_decoy")
	{
		if (stage(me) != "catch_hua_yaozi") return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "decoy_ready", 1);
		return "杜宽按伪信笔迹写好一封带错批号的回信，只等花鹞子开口纠正。";
	}
	if (action == "fight_hua") return resolve_hua(me, "fight");
	if (action == "block_hua") return resolve_hua(me, "block");
	if (action == "lure_hua") return resolve_hua(me, "lure");
	if (action == "decode_place_salt") return decode_invitation(me, "place", "salt");
	if (action == "decode_place_grain") return decode_invitation(me, "place", "grain");
	if (action == "decode_tide_second") return decode_invitation(me, "tide", "second");
	if (action == "decode_tide_first") return decode_invitation(me, "tide", "first");
	if (action == "prepare_letters") return prepare_evidence(me, "letters");
	if (action == "prepare_invitation") return prepare_evidence(me, "invitation");
	if (action == "prepare_stub") return prepare_evidence(me, "stub");
	if (action == "prepare_testimony") return prepare_evidence(me, "testimony");
	if (action == "prepare_employer") return prepare_evidence(me, "employer");
	if (action == "confirm_preparation") return confirm_preparation(me);
	if (action == "chapter_close" || action == "aftermath") return close_chapter(me);
	if (action == "claim_invitation") return claim_invitation(me);
	if (action == "enter")
	{
		if (stage(me) == "meet_lu_xiaoshuan" || stage(me) == "verify_letters") return enter_relay(me);
		if (stage(me) == "catch_hua_yaozi" || stage(me) == "decode_invitation") return enter_road(me);
		return render_log(me);
	}
	if (action == "leave")
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = "/d/city/postofficer";
		if (in_instance(me)) me->move(safe);
		instance_left(me);
		return "你离开查信现场，回到扬州安全处。";
	}
	return "这项剧情操作不存在。";
}
