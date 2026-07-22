// Stone Seven, posing as Lu Xiaoshuan in chapter four.

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/visitors_from_home"->question_visitor(this_player());
}

void create()
{
	set_name("陆小栓", ({ "lu xiaoshuan", "lu", "xiaoshuan", "stone seven" }));
	set("real_name", "石七");
	set("gender", "男性");
	set("age", 27);
	set("long", "自称由乡人推举来扬州送信的年轻人。他熟悉信封格式，却总把话题引向你已经保存的证据。\n");
	set("attitude", "friendly");
	set("plot_lifecycle", "instance_essential");
	set("plot_chapter", "visitors_from_home_04");
	set("combat_exp", 1800);
	set("max_qi", 360);
	set("qi", 360);
	set("max_jing", 220);
	set("jing", 220);
	set("inquiry", ([ "乡书" : (: ask_plot :), "来处" : (: ask_plot :), "证据" : (: ask_plot :) ]));
	setup();
}

private void survive_injury()
{
	"/adm/daemons/plot/visitors_from_home"->record_stone_injury(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	reincarnate();
	set("eff_qi", query("max_qi"));
	set("qi", query("max_qi"));
	set("eff_jing", query("max_jing"));
	set("jing", query("max_jing"));
	set("plot_condition", "injured");
	message_vision("$N护住头脸退到廊柱后，包袱里掉出一张杭州纸行的封套。\n", this_object());
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
		set("long", "自称陆小栓的来客受了伤，仍紧盯着装证据的信匣；杭州纸行封套已经从包袱里露了出来。\n");
	}
}
