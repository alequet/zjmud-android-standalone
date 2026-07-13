// Android standalone full character persistence.

#define FULL_SAVE_VERSION 1
#define FULL_SAVE_MAX_DEPTH 12
#define FULL_SAVE_MAX_ITEMS 500

int full_save_version;
string full_save_room;
mixed *full_save_inventory;

static int full_save_item_count;
static mixed *full_restore_equipment;
static int full_restore_pending;

private int full_value_saveable(mixed value, int depth)
{
	mixed key;

	if (depth > FULL_SAVE_MAX_DEPTH)
		return 0;

	if (intp(value) || stringp(value) || floatp(value))
		return 1;

	if (arrayp(value))
	{
		foreach (mixed entry in value)
			if (! full_value_saveable(entry, depth + 1))
				return 0;
		return 1;
	}

	if (mapp(value))
	{
		foreach (key in keys(value))
		{
			if (! intp(key) && ! stringp(key))
				return 0;
			if (! full_value_saveable(value[key], depth + 1))
				return 0;
		}
		return 1;
	}

	return 0;
}

private mapping full_copy_dbase(object ob)
{
	mapping source;
	mapping result;
	mixed key;

	result = ([]);
	if (! mapp(source = ob->query_entire_dbase()))
		return result;

	foreach (key in keys(source))
	{
		if ((stringp(key) || intp(key)) &&
		    key != "equipped" &&
		    full_value_saveable(source[key], 0))
			result[key] = source[key];
	}

	return result;
}

private mapping full_snapshot_item(object ob, int depth)
{
	mapping snapshot;
	mixed parameter;
	mixed *children;
	object child;
	string file;
	string state;

	if (! objectp(ob) || ob->is_character() ||
	    depth > FULL_SAVE_MAX_DEPTH ||
	    full_save_item_count >= FULL_SAVE_MAX_ITEMS)
		return 0;

	file = base_name(ob);
	if (! stringp(file) || file == "" || file[0] != '/')
		return 0;

	full_save_item_count++;
	children = ({});
	foreach (child in all_inventory(ob))
	{
		mapping saved_child;

		saved_child = full_snapshot_item(child, depth + 1);
		if (mapp(saved_child))
			children += ({ saved_child });
	}

	state = ob->query("equipped");
	snapshot = ([
		"file" : file,
		"dbase" : full_copy_dbase(ob),
		"children" : children,
	]);

	if (state == "worn" || state == "wielded")
		snapshot["equipped"] = state;

	if (function_exists("query_autoload", ob) &&
	    ! catch(parameter = ob->query_autoload()) &&
	    full_value_saveable(parameter, 0))
		snapshot["autoload"] = parameter;

	return snapshot;
}

private int full_room_allowed(string file)
{
	object room;
	string error;

	if (! stringp(file) || file == "" || file[0] != '/' ||
	    file == "/d/death/block" || file == "/d/death/shenxun" ||
	    file_size(file + ".c") < 0)
		return 0;

	error = catch(call_other(file, "???"));
	room = find_object(file);
	if (error || ! objectp(room) || ! room->is_room() ||
	    room->query("close_room") || room->query("out_room") ||
	    room->query("no_save_location") || room->query("no_login"))
		return 0;

	return 1;
}

void save_full_character_state()
{
	object room;
	object item;
	mixed *inventory;
	mapping snapshot;
	string file;

	room = environment(this_object());
	if (objectp(room) && room->is_room() && ! clonep(room))
	{
		file = base_name(room);
		if (full_room_allowed(file))
			full_save_room = file;
	}

	full_save_item_count = 0;
	inventory = ({});
	foreach (item in all_inventory(this_object()))
	{
		snapshot = full_snapshot_item(item, 0);
		if (mapp(snapshot))
			inventory += ({ snapshot });
	}

	full_save_inventory = inventory;
	full_save_version = FULL_SAVE_VERSION;
}

void prepare_full_character_restore()
{
	full_restore_pending = full_save_version > 0 &&
			       arrayp(full_save_inventory);
}

int has_full_character_state()
{
	return full_restore_pending && full_save_version > 0 &&
	       arrayp(full_save_inventory);
}

string query_full_save_room()
{
	return full_room_allowed(full_save_room) ? full_save_room : 0;
}

private void full_apply_dbase(object ob, mapping data)
{
	mixed key;

	if (! mapp(data))
		return;

	foreach (key in keys(data))
		if ((stringp(key) || intp(key)) &&
		    key != "equipped")
			catch(ob->set(key, data[key]));
}

private object full_restore_item(mapping snapshot, object destination, int depth)
{
	object ob;
	object prototype;
	mixed parameter;
	mixed *children;
	mapping child;
	string file;
	string error;
	string state;

	if (! mapp(snapshot) || ! objectp(destination) ||
	    depth > FULL_SAVE_MAX_DEPTH)
		return 0;

	file = snapshot["file"];
	children = snapshot["children"];
	if (! stringp(file) || file == "" || file[0] != '/' ||
	    file_size(file + ".c") < 0)
	{
		if (arrayp(children))
			foreach (child in children)
				full_restore_item(child, destination, depth + 1);
		return 0;
	}

	error = catch(call_other(file, "???"));
	prototype = find_object(file);
	if (! error && objectp(prototype))
	{
		if (prototype->is_no_clone())
		{
			if (! environment(prototype) &&
			    ! prototype->is_not_belong_me(this_object()))
				ob = prototype;
		} else
			ob = new(file);
	}

	if (! objectp(ob) || ob->is_character())
	{
		if (arrayp(children))
			foreach (child in children)
				full_restore_item(child, destination, depth + 1);
		log_file("fullsave", sprintf("Unable to restore %s for %s.\n",
			 file, this_object()->query("id")));
		return 0;
	}

	export_uid(ob);
	full_apply_dbase(ob, snapshot["dbase"]);
	if (! undefinedp(snapshot["autoload"]) &&
	    function_exists("autoload", ob))
	{
		parameter = snapshot["autoload"];
		catch(ob->autoload(parameter, this_object()));
	}

	if (! ob->move(destination, 1) || ! objectp(ob))
	{
		if (objectp(ob))
			destruct(ob);
		if (arrayp(children))
			foreach (child in children)
				full_restore_item(child, destination, depth + 1);
		return 0;
	}

	if (arrayp(children))
		foreach (child in children)
			full_restore_item(child, ob, depth + 1);

	state = snapshot["equipped"];
	if (state == "worn" || state == "wielded")
		full_restore_equipment += ({ ([ "object" : ob, "state" : state ]) });

	return ob;
}

private void full_equip_restored_items()
{
	mapping entry;
	object ob;

	foreach (entry in full_restore_equipment)
	{
		ob = entry["object"];
		if (! objectp(ob) || environment(ob) != this_object())
			continue;
		if (entry["state"] == "worn")
			catch(ob->wear());
	}

	foreach (entry in full_restore_equipment)
	{
		ob = entry["object"];
		if (! objectp(ob) || environment(ob) != this_object())
			continue;
		if (entry["state"] == "wielded")
			catch(ob->wield());
	}
}

void restore_full_character_state()
{
	mapping snapshot;

	if (! has_full_character_state())
		return;

	full_restore_pending = 0;
	full_restore_equipment = ({});
	foreach (snapshot in full_save_inventory)
		full_restore_item(snapshot, this_object(), 0);
	full_equip_restored_items();

	full_restore_equipment = 0;
	full_save_inventory = 0;
}
