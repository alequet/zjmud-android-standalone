// Isolated arena whose visible exit rejects movement for bounded retry tests.

inherit ROOM;

void create()
{
	set("short", "AI 阻断撤退测试室");
	set("long", "这里的出口用于验证撤退失败不会造成无限重试。\n");
	set("no_save_location", 1);
	set("no_login", 1);
	set("no_fight", 0);
	set("ai_test_room", 1);
	set("ai_test_block_exit", 1);
	set("exits", ([ "out" : "/d/standalone/ai_test_safe" ]));
	setup();
}

int valid_leave(object me, string dir)
{
	if (objectp(me) && me->query("ai_player") && dir == "out")
		return notify_fail("测试出口暂时被阻断。\n");
	return 1;
}
