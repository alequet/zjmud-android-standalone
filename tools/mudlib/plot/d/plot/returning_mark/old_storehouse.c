// Chapter-two personal instance: old warehouse front room.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "扬州旧仓");
	set("long", "旧仓的梁柱被潮气泡得发黑，空箱堆成一条窄窄的通道。墙上残留着船户互保时用过的水纹印，\n"
		"孟四守在一张缺角案台旁，另一半凭据压在他的手肘下。后面有一扇通往潮印案台的偏门。\n");
	set("no_save_location", 1);
	set("plot_instance", 1);
	set("no_sleep_room", 1);
	set("exits", ([ "out" : "/d/city/beimen" ]));
	setup();
}

void cleanup_after_exit(object me)
{
	if (objectp(me) && environment(me) != this_object())
		"/adm/daemons/plot/returning_mark"->instance_left(me);
}

int valid_leave(object me, string dir)
{
	if (dir == "out" && me->query("id") != query("plot_owner"))
		return notify_fail("这间旧仓不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
