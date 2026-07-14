// Offline replacement for the external telnet shadow.

int is_telneting() { return 0; }
string query_dest_addr() { return ""; }

void connect_to(string address)
{
	notify_fail("External telnet is unavailable in standalone mode.\n");
}

void remove_interactive() {}
varargs string short(int raw) { return 0; }
