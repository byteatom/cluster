#include "terminal.h"

#include <cstdlib>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "logger.h"

#include "server.h"

const char terminal_message::REG_TAG[terminal_message::REG_TAG_LEN + 1] = {'K','L','R', 0};
const char terminal_message::HTH_TAG[terminal_message::HTH_TAG_LEN + 1] = {',','H','T','H', 0};

bool terminal_message::decode() {
	if (nullptr == buf_)
		return false;

	if (0 != memcmp(buf_, terminal_message::REG_TAG, terminal_message::REG_TAG_LEN)) {
		is_register_ = false;
		return true;
	}
	is_register_ = true;
	char *pos = strstr(buf_, HTH_TAG);
	if (nullptr != pos) {			
		id_.assign(buf_ + REG_TAG_LEN, pos - buf_);
		hth_ = strtol(pos + HTH_TAG_LEN, nullptr, 0);
	}
	else {
		id_.assign(buf_ + REG_TAG_LEN, len_ - REG_TAG_LEN);
	}
	id_.resize(client_message::ID_LEN);

	return true;
}

std::ostream& operator<<(std::ostream& o, const terminal_message& msg) {
	if (msg.is_register_)
		o << msg.buf_;
#ifdef _DEBUG
	else {
		for (unsigned int i = 0; i < msg.len_; i++) {
			o << " " << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)(msg.buf_[i]);
		}
	}
#endif
  	return o;
}

terminal::terminal(boost::asio::io_service &ioer, terminal_message *msg)
	:timer_(ioer)
{	
	socket_ = &g_server->terminal_server_;
	endpoint_ = msg->ep_;
	id_ = msg->id_;
	g_server->add_terminal(this);	
	timeout_ = g_server->opt.terminal_timeout_;
}

terminal::~terminal() {
	g_server->del_terminal(this);
}

void terminal::recv(terminal_message *msg, size_t len) {	
	if (nullptr == msg) return;
	msg->len_ = len;
	msg->buf_[len] = 0;
	if (!msg->decode()) {
		delete msg;
		return;
	}	
	
	terminal *t = nullptr;
	terminal *t2 = nullptr;
	if (msg->is_register_) {		
		t = g_server->get_terminal(msg->id_);
		t2 = g_server->get_terminal(msg->ep_);
		if (t != t2) {
			LOG(WARNING) << "terminal conflict! ip: " << msg->ep_ << ", id: " << msg->id_;
			delete t;
			t = nullptr;
			delete t2;
			t2 = nullptr;
		}
		if (nullptr == t) {
			t = new terminal(g_server->ioer_, msg);
		}
		t->rcvd_heartbeat(msg);
	} else {		
		LOG(INFO) << "receive data from terminal " << msg->ep_ << " length " << msg->len_ << ": " << *msg;
		t = g_server->get_terminal(msg->ep_);
		if (nullptr == t) {
			//TODO
		}
		else {
			t->rcvd_data(msg);
		}
	}

	if (nullptr != t)
		t->reset_timer();
}

void terminal::rcvd_heartbeat(terminal_message *msg) {
	if (msg->hth_ > 0) {
		timeout_ = msg->hth_ * 2.5;
	}
	send_heartbeat(msg);
}

void terminal::send_heartbeat(terminal_message *msg) {
	//TODO: save msg
	//LOG(INFO) << "send message to terminal " << msg->ep_ << " " << *msg;
	socket_->async_send_to(boost::asio::buffer(msg->buf_, msg->len_),
		msg->ep_, boost::bind(&terminal::sent_heartbeat, this, msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void terminal::sent_heartbeat(terminal_message *msg, const boost::system::error_code& e, size_t sent) {
	delete msg;
}

void terminal::rcvd_data(terminal_message *msg) {
	
	client *c = g_server->get_client(id_);
	if (nullptr != c)
		c->relay_data(id_, msg);
	else
		delete msg;
}

void terminal::relay_data(client_message *msg) {
	LOG(INFO) << "relay data to terminal " << endpoint_ << " " << *msg;
	socket_->async_send_to(boost::asio::buffer(msg->head_->data, msg->head_->data_len),
		endpoint_, boost::bind(&terminal::relaid_data, this, 
		msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void terminal::relaid_data(client_message *msg, const boost::system::error_code& e, size_t sent) {
	LOG(INFO) << "relaid data to terminal " << endpoint_ << " " <<  e.message() << " " << *msg;
	delete msg;
}

void terminal::reset_timer() {
	timer_.expires_from_now(boost::posix_time::seconds(timeout_));
	timer_.async_wait(boost::bind(&terminal::timeout, this, boost::asio::placeholders::error));
}

void terminal::timeout(const boost::system::error_code& e) {
	if (e != boost::asio::error::operation_aborted) {
		LOG(WARNING) << "terminal " << id_ << " from " << endpoint_ << " timeout";
		delete this;
	}
}
