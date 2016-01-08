#ifndef __SERVER__
#define __SERVER__

#include <string>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>

#include "option.h"
#include "client.h"
#include "terminal.h"

template <typename endpoint_type>
struct endpoint_hasher {
	size_t operator()(const endpoint_type &e) const {
		size_t seed = 0;
		boost::hash_combine(seed, e.address().to_v4().to_ulong());
		boost::hash_combine(seed, e.port());
		return seed;
	}
};

class server {
public:
	option opt;
	boost::asio::io_service ioer_;	

	boost::asio::ip::tcp::acceptor client_server_;
	typedef std::unordered_map<std::string, client*> client_id_map;
	client_id_map client_id_map_;
		
	boost::asio::ip::udp::socket terminal_server_;
	typedef std::unordered_map<std::string, terminal*> terminal_id_map;
	terminal_id_map terminal_id_map_;
	typedef std::unordered_map<boost::asio::ip::udp::endpoint, terminal*, endpoint_hasher<boost::asio::ip::udp::endpoint>> terminal_endpoint_map;
	terminal_endpoint_map terminal_endpoint_map_;

	server();
	void run();
	void start_accept_client();
	void accept_client(client *c, const boost::system::error_code& ec);
	client* get_client(std::string &id);
	bool add_client(client *c);
	void del_client(client *c);

	void start_receive_terminal();
	void receive_terminal(terminal_message *buf, const boost::system::error_code& e, size_t len);
	terminal* get_terminal(std::string &id);
	terminal* get_terminal(boost::asio::ip::udp::endpoint &endpoint);
	bool add_terminal(terminal *t);
	void del_terminal(terminal *t);
};

extern server *g_server;

#endif
