// Zhou Shouyi, a chapter-only investigation contact.

#include <ansi.h>

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/origin_letter"->handle_action(
		this_player(), "hear_grain");
}

string ask_mark()
{
	return "/adm/daemons/plot/returning_mark"->npc_notice(this_player());
}

string ask_visitors()
{
	return "/adm/daemons/plot/visitors_from_home"->npc_notice(this_player());
}

void create()
{
	set_name("周守义", ({ "zhou shouyi", "zhou", "grain owner" }));
	set("gender", "男性");
	set("age", 49);
	set("long", "粮行东家，袖口沾着常年搬运粮袋留下的细尘。\n");
	set("combat_exp", 1500);
	set("attitude", "friendly");
	set("plot_lifecycle", "world_refresh");
	set("plot_chapter", "origin_letter_01");
	set("inquiry", ([ "义仓簿" : (: ask_plot :), "出库簿" : (: ask_plot :),
		"旧印" : (: ask_mark :), "回浪印" : (: ask_mark :),
		"伪信" : (: ask_visitors :), "故园来客" : (: ask_visitors :) ]));
	set_skill("literate", 70);
	setup();
}
