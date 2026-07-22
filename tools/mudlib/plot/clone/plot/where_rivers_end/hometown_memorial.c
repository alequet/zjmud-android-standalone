// Five-chapter arc keepsake with no combat stats.

#include <ansi.h>

inherit ITEM;

void create()
{
	set_name(HIW "乡书篇纪念页" NOR, ({ "hometown memorial", "memorial page", "memorial" }));
	set_weight(20);
	set("unit", "页");
	set("long", "旧驿木牌上添成的一页篇章纪念：乡书、回浪印、夜船灯、盐仓请帖与百川总簿依次刻在纸边。它不提供属性优势。\n");
	set("value", 0);
	set("material", "paper");
	set("no_sell", 1);
	set("plot_item", "where_rivers_end_05");
	setup();
}

void init()
{
	add_action("do_read", "read");
}

int do_read(string arg)
{
	object me;
	string choice;

	me = this_player();
	if (! arg || present(arg, me) != this_object()) return 0;
	choice = me->query("plot/arc/hometown_letters_01/choices/archive_disposition");
	write("五章回顾：缺失人名、双半印、无声夜船、真假乡书，最终都汇入百川总簿。你的见证是“" +
		(choice == "full_archive" ? "全簿追责" : "隐名存证") + "”。\n");
	return 1;
}
