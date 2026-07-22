// Per-player warehouse instance for origin_letter_01.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "城外废仓");
	set("long", "废仓的门轴发出沉闷的声响，旧麻袋和断裂的木箱堆在墙边。\n"
		"一册义仓簿的去向，就藏在这间不属于任何常驻地图的仓房里。\n");
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/beimen" ]));
	setup();
}

void cleanup_after_exit(object me)
{
	if (objectp(me) && environment(me) != this_object())
		"/adm/daemons/plot/origin_letter"->instance_left(me);
}

int valid_leave(object me, string dir)
{
	if (dir == "out" && me->query("id") != query("plot_owner"))
		return notify_fail("这间废仓不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
