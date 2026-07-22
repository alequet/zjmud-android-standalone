// Liu Dashao, owner of the nameless night boat.

#include <ansi.h>

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/silent_crossing"->ask_liu(this_player());
}

void create()
{
	set_name("柳大艄", ({ "liu dashao", "liu", "boat owner" }));
	set("gender", "男性");
	set("age", 47);
	set("long", "肩背被多年撑篙磨得微驼的船主。他不替闻守拙辩白，却坚持救命粮不能先被毁掉。\n");
	set("plot_lifecycle", "instance_social");
	set("plot_chapter", "silent_crossing_03");
	set("combat_exp", 5000);
	set("attitude", "peaceful");
	set("max_qi", 500);
	set("qi", 500);
	set("inquiry", ([ "夜船" : (: ask_plot :), "货物" : (: ask_plot :), "百川会" : (: ask_plot :) ]));
	setup();
}
