// Offline replacement for the external MUD Who client.

void create()
{
	seteuid(getuid());
}

void add_user(object user, int partition) {}
void add_all_users(int partition) {}
void refresh(int partition) {}
void keepalive() {}
void boot() {}
void halt() {}
void remove_user(object user) {}
