// Offline replacement for the inter-MUD channel transport.

void create()
{
	seteuid(getuid());
}

void send_msg(string channel, string id, string name, string msg,
	      int emoted, mixed filter)
{
}

void incoming_request(mapping info)
{
}
