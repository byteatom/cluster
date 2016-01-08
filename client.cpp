#include "client.h"

#include <stdexcept>
#include <string>
#include <iomanip>

#include <boost/bind.hpp>

#include "logger.h"

#include "server.h"



const char client_message::REG_TAG[client_message::REG_TAG_LEN] = 
	{ 0xEF, 0xCD, 0xAb, 0x89, 0x67, 0x45, 0x23, 0x01};

client_message::client_message() {
	buf_ = new char[MAX_BUF_LEN];
	head_ = (head*)buf_;
	len_ = 0;
}

client_message::client_message(std::string id, terminal_message *msg) {
	len_ = sizeof(head) + msg->len_;
	buf_ = new char[len_];
	head_ = (head*)buf_;
	std::copy(id.begin(), id.end(), head_->id);
	head_->type = TYPE_DATA;
	head_->reserved = 0;
	head_->data_len = msg->len_;
	memcpy(head_->data, msg->buf_, msg->len_);
}

client_message::~client_message() {
	delete buf_;
}

std::ostream& operator<<(std::ostream& o, const client_message& msg) {
	if (nullptr != msg.head_) {
		o << "id:" << msg.head_->id;
		o << ", type:" << (int)msg.head_->type;
		o << ", data_len:" << msg.head_->data_len;
#ifdef _DEBUG
		if (msg.head_->data_len > 0) {
			o << ", data:";
			for (int i = 0; i < msg.head_->data_len; i++) {
				o << " " << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)(msg.head_->data[i]);
			}
		}
#endif
	}
  	return o;
}

client::~client() {
	g_server->del_client(this);
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	socket_.close();
}

void client::recv_field_tag() {
	client_message *msg = new client_message();
	async_read(socket_, boost::asio::buffer(msg->buf_, msg->REG_TAG_LEN), 
		boost::bind(&client::rcvd_field_tag, this, 
		msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void client::rcvd_field_tag(client_message *msg, const boost::system::error_code& e, size_t len) {
	if (nullptr == msg || e || len != msg->REG_TAG_LEN) {		
		error(msg);
		return;
	}
	
	if (0 != memcmp(msg->buf_, msg->REG_TAG, msg->REG_TAG_LEN)) {		
		error(msg);
		return;
	}

	LOG(INFO) << "receive register from client " << socket_.remote_endpoint();

	recv_field_head(msg);
}

void client::recv_field_head(client_message *msg) {
	async_read(socket_, boost::asio::buffer(msg->buf_, sizeof(client_message::head)),
		boost::bind(&client::rcvd_field_head, this,
		msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void client::rcvd_field_head(client_message *msg, const boost::system::error_code& e, size_t len) {
	if (nullptr == msg || e || len != sizeof(client_message::head)) {
		error(msg);
		return;
	}	

	msg->len_ += len;
	id_.assign(msg->head_->id, client_message::ID_LEN);
	if (!g_server->add_client(this)) {
		error(msg);
		return;
	};

	async_read(socket_, boost::asio::buffer(msg->buf_ + msg->len_, msg->head_->data_len),
		boost::bind(&client::rcvd_field_data, this, 
		msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void client::rcvd_field_data(client_message *msg, const boost::system::error_code& e, size_t len) {
	if (nullptr == msg || e || len != msg->head_->data_len) {
		error(msg);
		return;
	}		
	
	msg->len_ += len;

	LOG(INFO) << "receive message from client " << socket_.remote_endpoint() << " " << *msg;
	
	switch (msg->head_->type) {
		case client_message::TYPE_DATA:
			rcvd_data(msg);
			break;
		case client_message::TYPE_GET_STATUS:
			rcvd_status(msg);
			break;
		default:
			LOG(FATAL) << "unknown client message type " << msg->head_->type;
			delete msg;
			break;
	}

	recv_field_head(new client_message());
}

void client::rcvd_data(client_message *msg) {
	terminal *t = g_server->get_terminal(std::string(msg->head_->id, client_message::ID_LEN));
	if (nullptr != t) 
		t->relay_data(msg);
	else
		send_result(msg, client_message::TYPE_SEND_RESULT, client_message::RELAY_NOSUCHDTU);	
}

void client::rcvd_status(client_message *msg) {
	int result = (nullptr == g_server->get_terminal(std::string(msg->head_->id, msg->ID_LEN))?
		client_message::STATUS_OFFLINE : client_message::STATUS_ONLINE);
	send_result(msg, client_message::TYPE_GET_STATUS, result);
}

void client::send_result(client_message *msg, client_message::message_type type, int result) {
	msg->head_->type = type;
	msg->head_->data_len = 1;
	msg->head_->data[0] = result;

	LOG(INFO) << "send message to client " << socket_.remote_endpoint() << " " << *msg;
	
	async_write(socket_, boost::asio::buffer(msg->head_, 1 + sizeof(client_message::head)),
		boost::bind(&client::sent_result, this, msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void client::sent_result(client_message *msg, const boost::system::error_code& e, size_t sent) {
	delete msg;
}

void client::relay_data(std::string &id, terminal_message *t_msg) {
	client_message *c_msg = new client_message(id, t_msg);

	LOG(INFO) << "relay message to client " << socket_.remote_endpoint() << " " << *c_msg;
	
	async_write(socket_, boost::asio::buffer(c_msg->buf_, c_msg->len_),
		boost::bind(&client::relaid_data, this, c_msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	delete t_msg;
}

void client::relaid_data(client_message *msg, const boost::system::error_code& e, size_t sent) {
	delete msg;
}

void client::error(client_message *msg) {
	if (msg) delete msg;
	delete this;
	LOG(WARNING) << "client transmit error";
	return;
}


