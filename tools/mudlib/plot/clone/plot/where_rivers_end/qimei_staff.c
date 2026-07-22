// Non-dropping plot weapon for Wen Shouzhuo's fixed Gaibang old-branch style.

#include <weapon.h>

inherit STAFF;

void create()
{
	set_name("齐眉棍", ({ "qimei gun", "gun", "staff" }));
	set_weight(3000);
	if (clonep())
		set_default_object(__FILE__);
	else
	{
		set("unit", "根");
		set("long", "一根桦木白蜡齐眉棍，缠手处留着褪色旧结绳。\n");
		set("value", 0);
		set("no_sell", 1);
		set("no_get", 1);
		set("plot_lifecycle", "instance_weapon");
		set("wield_msg", "$N抽出一根$n握在手中。\n");
		set("unwield_msg", "$N将$n横回腕侧。\n");
		init_staff(15);
		setup();
	}
}
