// Shared access and cleanup behavior for chapter-five instance rooms.

#include <room.h>

inherit ROOM;

void setup_plot_room(string room_short, string room_long)
{
	set("short", room_short);
	set("long", room_long);
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/beimen" ]));
	setup();
}

void cleanup_after_exit(object me)
{
	if (objectp(me) && environment(me) != this_object())
		"/adm/daemons/plot/where_rivers_end"->instance_left(me);
}

int valid_leave(object me, string dir)
{
	if (dir == "out" && me->query("id") != query("plot_owner"))
		return notify_fail("这处旧盐仓不是你的剧情现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
