#ifndef __CLIENT__
#define __CLIENT__

#include <unordered_map>

#include <boost/asio.hpp>

#include "terminal.h"

class terminal;
class terminal_message;

class client_message {
public:	
	static const int ID_LEN = 16; //TODO: 14 or 18?
	static const int NAME_LEN = 16; //TODO: 14 or 18?		
	static const int REG_TAG_LEN = 8;
	static const char REG_TAG[REG_TAG_LEN];
	
	typedef enum {
		TYPE_DATA = 0,
		TYPE_GET_STATUS = 3,
		TYPE_SEND_RESULT = 4,
		TYPE_STATUS_RESULT = 5,
	}message_type;

	typedef enum {
		RELAY_SUCCESS,
		RELAY_NOSUCHDTU,
		RELAY_OFFLINE,
		RELAY_CONGESTED,
	}relay_result;

	typedef enum {
		STATUS_NOSUCHGPRS,
		STATUS_ONLINE,
		STATUS_OFFLINE,
	}status_result;
	
	typedef struct {
		char id[ID_LEN];
		unsigned char type;	/*enum message_type*/
		unsigned char reserved;
		unsigned short data_len;
		char data[0];
	}head;
	static const int MAX_BUF_LEN = sizeof(head)+1492;

	char *buf_ = nullptr;
	head *head_ = nullptr;
	size_t len_ = 0;
	bool is_register_ = false;

	client_message();
	client_message(std::string id, terminal_message *buf);
	~client_message();
};

std::ostream& operator<<(std::ostream& o, const client_message& msg);

class client {
public:	
	boost::asio::ip::tcp::socket socket_;
	std::string id_;

	client(boost::asio::io_service *ioer):
		socket_(*ioer){};
	~client();

	void recv_field_tag();
	void rcvd_field_tag(client_message *msg, const boost::system::error_code& e, size_t len);
	void recv_field_head(client_message *msg);
	void rcvd_field_head(client_message *msg, const boost::system::error_code& e, size_t len) ;
	void rcvd_field_data(client_message *msg, const boost::system::error_code& e, size_t len);
	void rcvd_data(client_message *msg);
	void rcvd_status(client_message *msg);
	void send_result(client_message *msg, client_message::message_type type, int result);
	void sent_result(client_message *msg, const boost::system::error_code& e, size_t sent);
	void relay_data(std::string &id, terminal_message *t_msg);
	void relaid_data(client_message *msg, const boost::system::error_code& e, size_t sent);
	void error(client_message *msg);
};

#endif
