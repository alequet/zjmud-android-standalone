// Xu Sanniang, a chapter-only investigation contact.

#include <ansi.h>

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/origin_letter"->handle_action(
		this_player(), "hear_hall");
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
	set_name("许三娘", ({ "xu sanniang", "xu", "sanniang" }));
	set("gender", "女性");
	set("age", 42);
	set("long", "同乡会馆的管事，手边总有一本记着灾村和作保人的簿册。\n");
	set("combat_exp", 1200);
	set("attitude", "friendly");
	set("plot_lifecycle", "world_refresh");
	set("plot_chapter", "origin_letter_01");
	set("inquiry", ([ "义仓簿" : (: ask_plot :), "账簿" : (: ask_plot :),
		"旧印" : (: ask_mark :), "回浪印" : (: ask_mark :),
		"伪信" : (: ask_visitors :), "故园来客" : (: ask_visitors :) ]));
	set_skill("literate", 80);
	setup();
}
