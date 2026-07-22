#include <ansi.h>

inherit COMBINED_ITEM;

string query_autoload()
{
	return query_amount() + "";
}

void autoload(string param)
{
	int amount;

	if (sscanf(param, "%d", amount) == 1)
		set_amount(amount);
}

void setup()
{
	set_amount(1);
	::setup();
}

void create()
{
	set_name(HIY "洗点丹" NOR, ({ "xidian dan" }));
	if (clonep())
		set_default_object(__FILE__);
	else
	{
		set("value", 100);
		set("yuanbao", 1);
		set("no_sell", 1);
		set("unit", "颗");
		set("base_unit", "颗");
		set("base_weight", 10);
		set("base_value", 100);
		set("long", "使用后可直接重新分配先天臂力、悟性、根骨和身法。\n");
		set("only_do_effect", 1);
	}
	setup();
}

void init()
{
	add_action("do_reset", "xidian_dan");
}

void show_reset_prompt(object me, int total, string lighting)
{
	tell_object(me, INPUTTXT(lighting +
			"注意：" HIR "洗点后先天属性不符合要求的武学将被删除，请认真考虑。" NOR "$br#"
			"请输入你想分配的方案：$br#按照臂力、悟性、根骨、身法次序输入4个用空格分开的数，"
			"每个数不能低于13，4个数之和必须为" + total + "。",
		"xidian_dan $txt#") + "\n");
}

void remove_invalid_skills(object me)
{
	int i;
	string *skills;
	mapping skill_map;

	skill_map = me->query_skills();
	if (!mapp(skill_map))
		return;

	skills = keys(skill_map);
	for (i = 0; i < sizeof(skills); i++)
	{
		if (!"/adm/daemons/attribute_skilld"->valid_for_attributes(skills[i], me) &&
		    strsrch(skills[i], "-cognize") == -1)
		{
			tell_object(me, to_chinese(skills[i]) + "不符合当前先天属性要求，自动删除，如有错误请联系代码管理恢复。\n");
			log_ufile(me, "viplvgift", me->query_skill(skills[i], 1) + "级" + skills[i] + "洗点后不符合先天属性要求，自动删除。\n");
			me->set("valid_skill/" + skills[i], me->query_skill(skills[i], 1));
			me->delete_skill(skills[i]);
		}
	}
}

int do_reset(string arg)
{
	int total, temp_str, temp_int, temp_con, temp_dex;
	object me;
	string lighting, message;

	me = this_player();
	if (environment(this_object()) != me)
		return notify_fail("你身上没有洗点丹。\n");

	if (me->is_busy())
		return notify_fail("你正忙着呢。\n");

	if (me->is_fighting())
		return notify_fail("正在战斗中，不能使用" + name() + "。\n");

	total = me->query("str") + me->query("int") + me->query("con") + me->query("dex");
	if (!me->query("gift/lighting"))
	{
		lighting = HIM "你尚未遭遇过雷劈天赋事件，本次洗点将赠送你雷劈天赋！" ZJBR;
		total++;
	}
	else
		lighting = "";

	if (!arg || arg == "")
	{
		show_reset_prompt(me, total, lighting);
		return 1;
	}

	if (sscanf(arg, "yes %d %d %d %d", temp_str, temp_int, temp_con, temp_dex) == 4)
	{
		if (temp_str < 13 || temp_int < 13 || temp_con < 13 || temp_dex < 13 ||
		    temp_str + temp_int + temp_con + temp_dex != total)
		{
			show_reset_prompt(me, total, lighting);
			return 1;
		}

		me->set("str", temp_str);
		me->set("int", temp_int);
		me->set("con", temp_con);
		me->set("dex", temp_dex);
		if (lighting != "")
			me->add("gift/lighting", 1);

		log_ufile(me, "viplvgift", sprintf("使用洗点丹成功洗点，目前：臂力[%d]，悟性[%d]，根骨[%d]，身法[%d]。\n",
			me->query("str"), me->query("int"), me->query("con"), me->query("dex")));
		remove_invalid_skills(me);
		tell_object(me, "洗点成功，祝您游戏愉快。\n");
		add_amount(-1);
		return 1;
	}

	if (sscanf(arg, "%d %d %d %d", temp_str, temp_int, temp_con, temp_dex) == 4)
	{
		if (temp_str < 13 || temp_int < 13 || temp_con < 13 || temp_dex < 13 ||
		    temp_str + temp_int + temp_con + temp_dex != total)
		{
			show_reset_prompt(me, total, lighting);
			return 1;
		}

		message = ZJOBLONG + "臂力[" + temp_str + "]，悟性[" + temp_int + "]，根骨[" +
			temp_con + "]，身法[" + temp_dex + "]，你确定吗？\n";
		message += ZJOBACTS2 + ZJMENUF(2, 2, 9, 30);
		message += "确定:xidian_dan yes " + arg;
		message += ZJSEP "取消:look";
		tell_object(me, message + "\n");
		return 1;
	}

	show_reset_prompt(me, total, lighting);
	return 1;
}

int do_effect(object me)
{
	return do_reset(0);
}
