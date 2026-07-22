// Chapter-four personal instance: relay-station rear court.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "扬州驿站后院");
	set("long", "后院堆着同日送达的三只信匣，封泥、纸张和路引格式彼此矛盾。自称陆小栓的来客坐在廊下，\n"
		"鞋边却沾着只在杭州塔边常见的细白沙。他的包袱始终压在能够看见信匣的位置。\n");
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/postofficer" ]));
	setup();
}

void cleanup_after_exit(object me)
{
	if (objectp(me) && environment(me) != this_object())
		"/adm/daemons/plot/visitors_from_home"->instance_left(me);
}

int valid_leave(object me, string dir)
{
	if (dir == "out" && me->query("id") != query("plot_owner"))
		return notify_fail("这处驿站后院不是你的查信现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
