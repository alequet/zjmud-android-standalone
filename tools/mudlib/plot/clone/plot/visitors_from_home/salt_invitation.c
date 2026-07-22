// Story reward: verified fragment of the old salt-storehouse invitation.

#include <ansi.h>

inherit ITEM;

void create()
{
	set_name(HIW "旧盐仓请帖残件" NOR, ({ "salt invitation", "invitation fragment", "invitation" }));
	set_weight(20);
	set("unit", "张");
	set("long", "一张经过三方核验的请帖残件，潮次、仓号和百川总簿四字仍清晰可辨。它不提供属性优势。\n");
	set("value", 0);
	set("no_sell", 1);
	set("no_drop", 1);
	set("plot_item", "visitors_from_home_04");
	setup();
}
