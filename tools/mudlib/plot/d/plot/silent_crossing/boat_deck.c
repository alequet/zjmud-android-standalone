// Chapter-three personal instance: night boat deck.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "乌篷夜船甲板");
	set("long", "低矮乌篷压住大半甲板，三盏船灯被黑布遮去一半。湿缆绕在系柱上，船舷外就是青芦渡暗水。\n"
		"柳大艄守着通往货舱的窄梯，船上没有固定船名，只有可以随时换下的旧船签。\n");
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
		return notify_fail("这艘夜船不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
