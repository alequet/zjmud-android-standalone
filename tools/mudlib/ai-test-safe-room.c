// Destination used only to prove that constrained combat retreat changed room.

inherit ROOM;

void create()
{
	set("short", "AI 撤退安全室");
	set("long", "这里是隔离战斗撤退后的安全落点。\n");
	set("no_save_location", 1);
	set("no_login", 1);
	set("no_fight", 1);
	set("ai_test_room", 1);
	set("exits", ([ "in" : "/d/standalone/ai_test_retreat" ]));
	setup();
}
