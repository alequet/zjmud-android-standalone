// Offline replacement for legacy inter-MUD service objects.

void create() { seteuid(getuid()); }
void incoming_request(mapping info) {}

varargs void send_msg(string channel, string id, string name, string msg,
		      int emoted, mixed filter) {}
varargs int send_gtell(string mud, string user, mixed source, string msg,
		       string display_name) { return 0; }
string send_gfinger_q(string mud, string user, mixed source)
{
	return "Inter-MUD lookup is unavailable in standalone mode.\n";
}
void send_remote_q(string mud, string channel, string me,
		   string who, string msg) {}
void send_ping_q(string host, mixed port) {}
void send_mudlist_q(string host, string port) {}
void send_rwho_q(string mud, object source, int verbose) {}
void send_support_q(string host, mixed port, string command, string param) {}
void send_affirmation_a(string host, string port, string from, string to,
			string message) {}
void send_shutdown(string host, int port) {}

varargs void check_for_mail(string mud, int flag) {}
string query_save_file() { return DATA_DIR + "offline-netmail"; }
void get_status() {}
void clear_queues() {}

int process_list(string index, mapping info) { return 0; }
int clear_db_flag() { return 0; }
int query_db_flag() { return 0; }
int visiblep(object ob) { return 1; }
string get_name(object ob) { return capitalize(geteuid(ob)); }
int support_rwho_q_VERBOSE() { return 0; }
int support_tcp_mail() { return 0; }
int support_tcp_tell() { return 0; }
int support_tcp_finger() { return 0; }
int support_tcp_interwiz() { return 0; }
