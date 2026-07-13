inherit ROOM;

void create()
{
    set("short", "Transit room");
    set("long", "This compatibility room releases legacy anti-cheat saves.\n");
    set("exits", ([ "out" : "/d/city/wumiao" ]));
    setup();
}

void init()
{
    object me;

    me = this_player();
    if (! objectp(me) || wizardp(me)) return;
    me->set("startroom", "/d/city/wumiao");
    me->move("/d/city/wumiao");
}
