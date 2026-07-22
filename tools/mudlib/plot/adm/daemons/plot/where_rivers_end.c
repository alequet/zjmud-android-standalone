// Chapter five: Where Rivers End.

#include <ansi.h>

#define PLOT_ID "where_rivers_end_05"
#define ROOT "plot/where_rivers_end_05"
#define SAFE_ROOM "/d/city/beimen"
#define SALT_GATE "/d/plot/where_rivers_end/salt_gate"
#define DRAIN "/d/plot/where_rivers_end/drain_tunnel"
#define WATER_GATE "/d/plot/where_rivers_end/water_gate"
#define INSPECTION "/d/plot/where_rivers_end/inspection_yard"
#define GALLERY "/d/plot/where_rivers_end/outer_gallery"
#define ASSEMBLY "/d/plot/where_rivers_end/assembly_floor"
#define CELL "/d/plot/where_rivers_end/dark_cell"
#define GUARD_WALK "/d/plot/where_rivers_end/guard_walk"
#define BURNING "/d/plot/where_rivers_end/burning_archive"
#define INDEX_ROOM "/d/plot/where_rivers_end/index_room"
#define SEAL_ROOM "/d/plot/where_rivers_end/seal_chamber"
#define REAL_LU "/clone/plot/where_rivers_end/real_lu_xiaoshuan"
#define LUO "/clone/plot/where_rivers_end/luo_qiniang"
#define WEN "/clone/plot/where_rivers_end/wen_shouzhuo"
#define MEMORIAL "/clone/plot/where_rivers_end/hometown_memorial"

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

private string *instance_blueprints()
{
	return ({ SALT_GATE, DRAIN, WATER_GATE, INSPECTION, GALLERY, ASSEMBLY,
		CELL, GUARD_WALK, BURNING, INDEX_ROOM, SEAL_ROOM });
}

private int in_instance(object me)
{
	if (! objectp(environment(me))) return 0;
	return environment(me)->query("plot_owner") == me->query("id") &&
		member_array(base_name(environment(me)), instance_blueprints()) >= 0;
}

private object query_room(object me, string slot)
{
	if (slot == "salt_gate") return PLOT_D->query_instance(me, PLOT_ID);
	return PLOT_D->query_instance_room(me, PLOT_ID, slot);
}

private string blueprint(string slot)
{
	if (slot == "salt_gate") return SALT_GATE;
	if (slot == "drain_tunnel") return DRAIN;
	if (slot == "water_gate") return WATER_GATE;
	if (slot == "inspection_yard") return INSPECTION;
	if (slot == "outer_gallery") return GALLERY;
	if (slot == "assembly_floor") return ASSEMBLY;
	if (slot == "dark_cell") return CELL;
	if (slot == "guard_walk") return GUARD_WALK;
	if (slot == "burning_archive") return BURNING;
	if (slot == "index_room") return INDEX_ROOM;
	if (slot == "seal_chamber") return SEAL_ROOM;
	return 0;
}

private object ensure_room(object me, string slot)
{
	object room;
	string source;

	room = query_room(me, slot);
	if (objectp(room)) return room;
	source = blueprint(slot);
	if (! stringp(source)) return 0;
	if (slot == "salt_gate")
		return PLOT_D->open_instance(me, PLOT_ID, source, SAFE_ROOM);
	return PLOT_D->open_instance_room(me, PLOT_ID, slot, source);
}

private string enter_room(object me, string slot, string text)
{
	object room;

	/* The primary clone owns the instance; every route also creates it. */
	if (! objectp(ensure_room(me, "salt_gate")))
		return "旧盐仓实例暂时无法打开，请稍后再试。";
	room = ensure_room(me, slot);
	if (! objectp(room)) return "旧盐仓这处通道暂时无法打开，请稍后再试。";
	me->move(room);
	return text;
}

private int joint_entry_available(object me)
{
	int count;
	string key;

	foreach (key in ({ "hometown_hall", "yamen", "grain_house" }))
		if (me->query("plot/arc/hometown_letters_01/relations/" + key) >= 0) count++;
	return count >= 2;
}

private string entry_slot(string method)
{
	if (method == "invitation") return "salt_gate";
	if (method == "boatmen") return "water_gate";
	if (method == "joint") return "inspection_yard";
	return "drain_tunnel";
}

private string entry_text(string method)
{
	if (method == "invitation")
		return "你递上二潮请帖，从盐仓正门进入，可以先听见集会第一轮争执。";
	if (method == "boatmen")
		return "柳大艄从水门暗栓接应你，先把一名被扣船工送出仓外。";
	if (method == "joint")
		return "会馆、衙门和粮行的人在外院相互监看，为你牵制了外围护卫。";
	return "你沿退潮后的排水道钻入旧盐仓，必须自己扳开锈死的潮闸。";
}

private string begin_chapter(object me)
{
	string here;

	if (status(me) == "available")
	{
		if (! PLOT_D->begin_chapter(me, PLOT_ID))
			return "第五章暂时无法接取，请稍后再试。";
	}
	if (status(me) != "active" || stage(me) != "enter_salt_storehouse")
		return render_log(me);
	here = objectp(environment(me)) ? base_name(environment(me)) : SAFE_ROOM;
	if (! PLOT_D->query_flag(me, PLOT_ID, "safe_return"))
		PLOT_D->set_flag(me, PLOT_ID, "safe_return", here);
	return ZJOBLONG "百川归处\n\n"
		"七号旧盐仓在二潮后开门。百川会把救命粮、欠据和被改写的人名压在同一本总簿里，"
		"真正陆小栓也被囚在仓内。你可以使用已有便利，也可以走不依赖关系的排水道。\n\n" + render_log(me);
}

private string choose_entry(object me, string method)
{
	string saved;

	if (stage(me) != "enter_salt_storehouse") return render_log(me);
	saved = PLOT_D->query_flag(me, PLOT_ID, "entry_method");
	if (stringp(saved) && saved != method)
		return "入场方式已经确定。退出或强停后会从同一路线恢复；实例损坏时仍可改走排水道。";
	if (method == "invitation" &&
		me->query("plot/visitors_from_home_04/flags/invitation_quality") < 2)
		return "残缺请帖不足以通过正门核验，可以改走排水道。";
	if (method == "boatmen" && me->query("plot/arc/hometown_letters_01/relations/boatmen") < 2)
		return "船户尚不能公开替你开水门，可以改走排水道。";
	if (method == "joint" && ! joint_entry_available(me))
		return "三方中还没有两方愿意互相监督查仓，可以改走排水道。";
	if (method != "invitation" && method != "boatmen" && method != "joint" && method != "drain")
		return "没有这种入场方式。";
	PLOT_D->set_flag(me, PLOT_ID, "entry_method", method);
	if (method == "invitation") PLOT_D->set_flag(me, PLOT_ID, "assembly_relief_heard", 1);
	if (method == "invitation")
	{
		PLOT_D->set_flag(me, PLOT_ID, "testimony_relief", 1);
		PLOT_D->set_flag(me, PLOT_ID, "testimonies_heard", 1);
	}
	if (method == "boatmen") PLOT_D->set_flag(me, PLOT_ID, "worker_freed_early", 1);
	if (method == "joint") PLOT_D->set_flag(me, PLOT_ID, "outer_guard_distracted", 1);
	if (method == "drain") PLOT_D->set_flag(me, PLOT_ID, "drain_gate_needed", 1);
	return enter_room(me, entry_slot(method), entry_text(method)) + "\n\n" + render_log(me);
}

private string reach_assembly(object me)
{
	string method;

	if (stage(me) != "enter_salt_storehouse" || ! in_instance(me)) return render_log(me);
	method = PLOT_D->query_flag(me, PLOT_ID, "entry_method");
	if (! stringp(method)) return "先确定进入旧盐仓的方式。";
	if (method == "drain" && ! PLOT_D->query_flag(me, PLOT_ID, "drain_gate_open"))
		return "排水道潮闸已经锈死。先按二潮水痕的高低顺序扳开上下两道闸片。";
	PLOT_D->set_flag(me, PLOT_ID, "routes_converged", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "enter_salt_storehouse", "hear_the_assembly");
	return enter_room(me, "assembly_floor", "四条入场路线在主仓木屏后合流。你压低身形，听见集会四方同时争执。") +
		"\n\n" + render_log(me);
}

private string hear_testimony(object me, string kind)
{
	int count;
	string text;

	if (stage(me) != "hear_the_assembly" || ! in_instance(me) ||
		base_name(environment(me)) != ASSEMBLY) return render_log(me);
	if (kind == "relief")
	{
		PLOT_D->set_flag(me, PLOT_ID, "testimony_relief", 1);
		text = "乡民代表承认暗粮路救过整村人，也说每袋粮都被迫写下家属姓名。";
	}
	else if (kind == "debt")
	{
		PLOT_D->set_flag(me, PLOT_ID, "testimony_debt", 1);
		text = "船户和脚夫拿出欠据：已还清的粮债仍被改成可转卖的“欠名契”。";
	}
	else if (kind == "profiteering")
	{
		PLOT_D->set_flag(me, PLOT_ID, "testimony_profiteering", 1);
		text = "商家管事的旧票显示，同一批粮先按救济价入仓，又按市价记成百川会私产。";
	}
	else return "没有这类集会证词。";
	count = (PLOT_D->query_flag(me, PLOT_ID, "testimony_relief") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "testimony_debt") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "testimony_profiteering") ? 1 : 0);
	PLOT_D->set_flag(me, PLOT_ID, "testimonies_heard", count);
	if (count >= 3)
	{
		PLOT_D->advance_stage(me, PLOT_ID, "hear_the_assembly", "rescue_real_lu");
		text += "\n\n三类事实已经齐全。木屏后传来敲击声，真正陆小栓被锁在暗牢。";
		text += "\n\n" + enter_room(me, "dark_cell", "你沿仓柱后的窄梯下到暗牢。潮水正从石缝逼近木栅。 ");
	}
	return text + "\n\n" + render_log(me);
}

private object ensure_real_lu(object me)
{
	object room;
	object lu;
	string condition;

	room = ensure_room(me, "dark_cell");
	if (! objectp(room)) return 0;
	lu = present("lu xiaoshuan", room);
	condition = PLOT_D->query_flag(me, PLOT_ID, "real_lu_condition");
	if (objectp(lu))
	{
		room->set("plot_spawned/real_lu", 1);
		if (stringp(condition)) lu->restore_plot_condition(condition);
		return lu;
	}
	if (room->query("plot_spawned/real_lu")) return 0;
	lu = new(REAL_LU);
	if (! objectp(lu)) return 0;
	lu->set("plot_owner", me->query("id"));
	lu->move(room);
	room->set("plot_spawned/real_lu", 1);
	if (stringp(condition)) lu->restore_plot_condition(condition);
	return lu;
}

private string rescue_lu(object me, string method)
{
	object lu;

	if (stage(me) != "rescue_real_lu" || ! in_instance(me) ||
		base_name(environment(me)) != CELL) return render_log(me);
	lu = ensure_real_lu(me);
	if (! objectp(lu)) return "暗牢中的信使暂时无法恢复，请退出实例后重试。";
	if (method == "inspect")
	{
		PLOT_D->set_flag(me, PLOT_ID, "cell_lock_seen", 1);
		return "锁盘上不是数字，而是仓号与潮次：七号仓的缺口朝下，二潮水痕在第二格。";
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "cell_lock_seen"))
		return "木栅锁盘有两圈刻度。先查看锁盘，避免涨潮时反锁。";
	if (method == "wrong") return "四号仓与早潮会把内栓推向死位；这与请帖的七号仓、二潮相冲突。";
	if (method != "tide") return "没有这种开锁方法。";
	PLOT_D->set_flag(me, PLOT_ID, "real_lu_rescued", 1);
	PLOT_D->set_flag(me, PLOT_ID, "real_lu_condition", "rescued");
	PLOT_D->set_flag(me, PLOT_ID, "real_lu_testimony", 1);
	lu->restore_plot_condition("rescued");
	PLOT_D->advance_stage(me, PLOT_ID, "rescue_real_lu", "break_single_control");
	return "锁盘按七号仓、二潮依次弹开。陆小栓证实石七替换了他，灭口命令虽盖闻印，用的却是杭州纸行词法。\n\n" +
		enter_room(me, "guard_walk", "你护送陆小栓回到上层，罗七娘与护簿人已经封住通往账房的吊桥。") +
		"\n\n" + render_log(me);
}

void record_real_lu_injury(object lu)
{
	object me;
	string owner;

	if (! objectp(lu) || base_name(lu) != REAL_LU) return;
	owner = lu->query("plot_owner");
	me = stringp(owner) ? find_player(owner) : 0;
	if (! objectp(me) || environment(me) != environment(lu)) return;
	PLOT_D->set_flag(me, PLOT_ID, "real_lu_condition", "injured");
}

private object ensure_luo(object me)
{
	object room;
	object luo;

	if (PLOT_D->query_flag(me, PLOT_ID, "control_broken")) return 0;
	room = ensure_room(me, "guard_walk");
	if (! objectp(room)) return 0;
	luo = present("luo qiniang", room);
	if (objectp(luo) && living(luo))
	{
		room->set("plot_spawned/luo_qiniang", 1);
		PLOT_COMBAT_D->restore_enemy(luo);
		return luo;
	}
	if (room->query("plot_spawned/luo_qiniang")) return 0;
	luo = new(LUO);
	if (! objectp(luo)) return 0;
	luo->set("plot_owner", me->query("id"));
	if (! mapp(PLOT_COMBAT_D->configure_enemy(luo, me, "ledger_guard", 0)))
	{
		destruct(luo);
		return 0;
	}
	luo->move(room);
	room->set("plot_spawned/luo_qiniang", 1);
	return luo;
}

private string settle_control(object me, string method, string fate)
{
	PLOT_D->set_flag(me, PLOT_ID, "control_method", method);
	PLOT_D->set_flag(me, PLOT_ID, "luo_fate", fate);
	PLOT_D->set_flag(me, PLOT_ID, "control_broken", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "break_single_control", "save_people_and_records");
	return enter_room(me, "burning_archive", "吊桥刚落下，总簿室侧门便冒出火光。强硬护簿人反锁人门，转身点燃卷架。 ");
}

void record_luo_defeat(object luo)
{
	object me;
	string owner;

	if (! objectp(luo) || base_name(luo) != LUO) return;
	owner = luo->query("plot_owner");
	me = stringp(owner) ? find_player(owner) : 0;
	if (! objectp(me) || environment(me) != environment(luo) ||
		stage(me) != "break_single_control") return;
	settle_control(me, "fight", "defeated_again");
}

private string break_control(object me, string method)
{
	object luo;

	if (stage(me) != "break_single_control" || ! in_instance(me) ||
		base_name(environment(me)) != GUARD_WALK) return render_log(me);
	if (method == "fight")
	{
		luo = ensure_luo(me);
		if (! objectp(luo)) return render_log(me);
		me->kill_ob(luo);
		luo->kill_ob(me);
		return "罗七娘用短剑守住吊桥。击败她只会解除仓门控制，不能跳过纵火、双笔迹谜题或总簿选择。";
	}
	if (method == "boatmen")
	{
		if (me->query("plot/arc/hometown_letters_01/relations/boatmen") < 2)
			return "船户仍受欠据约束，不敢仅凭你的号召倒戈。可出示公共证据或正面交手。";
		luo = present("luo qiniang", environment(me));
		settle_control(me, "boatmen", "stood_aside");
		if (objectp(luo)) destruct(luo);
		return "柳大艄带船户放下吊桥，罗七娘看见众人已不再服从欠据，只得让开。\n\n" + render_log(me);
	}
	if (method == "evidence")
	{
		if (! me->query("plot/silent_crossing_03/flags/records_saved") ||
			! PLOT_D->query_flag(me, PLOT_ID, "real_lu_testimony"))
			return "还缺少能让护簿人当场核验的改签存根或真信使口供。";
		luo = present("luo qiniang", environment(me));
		settle_control(me, "evidence", "withdrew");
		if (objectp(luo)) destruct(luo);
		return "你把改签存根与陆小栓的口供并排展示，护簿人认出欠据已被第二只手改写，主动放下吊桥。\n\n" + render_log(me);
	}
	return "没有这种解除单方控制的方法。";
}

private string handle_fire(object me, string action)
{
	int saved;
	string text;

	if (stage(me) != "save_people_and_records" || ! in_instance(me) ||
		base_name(environment(me)) != BURNING) return render_log(me);
	if (action == "open_people_door")
	{
		if (PLOT_D->query_flag(me, PLOT_ID, "people_door_open")) return "人员出口已经打开。";
		PLOT_D->set_flag(me, PLOT_ID, "people_door_open", 1);
		return "你先砸断人门横木，让乡民、脚夫和船户沿外廊撤出。火势仍未越过核心索引架。";
	}
	if (action == "save_loose_pages")
	{
		if (! PLOT_D->query_flag(me, PLOT_ID, "people_door_open"))
		{
			PLOT_D->set_flag(me, PLOT_ID, "people_door_open", 1);
			PLOT_D->set_flag(me, PLOT_ID, "extra_pages_lost", 1);
			text = "你先扑向散页，许三娘替你撞开人门。所有人都活着撤出，但一架附加卷页被火吞没。";
		}
		else
		{
			PLOT_D->set_flag(me, PLOT_ID, "loose_pages_saved", 1);
			text = "人员撤出后，你用湿盐袋压住飞散卷页，保住一部分附加责任链。";
		}
		return text;
	}
	if (action != "save_core_index") return "没有这种应对纵火的行动。";
	if (! PLOT_D->query_flag(me, PLOT_ID, "people_door_open"))
		return "人门仍被反锁。必须先确保普通人能够离仓；现场不会接受以人命换全簿。";
	saved = PLOT_D->query_flag(me, PLOT_ID, "extra_pages_lost") ? 1 : 2;
	PLOT_D->set_flag(me, PLOT_ID, "records_saved", saved);
	PLOT_D->set_flag(me, PLOT_ID, "core_index_saved", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "save_people_and_records", "separate_two_hands");
	return "你抽出百川总簿的核心索引，以湿盐袋隔开火线。人命与可验证罪证都已保住。\n\n" +
		enter_room(me, "index_room", "烧焦的索引室里只剩五份盖着同一枚掌簿印的命令。墨迹却明显来自两只手。") +
		"\n\n" + render_log(me);
}

private string inspect_order(object me, string order)
{
	int count;
	string key;
	string text;

	if (stage(me) != "separate_two_hands" || ! in_instance(me) ||
		base_name(environment(me)) != INDEX_ROOM) return render_log(me);
	if (order == "grain") text = "放粮令称“平码入簿”，使用旧式账房语，“潮”字笔画完整。";
	else if (order == "debt") text = "追欠令称“旧欠归栏”，称谓与放粮令一致，收笔厚重。";
	else if (order == "silence") text = "灭口令称“封声换页”，“潮”字固定缺去右下一点。";
	else if (order == "letter") text = "伪信令写“纸行平码”，把杭州纸行用语混进驿路命令。";
	else if (order == "name") text = "换名令称活人为“旧壳”，缺笔潮字与灭口令完全相同。";
	else return "没有这份命令。";
	PLOT_D->set_flag(me, PLOT_ID, "order_" + order + "_seen", 1);
	count = 0;
	foreach (key in ({ "grain", "debt", "silence", "letter", "name" }))
		if (PLOT_D->query_flag(me, PLOT_ID, "order_" + key + "_seen")) count++;
	PLOT_D->set_flag(me, PLOT_ID, "orders_seen", count);
	return text + "\n\n" + render_log(me);
}

private string solve_hands(object me, string answer)
{
	if (stage(me) != "separate_two_hands") return render_log(me);
	if (PLOT_D->query_flag(me, PLOT_ID, "orders_seen") < 5)
		return "五份命令还没有全部核对。分组必须同时解释用词和笔画。";
	if (answer == "grain_silence")
		return "放粮令使用旧式账房语且潮字完整，灭口令使用杭州词法且缺笔；两份不能归为同一只手。";
	if (answer == "debt_name")
		return "追欠令与放粮令称谓相同，换名令却把活人称为“旧壳”；这组在称谓上冲突。";
	if (answer == "grain_debt_letter")
		return "伪信令的“纸行平码”只见于杭州纸行，不能与两份旧式账房命令归为同一作者。";
	if (answer != "grain_debt") return "这组分法无法同时解释五份命令。";
	PLOT_D->set_flag(me, PLOT_ID, "two_hands_solved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "shen_identity_known", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "separate_two_hands", "confront_wen_shouzhuo");
	return "唯一一致的分组是：放粮令、追欠令出自闻守拙；灭口令、伪信令、换名令出自第二人。"
		"陆小栓认出杭州词法，闻守拙终于说出听潮楼楼主沈观澜之名。\n\n" +
		enter_room(me, "seal_chamber", "闻守拙退入封印室，手中铁笔压着总簿印钥。他承认交出副印，却仍声称只有自己能掌簿。") +
		"\n\n" + render_log(me);
}

private int confrontation_count(object me)
{
	return (PLOT_D->query_flag(me, PLOT_ID, "cite_relief") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "cite_debt") ? 1 : 0) +
		(PLOT_D->query_flag(me, PLOT_ID, "cite_second_hand") ? 1 : 0);
}

private string cite_fact(object me, string fact)
{
	string text;

	if (stage(me) != "confront_wen_shouzhuo" || ! in_instance(me) ||
		base_name(environment(me)) != SEAL_ROOM) return render_log(me);
	if (fact == "relief") text = "许三娘指出：救过人是真的，拿救命恩情永久拴住人也是真的。";
	else if (fact == "debt") text = "谭友纪把已还清却仍被转卖的欠据逐张验印，证明单方掌簿本身已失去约束。";
	else if (fact == "second_hand") text = "你摆出双笔迹分组：闻守拙连自己副印写过哪些灭口令都无法控制。";
	else return "没有这项对质事实。";
	PLOT_D->set_flag(me, PLOT_ID, "cite_" + fact, 1);
	PLOT_D->set_flag(me, PLOT_ID, "confrontation_facts", confrontation_count(me));
	return text + "\n\n" + render_log(me);
}

private int desired_boss_phase(object me)
{
	mapping profile;

	profile = PLOT_COMBAT_D->query_player_profile(me, "ledger_master", 0);
	if (! mapp(profile)) return 0;
	if (profile["combat_tier"] >= 4) return 2;
	if (profile["combat_tier"] >= 2) return 1;
	return 0;
}

private object ensure_wen(object me)
{
	object room;
	object wen;
	int phase;
	int maximum;

	if (PLOT_D->query_flag(me, PLOT_ID, "wen_resolved")) return 0;
	room = ensure_room(me, "seal_chamber");
	if (! objectp(room)) return 0;
	wen = present("wen shouzhuo", room);
	if (objectp(wen) && living(wen))
	{
		room->set("plot_spawned/wen_shouzhuo", 1);
		PLOT_COMBAT_D->restore_enemy(wen);
		return wen;
	}
	phase = PLOT_D->query_flag(me, PLOT_ID, "boss_phase");
	if (! PLOT_D->query_flag(me, PLOT_ID, "boss_scale_set"))
	{
		maximum = desired_boss_phase(me);
		PLOT_D->set_flag(me, PLOT_ID, "boss_max_phase", maximum);
		PLOT_D->set_flag(me, PLOT_ID, "boss_scale_set", 1);
	}
	else maximum = PLOT_D->query_flag(me, PLOT_ID, "boss_max_phase");
	wen = new(WEN);
	if (! objectp(wen)) return 0;
	wen->set("plot_owner", me->query("id"));
	wen->set("plot_phase", phase);
	if (! mapp(PLOT_COMBAT_D->configure_enemy(wen, me, "ledger_master", phase)))
	{
		destruct(wen);
		return 0;
	}
	wen->move(room);
	room->set("plot_spawned/wen_shouzhuo", 1);
	return wen;
}

private string settle_wen(object me, string method)
{
	PLOT_D->set_flag(me, PLOT_ID, "wen_resolution", method);
	PLOT_D->set_flag(me, PLOT_ID, "wen_resolved", 1);
	PLOT_D->set_flag(me, PLOT_ID, "wen_fate", "awaiting_joint_hearing");
	PLOT_D->advance_stage(me, PLOT_ID, "confront_wen_shouzhuo", "choose_archive_disposition");
	return "闻守拙交出总簿印钥。他会为勒索、隐瞒和交出副印受审；沈观澜才是使用第二种笔迹制造灭口与换名令的人。";
}

int record_wen_defeat(object wen)
{
	object me;
	string owner;
	int phase;
	int maximum;

	if (! objectp(wen) || base_name(wen) != WEN) return 0;
	owner = wen->query("plot_owner");
	me = stringp(owner) ? find_player(owner) : 0;
	if (! objectp(me) || environment(me) != environment(wen) ||
		stage(me) != "confront_wen_shouzhuo") return 0;
	phase = wen->query("plot_phase");
	maximum = PLOT_D->query_flag(me, PLOT_ID, "boss_max_phase");
	if (phase < maximum)
	{
		PLOT_D->set_flag(me, PLOT_ID, "boss_phase", phase + 1);
		PLOT_D->set_flag(me, PLOT_ID, "boss_phase_defeated", phase);
		return phase + 1;
	}
	settle_wen(me, "battle");
	return 0;
}

private string confront_wen(object me, string method)
{
	object wen;

	if (stage(me) != "confront_wen_shouzhuo" || ! in_instance(me) ||
		base_name(environment(me)) != SEAL_ROOM) return render_log(me);
	if (confrontation_count(me) < 2)
		return "必须先用至少两类事实反驳“只有闻守拙能掌簿”。战斗不能代替证据链。";
	if (method == "evidence")
	{
		if (confrontation_count(me) < 3)
			return "要让护簿人不战而退，需要救济、欠据和第二只手三类事实全部到场。";
		return settle_wen(me, "evidence") + "\n\n" + render_log(me);
	}
	if (method != "fight") return "没有这种对质方式。";
	wen = ensure_wen(me);
	if (! objectp(wen)) return render_log(me);
	me->kill_ob(wen);
	wen->kill_ob(me);
	return sprintf("闻守拙以铁笔护住印钥。当前战斗阶段 %d/%d；每一阶段落败都会先保存，再进入下一层。",
		PLOT_D->query_flag(me, PLOT_ID, "boss_phase") + 1,
		PLOT_D->query_flag(me, PLOT_ID, "boss_max_phase") + 1);
}

private string choose_archive(object me, string choice, int confirmed)
{
	string prompt;

	if (stage(me) != "choose_archive_disposition") return render_log(me);
	if (choice == "full_archive")
		prompt = "封存完整总簿，由三方共同持钥。责任链完整，但受助者姓名仍需长期防止泄露。";
	else if (choice == "protected_archive")
		prompt = "当众隐去受助者姓名和无罪船户住址，只保留验明的改签、侵吞和勒索证据。普通人更安全，但部分追责线会断开。";
	else return "没有这种总簿处置。";
	if (! confirmed)
		return ZJOBLONG "总簿处置\n\n" + prompt + "\n\n确认后不可改写，两种选择奖励等价并回到同一开放世界。\n\n" +
			ZJOBACTS2 + ZJMENUF(1, 1, 8, 32) +
			"确认选择:plot act where_rivers_end_05 confirm_" + choice + ZJSEP +
			"返回日志:plot where_rivers_end_05\n";
	if (! PLOT_D->record_choice(me, PLOT_ID, "archive_disposition", choice))
		return "总簿已经有了不同处置，不能再次改写。";
	if (! PLOT_D->set_arc_choice(me, "archive_disposition", choice))
		return "总簿选择已保存，但篇章索引暂时无法同步；请再次确认同一选择。";
	PLOT_D->set_flag(me, PLOT_ID, "choice_confirmed", 1);
	PLOT_D->advance_stage(me, PLOT_ID, "choose_archive_disposition", "arc_aftermath");
	return prompt + "\n\n选择已经持久化。尾声中断后只会补播和补领奖励。\n\n" + render_log(me);
}

private string finish_aftermath(object me)
{
	object memorial;
	string choice;
	string safe;
	string ending;

	choice = me->query(ROOT + "/choices/archive_disposition");
	if (choice != "full_archive" && choice != "protected_archive") return render_log(me);
	if (in_instance(me))
	{
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = SAFE_ROOM;
		me->move(safe);
	}
	if (status(me) == "active" && ! PLOT_D->complete_chapter(me, PLOT_ID, "arc_aftermath"))
		return "篇章尾声暂时无法保存，请再次查看。";
	PLOT_D->set_arc_choice(me, "archive_disposition", choice);
	PLOT_D->set_arc_flag(me, "shen_guanlan_known", 1);
	PLOT_D->set_arc_flag(me, "real_lu_rescued", 1);
	PLOT_D->set_arc_flag(me, "three_mountains_token_known", 1);
	PLOT_D->set_arc_value(me, "current_chapter", 5);
	PLOT_D->set_arc_value(me, "status", "completed");
	if (! me->query("plot/arc/hometown_letters_01/completed_at"))
		PLOT_D->set_arc_value(me, "completed_at", time());
	if (! PLOT_D->query_flag(me, PLOT_ID, "reward_claimed"))
	{
		if (present("hometown memorial", me))
			PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
		else
		{
			memorial = new(MEMORIAL);
			if (objectp(memorial) && memorial->move(me, 1))
				PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
			else if (objectp(memorial)) destruct(memorial);
		}
	}
	if (! PLOT_D->query_flag(me, PLOT_ID, "score_claimed"))
	{
		me->add("score", 25);
		PLOT_D->set_flag(me, PLOT_ID, "score_claimed", 1);
	}
	PLOT_D->set_flag(me, PLOT_ID, "aftermath_seen", 1);
	instance_left(me);
	ending = choice == "full_archive" ?
		"完整总簿由三方共同持钥。完整责任链得以保留，受助者却会更谨慎地看待每一次查阅。" :
		"受助者姓名与无罪船户住址被当众隐去，验明罪证另存抄件。民间更愿再开口，部分旧责任链却已断开。";
	return ZJOBLONG "百川归处\n\n" + ending + "\n\n"
		"闻守拙失去百川总簿、回浪印和秘密粮路的单方控制；真正陆小栓获救。沈观澜留下的听潮铜签上刻着三处山名，"
		"但第六章内容尚未开放。\n\n第五章「百川归处」与乡书篇完成。你获得等价的固定阅历和“乡书篇”纪念页。\n";
}

private string claim_memorial(object me)
{
	object memorial;

	if (status(me) != "completed") return "第五章尚未完成。";
	if (present("hometown memorial", me)) return "乡书篇纪念页还在你身上。";
	memorial = new(MEMORIAL);
	if (! objectp(memorial) || ! memorial->move(me, 1))
	{
		if (objectp(memorial)) destruct(memorial);
		return "你身上没有空位，清出一格后再补领纪念页。";
	}
	PLOT_D->set_flag(me, PLOT_ID, "reward_claimed", 1);
	return "杜宽依据五章日志补出一页“乡书篇”纪念页。";
}

void instance_left(object me)
{
	object *rooms;
	object room;
	object ob;
	string slot;

	if (! objectp(me) || in_instance(me)) return;
	rooms = ({});
	room = query_room(me, "salt_gate");
	if (objectp(room)) rooms += ({ room });
	foreach (slot in ({ "drain_tunnel", "water_gate", "inspection_yard", "outer_gallery",
		"assembly_floor", "dark_cell", "guard_walk", "burning_archive", "index_room", "seal_chamber" }))
	{
		room = query_room(me, slot);
		if (objectp(room)) rooms += ({ room });
	}
	foreach (room in rooms)
		foreach (ob in all_inventory(room))
			if (objectp(ob) && ob != me) destruct(ob);
	PLOT_D->close_instance_rooms(me, PLOT_ID);
	PLOT_D->close_instance(me, PLOT_ID);
}

string render_log(object me)
{
	string current;
	string entry;

	PLOT_D->ensure_player_state(me);
	current = stage(me);
	if (status(me) == "available")
		return "七号旧盐仓将在二潮后开门。开始：plot act where_rivers_end_05 begin\n";
	if (status(me) == "completed")
		return "乡书篇已经完成。总簿处置：" +
			(me->query(ROOT + "/choices/archive_disposition") == "full_archive" ? "全簿见证" : "隐名见证") +
			"。补领：plot act where_rivers_end_05 claim_memorial\n";
	if (current == "enter_salt_storehouse")
	{
		entry = PLOT_D->query_flag(me, PLOT_ID, "entry_method");
		if (stringp(entry)) return "入场方式已保存。重新进入现场后前往主仓，四路将在木屏后合流。\n";
		return "入场可选：请帖正门、船户水门、三方查仓；排水道是无关系、无师门和异常历史状态的保底路线。\n";
	}
	if (current == "hear_the_assembly")
		return sprintf("集会证词：%d/3。需要听取救济、欠据和侵吞三类事实。\n",
			PLOT_D->query_flag(me, PLOT_ID, "testimonies_heard"));
	if (current == "rescue_real_lu") return "真正陆小栓被关在暗牢。查看锁盘，并用七号仓、二潮线索开锁。\n";
	if (current == "break_single_control") return "吊桥被罗七娘与护簿人控制。可战斗、发动船户，或展示改签存根与真信使口供。\n";
	if (current == "save_people_and_records") return "纵火阶段：先开人员出口，再保住核心索引。若先抢散页，NPC 会救人但附加卷页会损失。\n";
	if (current == "separate_two_hands")
		return sprintf("双笔迹索引：已查看 %d/5 份命令。按旧式账房语、杭州词法和缺笔“潮”字分组。\n",
			PLOT_D->query_flag(me, PLOT_ID, "orders_seen"));
	if (current == "confront_wen_shouzhuo")
		return sprintf("已用于对质的事实：%d/3。至少两类事实后才能交战，三类事实可迫其交印。\n",
			confrontation_count(me));
	if (current == "choose_archive_disposition") return "总簿必须二次确认：封存全簿追责，或隐去无辜者姓名后存证。奖励等价。\n";
	if (current == "arc_aftermath") return "总簿选择已保存。查看尾声只会补播、补领并完成篇章结算。\n";
	return "目标：终止对百川总簿的单方控制，并分离罪证与无辜者姓名。\n";
}

string ask_real_lu(object me)
{
	if (! objectp(me)) return "陆小栓没有回应。";
	return "陆小栓说：关我的人用闻守拙的印，却把活人叫作“旧壳”，那是杭州听潮楼的词。\n";
}

mixed handle_action(object me, string action)
{
	string value;

	if (! objectp(me) || ! userp(me)) return "无效的剧情参与者。";
	PLOT_D->ensure_player_state(me);
	if (action == "begin" || action == "enter_salt_storehouse") return begin_chapter(me);
	if (action == "entry_invitation") return choose_entry(me, "invitation");
	if (action == "entry_boatmen") return choose_entry(me, "boatmen");
	if (action == "entry_joint") return choose_entry(me, "joint");
	if (action == "entry_drain") return choose_entry(me, "drain");
	if (action == "open_drain_gate")
	{
		if (stage(me) != "enter_salt_storehouse" || ! in_instance(me) || base_name(environment(me)) != DRAIN)
			return render_log(me);
		PLOT_D->set_flag(me, PLOT_ID, "drain_gate_open", 1);
		return "你按水痕先扳下片、再推上片，潮闸没有反锁，排水道通向主仓外廊。";
	}
	if (action == "reach_assembly") return reach_assembly(me);
	if (action == "hear_relief") return hear_testimony(me, "relief");
	if (action == "hear_debt") return hear_testimony(me, "debt");
	if (action == "hear_profiteering") return hear_testimony(me, "profiteering");
	if (action == "inspect_cell_lock") return rescue_lu(me, "inspect");
	if (action == "open_cell_tide") return rescue_lu(me, "tide");
	if (action == "open_cell_warehouse") return rescue_lu(me, "wrong");
	if (action == "fight_luo") return break_control(me, "fight");
	if (action == "turn_boatmen") return break_control(me, "boatmen");
	if (action == "show_public_evidence") return break_control(me, "evidence");
	if (action == "open_people_door" || action == "save_loose_pages" || action == "save_core_index")
		return handle_fire(me, action);
	if (sscanf(action, "inspect_order_%s", value) == 1) return inspect_order(me, value);
	if (action == "hands_hint_one")
	{
		PLOT_D->set_flag(me, PLOT_ID, "hint_level", 1);
		return "一级提示：“纸行平码”和“旧壳”都是杭州词法，先把包含它们的命令放在一起。";
	}
	if (action == "hands_hint_two")
	{
		PLOT_D->set_flag(me, PLOT_ID, "hint_level", 2);
		return "二级提示：放粮令是闻守拙承认的亲笔样本；追欠令的称谓与收笔同它一致。";
	}
	if (sscanf(action, "solve_hands_%s", value) == 1) return solve_hands(me, value);
	if (action == "cite_relief") return cite_fact(me, "relief");
	if (action == "cite_debt") return cite_fact(me, "debt");
	if (action == "cite_second_hand") return cite_fact(me, "second_hand");
	if (action == "fight_wen") return confront_wen(me, "fight");
	if (action == "force_wen_yield") return confront_wen(me, "evidence");
	if (action == "choose_full_archive") return choose_archive(me, "full_archive", 0);
	if (action == "choose_protected_archive") return choose_archive(me, "protected_archive", 0);
	if (action == "confirm_full_archive") return choose_archive(me, "full_archive", 1);
	if (action == "confirm_protected_archive") return choose_archive(me, "protected_archive", 1);
	if (action == "aftermath" || action == "chapter_close") return finish_aftermath(me);
	if (action == "claim_memorial") return claim_memorial(me);
	if (action == "enter")
	{
		if (stage(me) == "enter_salt_storehouse")
		{
			value = PLOT_D->query_flag(me, PLOT_ID, "entry_method");
			if (! stringp(value)) return render_log(me);
			return enter_room(me, entry_slot(value), entry_text(value));
		}
		if (stage(me) == "hear_the_assembly") return enter_room(me, "assembly_floor", "你回到旧盐仓主仓木屏后。 ");
		if (stage(me) == "rescue_real_lu")
		{
			string text;
			text = enter_room(me, "dark_cell", "你回到涨潮中的暗牢。 ");
			ensure_real_lu(me);
			return text;
		}
		if (stage(me) == "break_single_control")
		{
			string text;
			text = enter_room(me, "guard_walk", "你回到护簿吊桥前。 ");
			ensure_luo(me);
			return text;
		}
		if (stage(me) == "save_people_and_records") return enter_room(me, "burning_archive", "你回到起火的卷架间。人员默认仍然存活。 ");
		if (stage(me) == "separate_two_hands") return enter_room(me, "index_room", "你回到五份命令前。已保存的查看进度仍在。 ");
		if (stage(me) == "confront_wen_shouzhuo")
		{
			string text;
			text = enter_room(me, "seal_chamber", "你回到封印室，闻守拙仍守着印钥。 ");
			ensure_wen(me);
			return text;
		}
		return render_log(me);
	}
	if (action == "leave")
	{
		string safe;
		safe = PLOT_D->query_flag(me, PLOT_ID, "safe_return");
		if (! stringp(safe) || safe == "") safe = SAFE_ROOM;
		if (in_instance(me)) me->move(safe);
		instance_left(me);
		return "你离开旧盐仓，回到安全处。持久阶段不会回退。";
	}
	return "这项剧情操作不存在。";
}
