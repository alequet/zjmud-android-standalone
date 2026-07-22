// Fixed investigation room: grain house office.

inherit ROOM;

void create()
{
	set("short", "粮行账房");
	set("long", "账房里堆着尚未归档的货签，柜台后的周守义把手按在一册出库簿上。\n"
		"他愿意补粮，却坚持说公开账目会牵连更多无辜的人。\n");
	set("exits", ([ "out" : "/d/city/postofficer" ]));
	set("objects", ([ __DIR__ + "/npc/zhou_shouyi" : 1 ]));
	setup();
}

