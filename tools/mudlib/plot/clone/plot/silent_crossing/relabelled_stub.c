// Chapter-three story evidence keepsake.

#include <ansi.h>

inherit ITEM;

void create()
{
	set_name("改签存根", ({ "relabelled stub", "cargo stub", "stub" }));
	set_weight(1);
	set("unit", "份");
	set("long", "一份无属性优势的改签存根纪念副本。针脚下保留着义仓旧号，旁边另有被改写的姓名。\n");
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
	write("存根旁注：救命的暗路若只剩一本总簿说了算，也会成为改写姓名的权力。\n");
	return 1;
}
