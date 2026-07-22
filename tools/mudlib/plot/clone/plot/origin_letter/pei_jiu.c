// Pei Jiu, the bounded combat and negotiation opponent in the warehouse.

#include <ansi.h>

inherit NPC;

void create()
{
	set_name("裴九", ({ "pei jiu", "pei", "jiu" }));
	set("gender", "男性");
	set("age", 34);
	set("long", "受雇截簿的江湖散人。他的刀已经出鞘，却没有立刻向你扑来。\n");
	set("attitude", "peaceful");
	set("plot_lifecycle", "instance_opponent");
	set("plot_chapter", "origin_letter_01");
	set("can_speak", 1);
	set("combat_exp", 2000);
	set("level", 9);
	set_skill("unarmed", 50);
	set_skill("dodge", 50);
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
	"/adm/daemons/plot/origin_letter"->record_pei_death(this_object());
	::die();
}
