#include "server.h" 

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include "logger.h"

server *g_server = nullptr;

int main(int argc, char* argv[])
{
	try {
		Logger log("cluster", "./log");
		g_server = new server;
		g_server->run();
	} catch (std::exception &e) {
		LOG(FATAL) << e.what();
		return -1;
	}
	return 0;
}

server::server() : client_server_{ ioer_, boost::asio::ip::tcp::v4() }, terminal_server_{ ioer_, boost::asio::ip::udp::v4() }
{	
}

void server::run() {
	
	opt.load();
	
	client_server_.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), opt.client_port_));	
	client_server_.listen();
	start_accept_client();
	LOG(INFO) << "start accept client on " << client_server_.local_endpoint();
	
	terminal_server_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), opt.terminal_port_));
	start_receive_terminal();
	LOG(INFO) << "start receive from " << terminal_server_.local_endpoint();

	ioer_.run();
}

void server::start_accept_client(){	
	client *c = new client(&ioer_);	
	client_server_.async_accept(c->socket_, boost::bind(
		&server::accept_client, this, c, boost::asio::placeholders::error));
}

void server::accept_client(client *c, const boost::system::error_code& e) {
	start_accept_client();
	if (!e ) {
		c->recv_field_tag();
	} else {
		delete c;
	}
}

client* server::get_client(std::string &id) {
	auto it = client_id_map_.find(id);
	if (it == client_id_map_.end())
		return nullptr;
	else
		return it->second;
}

bool server::add_client(client *c) {
	client *old = get_client(c->id_);
	if (nullptr != old)
	{
		if (c == old)
			return true;
		else {
			LOG(WARNING) << "terminal " << c->id_ << " ocupied by " << old->socket_.remote_endpoint();
			return false;
		}
	} else {
		client_id_map_.insert({c->id_, c});
		LOG(INFO) << "add client " << c->socket_.remote_endpoint() << " for terminal " << c->id_;
		return true;
	}	
}

void server::del_client(client *c) {
	client_id_map_.erase(c->id_);
	LOG(INFO) << "delete client " << c->socket_.remote_endpoint() << " for terminal " << c->id_;
}

void server::start_receive_terminal()
{	
	terminal_message *msg = new terminal_message();
	
	terminal_server_.async_receive_from(boost::asio::buffer(msg->buf_, msg->MAX_MSG_LEN), 
		msg->ep_, boost::bind(&server::receive_terminal, this, 
		msg, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void server::receive_terminal(terminal_message *msg, const boost::system::error_code& e, size_t len) {
	if ((!e || e == boost::asio::error::message_size)) {		
		terminal::recv(msg, len);
	}
	start_receive_terminal();
}

terminal* server::get_terminal(std::string &id) {
	auto it = terminal_id_map_.find(id);
	if (it == terminal_id_map_.end())
		return nullptr;
	else
		return it->second;
}

terminal* server::get_terminal(boost::asio::ip::udp::endpoint &endpoint) {
	auto it = terminal_endpoint_map_.find(endpoint);
	if (it == terminal_endpoint_map_.end())
		return nullptr;
	else
		return it->second;
}

bool server::add_terminal(terminal *t) {
	
	terminal *old = get_terminal(t->id_);
	if (nullptr != old && t != old)
	{
		LOG(WARNING) << "delete zombie terminal " << old->id_ << " from " << old->endpoint_;
		delete old;
		old = nullptr;
		
	}
	if (nullptr == old) {
		terminal_id_map_.insert({t->id_, t});
		terminal_endpoint_map_.insert({t->endpoint_, t});
	}
	
	old = get_terminal(t->endpoint_);
	if (nullptr != old && t != old)
	{
		delete old;
		old = nullptr;
	}
	if (nullptr == old) {
		terminal_endpoint_map_.insert({t->endpoint_, t});
	}
	
	LOG(INFO) << "add terminal  " << t->id_ << " from " << t->endpoint_;

	return true;
}

void server::del_terminal(terminal *t) {
	LOG(INFO) << "delete terminal  " << t->id_ << " from " << t->endpoint_;
	terminal_id_map_.erase(t->id_);
	terminal_endpoint_map_.erase(t->endpoint_);
	client *c = get_client(t->id_);
	if (c) del_client(c);
}
