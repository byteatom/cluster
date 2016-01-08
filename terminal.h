#ifndef __TERMINAL__
#define __TERMINAL__

#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include "client.h"

class client;
class client_message;

class terminal_message {	
public:	
	static const size_t MAX_MSG_LEN = 65531;
	static const int REG_TAG_LEN = 3;
	static const char REG_TAG[REG_TAG_LEN + 1];
	static const int MIN_ID_LEN = 14;
	static const int MAX_ID_LEN = 18;
	static const int HTH_TAG_LEN = 4;
	static const char HTH_TAG[HTH_TAG_LEN + 1];
	
	char *buf_ = nullptr;
	size_t len_ = 0;
	boost::asio::ip::udp::endpoint ep_;
	bool is_register_ = false;
	std::string id_;
	int hth_ = 0;

	terminal_message(){buf_ = new char[MAX_MSG_LEN];};
	~terminal_message(){delete buf_;};

	bool decode();
};

std::ostream& operator<<(std::ostream& o, const terminal_message& msg);

class terminal {
public:
	boost::asio::ip::udp::socket *socket_ = nullptr;
	boost::asio::ip::udp::endpoint endpoint_;
	std::string id_;
	boost::asio::deadline_timer timer_;
	long timeout_;

	terminal(boost::asio::io_service &ioer, terminal_message *msg);
	~terminal();

	static void recv(terminal_message *buf, size_t len);
	void rcvd_heartbeat(terminal_message *msg);
	void send_heartbeat(terminal_message *msg);
	void sent_heartbeat(terminal_message *msg, const boost::system::error_code& e, size_t sent);
	void rcvd_data(terminal_message *msg);
	void relay_data(client_message *msg);
	void relaid_data(client_message *msg, const boost::system::error_code& e, size_t sent);
	void reset_timer();
	void timeout(const boost::system::error_code& e);
};

#endif
