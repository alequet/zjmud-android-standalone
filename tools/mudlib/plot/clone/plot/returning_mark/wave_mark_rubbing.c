// Chapter-two story keepsake.

#include <ansi.h>

inherit ITEM;

void create()
{
	set_name("回浪印拓样", ({ "wave mark rubbing", "wave rubbing", "returning mark" }));
	set_weight(1);
	set("unit", "张");
	set("long", "一张薄纸上的回浪印拓样。首尾相接的水纹被刻意拆开，旁边记着青芦渡的旧仓号。\n");
	set("value", 0);
	set("material", "paper");
	set("no_sell", 1);
	setup();
}

void init()
{
	add_action("do_read", "read");
}

int do_read(string arg)
{
	if (! arg || present(arg, this_player()) != this_object()) return 0;
	write("拓样旁注：两半合印，才知道货物真正送到了谁手里。\n");
	return 1;
}
