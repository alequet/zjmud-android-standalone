// A He, a protected young deckhand in chapter three.

#include <ansi.h>

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/silent_crossing"->ask_a_he(this_player());
}

void create()
{
	set_name("阿禾", ({ "a he", "ahe", "young deckhand" }));
	set("gender", "男性");
	set("age", 19);
	set("long", "年轻船工，手腕上缠着搬药箱留下的布条。他的家人曾靠暗粮路活过灾年。\n");
	set("plot_lifecycle", "instance_essential");
	set("plot_chapter", "silent_crossing_03");
	set("combat_exp", 1200);
	set("attitude", "friendly");
	set("max_qi", 320);
	set("qi", 320);
	set("max_jing", 200);
	set("jing", 200);
	set("inquiry", ([ "药签" : (: ask_plot :), "家人" : (: ask_plot :) ]));
	setup();
}

private void survive_injury()
{
	"/adm/daemons/plot/silent_crossing"->record_a_he_injury(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	reincarnate();
	set("eff_qi", query("max_qi"));
	set("qi", query("max_qi"));
	set("eff_jing", query("max_jing"));
	set("jing", query("max_jing"));
	set("plot_condition", "injured");
	message_vision("$N被滑落的货箱撞倒，受了伤，但仍护住了那张换名药签。\n", this_object());
}

void unconcious()
{
	survive_injury();
}

void die()
{
	survive_injury();
}

void restore_plot_condition(string condition)
{
	if (condition == "injured")
	{
		set("plot_condition", "injured");
		set("long", "年轻船工，手腕上缠着搬药箱留下的布条。阿禾受了伤，仍护着那张换名药签。\n");
	}
	else if (condition == "rescued")
		set("plot_condition", "rescued");
}
