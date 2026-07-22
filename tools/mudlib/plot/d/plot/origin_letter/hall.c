// Fixed investigation room: hometown hall.

inherit ROOM;

void create()
{
	set("short", "同乡会馆");
	set("long", "会馆里挂着几盏旧灯，墙上贴满了各地灾村的名册。许三娘在案前整理登记簿，\n"
		"她愿意救急，却不愿再让无辜的作保人被账簿牵连。\n");
	set("exits", ([ "out" : "/d/city/postofficer" ]));
	set("objects", ([ __DIR__ + "/npc/xu_sanniang" : 1 ]));
	setup();
}

