// Disposable nonlethal combat target for AI diagnostics.

inherit FIGHTER;

void create()
{
	set_name("测试木桩", ({ "ai test dummy", "ai_dummy", "dummy" }));
	set("gender", "无");
	set("age", 1);
	set("long", "一个只用于隔离测试的无害木桩。\n");
	set("ai_test_dummy", 1);
	set("no_get", 1);
	set("str", 1);
	set("con", 1);
	set("dex", 1);
	set("int", 1);
	set("max_qi", 500000);
	set("eff_qi", 500000);
	set("qi", 500000);
	set("max_jing", 1000);
	set("eff_jing", 1000);
	set("jing", 1000);
	set("max_neili", 1);
	set("neili", 1);
	set("jiali", 0);
	set("combat_exp", 1);
	set_skill("force", 1);
	set_skill("unarmed", 1);
	set_skill("dodge", 1);
	set_skill("parry", 1);
	setup();
}

void configure_for_ai(object me, string mode)
{
	int exp;

	if (! objectp(me) || ! stringp(mode))
		return;
	if (member_array(mode, ({ "retreat", "noexit", "blocked" })) != -1)
	{
		exp = me->query("combat_exp") * 3;
		if (exp < 1)
			exp = 1;
		set("combat_exp", exp);
		set("ai_test_threat", "overmatched");
	} else
	{
		set("combat_exp", 1);
		set("ai_test_threat", "weak");
	}
	set("ai_test_mode", mode);
}
