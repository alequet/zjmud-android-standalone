// Isolated room reserved for administrator-triggered AI diagnostics.

inherit ROOM;

void create()
{
	set("short", "AI 隔离测试室");
	set("long", "这里没有出口，只用于单机 AI 的受控诊断场景。\n");
	set("no_save_location", 1);
	set("no_login", 1);
	set("no_fight", 0);
	set("ai_test_room", 1);
	set("exits", ([]));
	setup();
}
