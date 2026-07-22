// Chapter-three personal instance: night boat foredeck.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "夜船船首");
	set("long", "船首吃水很深，湿缆从系柱交错拉向乌篷。正面截船惊动的押运人正借缆绳封住狭舱入口。\n");
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
		return notify_fail("这处船首不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
