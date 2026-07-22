// The real Lu Xiaoshuan, a protected chapter-five witness.

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/where_rivers_end"->ask_real_lu(this_player());
}

void create()
{
	set_name("陆小栓", ({ "lu xiaoshuan", "lu", "messenger" }));
	set("gender", "男性");
	set("age", 27);
	set("long", "真正的故乡信使，衣角还留着沿途驿站火漆。长期囚禁使他脸色苍白，但他说出的行程、口音和三短一长驿卒敲法彼此吻合。\n");
	set("plot_lifecycle", "instance_essential");
	set("plot_chapter", "where_rivers_end_05");
	set("combat_exp", 1500);
	set("attitude", "friendly");
	set("max_qi", 360);
	set("qi", 360);
	set("max_jing", 220);
	set("jing", 220);
	set("inquiry", ([ "假乡书" : (: ask_plot :), "沈观澜" : (: ask_plot :), "无面局" : (: ask_plot :) ]));
	setup();
}

private void survive_injury()
{
	"/adm/daemons/plot/where_rivers_end"->record_real_lu_injury(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	reincarnate();
	set("eff_qi", query("max_qi"));
	set("qi", query("max_qi"));
	set("eff_jing", query("max_jing"));
	set("jing", query("max_jing"));
	set("plot_condition", "injured");
	message_vision("$N被跌落的盐袋撞倒，受了伤，却仍把换名口供护在怀中。\n", this_object());
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
		set("long", "真正的故乡信使靠墙坐着，肩背受伤，仍把无面局换名口供护在怀中。\n");
	}
	else if (condition == "rescued")
	{
		set("plot_condition", "rescued");
		set("long", "真正的故乡信使已经脱离木栅，正整理沿途驿站火漆和无面局换名口供。\n");
	}
}
