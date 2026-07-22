// Chapter-three personal instance: night boat cargo hold.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "夜船货舱");
	set("long", "货舱被三排木柱分开：旧义仓号藏在粮袋针脚下，药箱挂着没有路引的船户药签，\n"
		"最里侧的无册灾粮贴着新换的收货名。断裂压舱板、绷紧主缆和舱底水都能改变眼前冲突。\n");
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
		return notify_fail("这间货舱不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
