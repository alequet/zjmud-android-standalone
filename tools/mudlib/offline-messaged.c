// Offline replacement for the cross-MUD message daemon.

#include <ansi.h>
#include <net/messaged.h>

inherit F_DBASE;

private string last_error_msg;
private mapping connection = ([]);
private mapping sending_queue = ([]);

void create()
{
	seteuid(getuid());
	set("channel_id", "Offline message daemon");
}

int error_msg(string msg)
{
	last_error_msg = msg;
	return 0;
}

string query_last_error()
{
	return last_error_msg;
}

object find_chatter(string user)
{
	object ob;
	object *chatters;

	if (objectp(ob = connection[user]))
		return ob;

	chatters = children(CHATTER_OB);
	foreach (ob in chatters)
		if (objectp(ob) && ob->is_chatter() && ob->query("id") == user)
			return ob;

	return 0;
}

object find_user(string user)
{
	object ob;

	if (objectp(ob = find_player(user)))
		return ob;
	return find_chatter(user);
}

string reject_tell(object from, object to)
{
	string no_tell;
	string can_tell;
	string to_name;

	no_tell = to->query("env/no_tell");
	can_tell = to->query("env/can_tell");
	to_name = to->name(1) + "(" + to->query("id") + ")";

	if (! wiz_level(from) &&
	    (no_tell == "all" || no_tell == "ALL" ||
	     is_sub(from->query("id"), no_tell)) &&
	    ! is_sub(from->query("id"), can_tell))
		return to_name + " does not accept messages from you.\n";

	if (! function_exists("is_chatter", to) && ! interactive(to))
		return to_name + " is not online.\n";
	if (! living(to))
		return to_name + " cannot hear you now.\n";
	return 0;
}

varargs void tell_user(mixed user, string fun, string msg, string add)
{
	object ob;

	if (! objectp(ob = user) && stringp(user))
		ob = find_user(user);
	if (! objectp(ob) || ! stringp(msg))
		return;

	// Chatter objects require the removed UDP client to consume packets.
	if (function_exists("is_chatter", ob) && ob->is_chatter())
		return;
	if (! interactive(ob) && function_exists("receive_message", ob))
	{
		ob->receive_message("write", msg);
		return;
	}
	tell_object(ob, msg);
}

int send_msg_to(mixed user, string msgto, string msg)
{
	object from;
	object to;
	string rejected;
	int i;

	if (! objectp(from = user) && stringp(user))
		from = find_user(user);
	if (! objectp(from))
		return error_msg("You are not logged in.\n");
	if (sscanf(msgto, "%*s@%*s") == 2)
		return error_msg("Inter-MUD messaging is unavailable in standalone mode.\n");

	to = find_user(msgto);
	if (! objectp(to) || ! from->visible(to))
		return error_msg("That user is not online.\n");
	if (to == from)
		return error_msg("You do not need to message yourself.\n");
	if (stringp(rejected = reject_tell(from, to)))
		return error_msg(rejected);

	i = strlen(msg);
	while (--i >= 0 && msg[i] == '\n');
	if (i < 0)
		return 1;
	msg = msg[0..i];

	to->set_temp("reply", from->query("id"));
	tell_user(to, FUN_TELL,
		HIG + from->name(1) + "(" + from->query("id") + ") tells you: " +
		msg + NOR "\n");
	tell_user(from, FUN_ACKTELL,
		HIG "You tell " + to->name(1) + ": " + msg + NOR "\n");
	return 1;
}

varargs void send_env(mixed user, mixed which)
{
	// Environment packets only existed for the removed remote chatter client.
}

void user_logout(mixed user, string msg)
{
	object ob;

	if (! objectp(ob = user) && stringp(user))
		ob = find_chatter(user);
	if (! objectp(ob))
		return;

	map_delete(connection, ob->query("id"));
	map_delete(sending_queue, ob);
	if (stringp(msg) &&
	    (! function_exists("is_chatter", ob) || ! ob->is_chatter()))
		tell_object(ob, msg);
	destruct(ob);
}

void remove_user(object user)
{
	user_logout(user, 0);
}

mixed query_connection(string user)
{
	if (! stringp(user) || user == "")
		return connection;
	return connection[user];
}

mixed query_sending_queue(object user)
{
	if (! objectp(user))
		return sending_queue;
	return sending_queue[user];
}

int query_udp_port()
{
	return 0;
}
