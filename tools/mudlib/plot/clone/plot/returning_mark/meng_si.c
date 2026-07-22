// Meng Si, the old warehouse keeper in chapter two.

#include <ansi.h>

inherit NPC;

string ask_plot()
{
	return "孟四把半张凭据按在案台上：另一半不是我藏的，是有人怕船户把两半合起来。你若已经看懂两份回浪印，就去拦住那个青篷客。\n";
}

void create()
{
	set_name("孟四", ({ "meng si", "meng", "warehouse keeper" }));
	set("gender", "男性");
	set("age", 58);
	set("long", "旧仓看守，手背上有多年搬箱留下的裂口。他曾替船户藏过互保凭据。\n");
	set("plot_lifecycle", "instance_essential");
	set("plot_chapter", "returning_mark_02");
	set("combat_exp", 900);
	set("attitude", "friendly");
	set("max_qi", 300);
	set("qi", 300);
	set("max_jing", 180);
	set("jing", 180);
	set("inquiry", ([ "回浪印" : (: ask_plot :), "凭据" : (: ask_plot :) ]));
	set_skill("literate", 75);
	setup();
}

private void survive_injury()
{
	"/adm/daemons/plot/returning_mark"->record_meng_injury(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	reincarnate();
	set("eff_qi", query("max_qi"));
	set("qi", query("max_qi"));
	set("eff_jing", query("max_jing"));
	set("jing", query("max_jing"));
	set("plot_condition", "injured");
	message_vision("$N被撞倒在箱角，勉强护住了怀里的半张凭据。\n", this_object());
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
		set("long", "旧仓看守，手背上有多年搬箱留下的裂口。孟四受了伤，仍把半张凭据护在怀里。\n");
	}
}
