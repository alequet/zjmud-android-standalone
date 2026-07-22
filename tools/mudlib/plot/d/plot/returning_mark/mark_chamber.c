// Chapter-two personal instance: the tide-mark workroom.

#include <room.h>

inherit ROOM;

void create()
{
	set("short", "潮印案台");
	set("long", "后室比前仓更窄，地面铺着退色的船板。三块木牌钉在案台上，分别记着潮次、船号和仓号，\n"
		"孟四留下的半张凭据正好可以覆在中间，水纹印的断口清晰可见。\n");
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
		return notify_fail("这间潮印案台不是你的调查现场。\n");
	if (dir == "out") call_out("cleanup_after_exit", 1, me);
	return ::valid_leave(me, dir);
}
