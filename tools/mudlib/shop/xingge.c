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
	set_name(HIM "性格丹" NOR, ({ "xingge dan" }));
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
		set("long", "使用后可直接在四种性格之间重新选择。\n");
		set("only_do_effect", 1);
	}
	setup();
}

void init()
{
	add_action("do_reset", "xingge_dan");
}

int valid_character(string character)
{
	return character == "光明磊落" || character == "心狠手辣" ||
	       character == "狡黠多变" || character == "阴险奸诈";
}

void show_character_menu(object me)
{
	string message;

	message = ZJOBLONG + HIR "注意：改变性格之后与新性格冲突的武学将被删除！" NOR ZJBR
		"光明磊落，宗师心法-九阴神功，提高攻击" ZJBR
		"心狠手辣，宗师心法-南海玄功，增加防御" ZJBR
		"狡黠多变，宗师心法-不败神功，提高命中" ZJBR
		"阴险奸诈，宗师心法-葵花魔功，增加闪避" ZJBR
		"请选择你的性格：\n";
	message += ZJOBACTS2 + ZJMENUF(2, 2, 9, 30);
	message += "光明磊落:xingge_dan 光明磊落";
	message += ZJSEP "狡黠多变:xingge_dan 狡黠多变";
	message += ZJSEP "阴险奸诈:xingge_dan 阴险奸诈";
	message += ZJSEP "心狠手辣:xingge_dan 心狠手辣";
	tell_object(me, message + "\n");
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
		if (!"/adm/daemons/character_skilld"->valid_for_character(skills[i], me->query("character")) &&
		    strsrch(skills[i], "-cognize") == -1)
		{
			tell_object(me, to_chinese(skills[i]) + "与当前性格冲突，自动删除，如有错误请联系代码管理恢复。\n");
			log_ufile(me, "viplvgift", me->query_skill(skills[i], 1) + "级" + skills[i] + "改变性格后与新性格冲突，自动删除。\n");
			me->set("valid_skill/" + skills[i], me->query_skill(skills[i], 1));
			me->delete_skill(skills[i]);
		}
	}
}

int do_reset(string arg)
{
	object me;
	string character, confirm, message, old_character;

	me = this_player();
	if (environment(this_object()) != me)
		return notify_fail("你身上没有性格丹。\n");

	if (me->is_busy())
		return notify_fail("你正忙着呢。\n");

	if (me->is_fighting())
		return notify_fail("正在战斗中，不能使用" + name() + "。\n");

	if (!arg || arg == "")
	{
		show_character_menu(me);
		return 1;
	}

	if (sscanf(arg, "%s %s", character, confirm) == 2)
	{
		if (!valid_character(character) || confirm != "yes")
		{
			tell_object(me, "没有这种性格，请认真选择。\n");
			return 1;
		}

		if (me->query("character") == character)
		{
			tell_object(me, "你本来就是这种性格。\n");
			return 1;
		}

		old_character = me->query("character");
		me->set("character", character);
		log_ufile(me, "viplvgift", "使用性格丹成功将性格由" + old_character + "改变为" + character + "！\n");
		remove_invalid_skills(me);
		tell_object(me, "你成功将性格改变为" + character + "。\n");
		add_amount(-1);
		return 1;
	}

	character = arg;
	if (!valid_character(character))
	{
		tell_object(me, "没有这种性格，请认真选择。\n");
		return 1;
	}

	if (me->query("character") == character)
	{
		tell_object(me, "你本来就是这种性格。\n");
		return 1;
	}

	message = ZJOBLONG + "你确定选择性格为" + HIG + character + NOR "吗？\n";
	message += ZJOBACTS2 + ZJMENUF(2, 2, 9, 30);
	message += "确定:xingge_dan " + character + " yes";
	message += ZJSEP "取消:look";
	tell_object(me, message + "\n");
	return 1;
}

int do_effect(object me)
{
	return do_reset(0);
}
