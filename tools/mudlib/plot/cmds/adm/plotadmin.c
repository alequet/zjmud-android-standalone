// Administrator diagnostics for the chapter plot platform.

#include <ansi.h>

inherit F_CLEAN_UP;

int main(object me, string arg)
{
	mapping result;
	mapping combat;
	mapping rooms;
	mapping state;
	string plot_id;
	string msg;
	int instance_refs;

	if (! wizardp(me))
		return 0;
	if (arg == "audit")
	{
		PLOT_D->ensure_player_state(me);
		state = me->query("plot");
		instance_refs = 0;
		foreach (plot_id in PLOT_D->query_plot_ids())
		{
			if (stringp(me->query_temp("plot/" + plot_id + "/instance")))
				instance_refs++;
			rooms = me->query_temp("plot/" + plot_id + "/instance_rooms");
			if (mapp(rooms)) instance_refs += sizeof(rooms);
		}
		write(sprintf("PLOT_AUDIT schema=%d state_chars=%d instance_refs=%d objects=%d\n",
			PLOT_D->selftest()["schema"], sizeof(sprintf("%O", state)),
			instance_refs, sizeof(objects())));
		return 1;
	}
	if (arg != "selftest")
		return notify_fail("用法：plotadmin selftest|audit\n");

	result = PLOT_D->selftest();
	combat = PLOT_COMBAT_D->selftest();
	msg = sprintf("PLOT_SELFTEST ok=%d schema=%d arc=%s errors=%O\n",
		result["ok"], result["schema"], result["arc"], result["errors"]);
	msg += sprintf("PLOT_COMBAT_SELFTEST ok=%d profiles=%d errors=%O\n",
		combat["ok"], combat["profiles"], combat["errors"]);
	write(result["ok"] && combat["ok"] ? HIG + msg + NOR : HIR + msg + NOR);
	return 1;
}

int help(object me)
{
	write("指令格式：plotadmin selftest|audit\n检查剧情注册表，或审计当前角色剧情状态和实例引用。\n");
	return 1;
}
