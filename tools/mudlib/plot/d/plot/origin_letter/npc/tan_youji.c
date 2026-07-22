// Thin yamen witness for origin_letter_01. Plot state stays in the controller.

inherit NPC;

string ask_plot()
{
	return "/adm/daemons/plot/origin_letter"->yamen_inquiry(this_player());
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
	set_name("谭友纪", ({ "tan youji", "tan", "clerk" }));
	set("gender", "男性");
	set("age", 36);
	set("long", "衙门书吏谭友纪，正把一叠签收卷宗摊在案上。\n");
	set("attitude", "friendly");
	set("plot_lifecycle", "world_refresh");
	set("plot_chapter", "origin_letter_01");
	set("inquiry", ([
		"乡书" : (: ask_plot :),
		"账簿" : (: ask_plot :),
		"义仓" : (: ask_plot :),
		"旧印" : (: ask_mark :),
		"回浪印" : (: ask_mark :),
		"伪信" : (: ask_visitors :),
		"故园来客" : (: ask_visitors :),
	]));
	set_skill("literate", 80);
	setup();
}
