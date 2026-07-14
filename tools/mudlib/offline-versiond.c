// Offline replacement for the networked version synchronization daemon.

inherit F_DBASE;

void create()
{
	seteuid(getuid());
	set("channel_id", "Offline version daemon");
}

private int unavailable()
{
	return notify_fail("Network version operations are unavailable in standalone mode.\n");
}

int generate_version() { return unavailable(); }
int build_cancel() { return unavailable(); }
int build_new() { return unavailable(); }
int build_path(string path) { return unavailable(); }
int synchronize_version() { return unavailable(); }
int fetch_file(string file) { return unavailable(); }
int append_sn(string file) { return unavailable(); }

int clear_syn_info() { return 0; }
int is_version_ok() { return 1; }
int is_boot_synchronizing() { return 0; }
int is_release_server() { return 1; }
int is_need_release_source(string path) { return 0; }

mixed query_client_info() { return 0; }
int query_vfd() { return 0; }
mapping query_socket_info() { return ([]); }
