// Luo Qiniang's chapter-five continuity encounter.

inherit NPC;

void create()
{
	set_name("罗七娘", ({ "luo qiniang", "luo qi", "qiniang" }));
	set("gender", "女性");
	set("age", 36);
	set("long", "罗七娘仍相信总簿一旦失控，暗粮路便会先断。她守的是吊桥和印钥，不是闻守拙的性命。\n");
	set("attitude", "peaceful");
	set("plot_lifecycle", "instance_continuity_opponent");
	set("plot_chapter", "where_rivers_end_05");
	set("combat_exp", 3500);
	set("level", 10);
	set("can_speak", 1);
	set("max_qi", 650);
	set("qi", 650);
	setup();
}

int plot_combat_action()
{
	return "/adm/daemons/plotcombat"->perform_enemy(this_object());
}

int plot_core_perform()
{
	object owner;

	owner = find_player(query("plot_owner"));
	if (is_busy()) this_object()->interrupt_busy(0);
	if (objectp(owner) && owner->is_busy()) owner->interrupt_busy(0);
	if (! is_fighting() && objectp(owner) && environment(owner) == environment())
	{
		kill_ob(owner);
		owner->kill_ob(this_object());
	}
	return "/adm/daemons/plotcombat"->perform_enemy_action(this_object(),
		query("plot_combat/core_perform"));
}

void enable_plot_combat_actions()
{
	set("chat_chance_combat", 80);
	set("chat_msg_combat", ({ (: plot_combat_action :) }));
}

void die()
{
	"/adm/daemons/plot/where_rivers_end"->record_luo_defeat(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	message_vision("$N短剑脱手，主动退出吊桥。她没有死，也再不能替总簿维持单方控制。\n", this_object());
	destruct(this_object());
}
