// Chapter-three personal instance: shared narrow passage.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "乌篷狭舱");
	set("long", "乌篷下的通道只能容一人侧身通过。甲板、船首和船尾三条登船路线都在这里汇合，\n"
		"一架沾着药渣和谷壳的木梯通向下层货舱。\n");
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
		return notify_fail("这条狭舱通道不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
