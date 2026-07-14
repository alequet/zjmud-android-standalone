// Offline replacement for the inter-MUD finger service.

void create() { seteuid(getuid()); }
void remote_finger(object source, string user, string mud) {}
void service_request(int id, mixed *params) {}
void read_callback(int id, string message) {}
void close_callback(int id) {}
void timeout(int id) {}
