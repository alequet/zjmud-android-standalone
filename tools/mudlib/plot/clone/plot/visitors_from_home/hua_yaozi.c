// Hua Yaozi, a recurring courier and adaptive chapter-four opponent.

inherit NPC;

void create()
{
	set_name("花鹞子", ({ "hua yaozi", "hua", "yaozi" }));
	set("gender", "男性");
	set("age", 32);
	set("long", "替百川会外围换封递帖的信使，衣襟里藏着旧盐仓请帖，腰间束带卷得异乎寻常，脚下始终留着两条退路。\n");
	set("attitude", "peaceful");
	set("plot_lifecycle", "instance_continuity_opponent");
	set("plot_chapter", "visitors_from_home_04");
	set("combat_exp", 2600);
	set("level", 10);
	set("can_speak", 1);
	set("max_qi", 560);
	set("qi", 560);
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
	"/adm/daemons/plot/visitors_from_home"->record_hua_defeat(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	message_vision("$N被截在界碑前，只得丢下请帖、翻入浅沟遁走。\n", this_object());
	destruct(this_object());
}
