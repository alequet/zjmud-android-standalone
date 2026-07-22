// Chapter-four personal instance: road outside the north gate.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "北门外旧驿路");
	set("long", "旧驿路在废亭前分成两股，一边通往运河埠头，一边绕向杭州官道。被撕碎的信封黏在界碑草根，\n"
		"路旁浅沟足以截住快马，废亭横梁也能封住翻墙退路。这不是一处可以随意替换的竞技场。\n");
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/beimen" ]));
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
		return notify_fail("这段旧驿路不是你的追踪现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
