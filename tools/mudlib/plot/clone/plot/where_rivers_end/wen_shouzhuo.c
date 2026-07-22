// Wen Shouzhuo, the adaptive multi-phase chapter-five Boss.

inherit NPC;

void create()
{
	set_name("闻守拙", ({ "wen shouzhuo", "wen", "ledger master" }));
	set("gender", "男性");
	set("age", 58);
	set("long", "百川会掌簿人，灰衣袖口沾着多年盐霜和墨痕，齐眉棍缠手处留着一段褪色旧结绳。他确实让暗粮路救过人，也把旧恩写成欠据，并把副印交给了沈观澜。\n");
	set("attitude", "peaceful");
	set("plot_lifecycle", "instance_boss_nonlethal");
	set("plot_chapter", "where_rivers_end_05");
	set("combat_exp", 5000);
	set("level", 12);
	set("can_speak", 1);
	set("max_qi", 800);
	set("qi", 800);
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
	int next_phase;

	next_phase = "/adm/daemons/plot/where_rivers_end"->record_wen_defeat(this_object());
	remove_all_enemy(1);
	remove_all_killer();
	if (next_phase > 0)
		message_vision("$N的齐眉棍撞上铁案。他退入第" + chinese_number(next_phase + 1) +
			"层印阵，借盐仓机关重新守住印钥。\n", this_object());
	else
		message_vision("$N再握不住印钥，扶着铁案认输。他将活着接受三方共同核验与审理。\n", this_object());
	destruct(this_object());
}
