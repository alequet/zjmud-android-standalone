// Offline replacement for the external payment callback listener.

inherit F_DBASE;

private int persistent;

void create()
{
	seteuid(getuid());
	set("channel_id", "Offline payment daemon");
}

int query_accesses() { return 0; }
void close_connection(int fd) {}
void set_persistent(int which) { persistent = which; }
string http_header(string code) { return ""; }
string common_date(int timestamp) { return ctime(timestamp); }
void do_get(string request) {}
void do_post(int fd, string request, string body) {}
