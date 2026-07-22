// Non-combat chapter keepsake.

#include <ansi.h>

inherit ITEM;

void create()
{
	set_name("旧驿木牌", ({ "old post token", "post token", "wood token" }));
	set_weight(10);
	set("unit", "块");
	set("long", "一块边角磨亮的旧驿木牌。翻到背面，可以看见故乡方向的刻痕和一行未完成的编号。\n");
	set("value", 0);
	set("material", "wood");
	set("no_sell", 1);
	setup();
}

void init()
{
	add_action("do_read", "read");
}

int do_read(string arg)
{
	if (! arg || present(arg, this_player()) != this_object())
		return 0;
	write("木牌背面刻着：来处可查，姓名不可轻弃。\n");
	return 1;
}

