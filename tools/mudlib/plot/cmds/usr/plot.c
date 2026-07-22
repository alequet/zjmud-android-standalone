// Player-facing chapter log.

#include <ansi.h>

inherit F_CLEAN_UP;

private string status_name(string status)
{
	if (status == "available") return HIG "可接取" NOR;
	if (status == "active") return HIY "进行中" NOR;
	if (status == "completed") return HIC "已完成" NOR;
	return WHT "未开放" NOR;
}

int main(object me, string arg)
{
	mapping info;
	string *ids;
	string plot_id;
	string action;
	string msg;

	if (! objectp(me))
		return 0;

	if (stringp(arg) && sscanf(arg, "act %s %s", plot_id, action) == 2)
	{
		mixed result;

		result = PLOT_D->dispatch_action(me, plot_id, action);
		if (stringp(result))
			write(result + "\n");
		return 1;
	}

	if (stringp(arg) && arg != "")
	{
		info = PLOT_D->query_player_chapter(me, arg);
		if (! mapp(info))
			return notify_fail("没有这条剧情记录。\n");
		msg = ZJOBLONG + HIY + info["name"] + NOR "\n\n";
		msg += "状态：" + status_name(info["status"]) + "\n";
		if (info["status"] == "active")
			msg += "当前阶段：" + info["stage"] + "\n";
		if (info["implemented"] && arg == "origin_letter_01")
			msg += "/adm/daemons/plot/origin_letter"->render_log(me) + "\n";
		if (info["implemented"] && arg == "returning_mark_02")
			msg += "/adm/daemons/plot/returning_mark"->render_log(me) + "\n";
		if (info["implemented"] && arg == "silent_crossing_03")
			msg += "/adm/daemons/plot/silent_crossing"->render_log(me) + "\n";
		if (info["implemented"] && arg == "visitors_from_home_04")
			msg += "/adm/daemons/plot/visitors_from_home"->render_log(me) + "\n";
		if (info["implemented"] && arg == "where_rivers_end_05")
			msg += "/adm/daemons/plot/where_rivers_end"->render_log(me) + "\n";
		if (! info["implemented"])
			msg += "本章内容尚未开放。\n";
		msg += ZJOBACTS2 + ZJMENUF(1, 1, 8, 30) + "返回剧情总览:plot\n";
		write(msg);
		return 1;
	}

	ids = PLOT_D->query_plot_ids();
	msg = ZJOBLONG "乡书篇\n\n从来处寄来的旧信，正牵出一条被人改写姓名与身份的暗线。\n"
		ZJOBACTS2 + ZJMENUF(2, 2, 9, 30);
	foreach (plot_id in ids)
	{
		info = PLOT_D->query_player_chapter(me, plot_id);
		msg += sprintf("第%s章 %s" ZJBR "%s:plot %s" ZJSEP,
			chinese_number(info["chapter"]), info["name"],
			status_name(info["status"]), plot_id);
	}
	write(msg + "\n");
	return 1;
}

int help(object me)
{
	write("指令格式：plot [剧情编号]\n查看章节剧情进度、线索和回顾。\n");
	return 1;
}
