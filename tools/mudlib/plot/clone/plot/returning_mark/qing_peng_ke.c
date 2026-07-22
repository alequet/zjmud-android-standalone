// Qing Peng Ke, a bounded unarmed opponent in chapter two.

inherit NPC;

void create()
{
	set_name("青篷客", ({ "qing peng ke", "qing peng", "peng ke" }));
	set("gender", "男性");
	set("age", 31);
	set("long", "戴青篷斗笠的受雇跑腿人，袖中没有兵器，靠短促的擒拿和步法抢证。\n");
	set("attitude", "peaceful");
	set("plot_lifecycle", "instance_opponent");
	set("plot_chapter", "returning_mark_02");
	set("combat_exp", 2000);
	set("level", 9);
	set("can_speak", 1);
	set("max_qi", 500);
	set("qi", 500);
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
	"/adm/daemons/plot/returning_mark"->record_qing_death(this_object());
	::die();
}
