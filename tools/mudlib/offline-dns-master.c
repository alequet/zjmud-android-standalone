// Offline replacement for the inter-MUD DNS and UDP daemon.

private mapping muds = ([]);
private mapping services = ([]);
private int sequence;

void create()
{
	seteuid(getuid());
}

int startup_udp() { return 0; }
void send_udp(string host, int port, string msg) {}
void send_shutdown() {}
void end_shutdown() {}
int query_udp_port() { return 0; }
string query_mud_name() { return "standalone"; }
string start_message() { return ""; }

void init_database() {}
void refresh_database() {}
void do_pings() {}

void set_mud_info(string name, mapping info) {}
varargs void zap_mud_info(string name, mapping info) {}
void support_q_callback(mapping info) {}

int query_service_method(string mud, string service) { return 0; }
mapping query_svc_entry(string mud) { return 0; }
string get_host_name(string name) { return 0; }
int get_mudresource(string mud, string resource) { return 0; }
mapping current_mud_info() { return ([]); }
mapping query_mud_info(string name) { return 0; }
int dns_mudp(string name) { return 0; }
mapping query_muds() { return muds; }
mapping query_svc() { return services; }

varargs int idx_request(function callback)
{
	sequence++;
	return sequence;
}

void idx_callback(int index, mixed param) {}
void sequence_callback(int index, mixed param) {}
void sequence_clean_up() {}
void dump_sequencer() {}
void dump_svc() {}
void dump_mud_keys() {}
void dump_svc_keys() {}
void set_monitor(object ob) {}
object query_monitor() { return 0; }
void aux_log(string file, string entry) {}
void aux_warning(string warning) {}
void resolve_callback(string address, string ip, int key) {}
