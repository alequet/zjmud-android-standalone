// Isolated arena with one policy-approved retreat exit.

inherit ROOM;

void create()
{
	set("short", "AI 撤退测试室");
	set("long", "这里仅用于验证 AI 在受压时选择安全出口。\n");
	set("no_save_location", 1);
	set("no_login", 1);
	set("no_fight", 0);
	set("ai_test_room", 1);
	set("exits", ([ "out" : "/d/standalone/ai_test_safe" ]));
	setup();
}
