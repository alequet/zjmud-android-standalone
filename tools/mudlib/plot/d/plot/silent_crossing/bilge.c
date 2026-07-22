// Chapter-three personal instance: night boat bilge.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "夜船舱底");
	set("long", "检修板下几乎无法直身，浑浊河水在龙骨两侧晃动。旧水桶、湿麻布和压舱石都贴着船底摆放。\n");
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/beimen" ]));
	setup();
}

void cleanup_after_exit(object me)
{
	if (objectp(me) && environment(me) != this_object())
		"/adm/daemons/plot/silent_crossing"->instance_left(me);
}

int valid_leave(object me, string dir)
{
	if (dir == "out" && me->query("id") != query("plot_owner"))
		return notify_fail("这处舱底不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
