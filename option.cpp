#include "option.h"

#include <fstream>

static const char CFG_FILE[] = "./config.ini";
static const char NAME_CLIENT_PORT[] = "ClientPort";
static const char NAME_TERMINAL_PORT[] = "TerminalPort";
static const char NAME_TERMINAL_TIMEOUT[] = "TerminalTimeout";

option::option() {
	glossary_.add_options()
		(NAME_CLIENT_PORT, bpo::value<unsigned short>(&client_port_)->default_value(DEFAULT_CLIENT_PORT), "")
		(NAME_TERMINAL_PORT, bpo::value<unsigned short>(&terminal_port_)->default_value(DEFAULT_TERMINAL_PORT), "")
		(NAME_TERMINAL_TIMEOUT, bpo::value<long>(&terminal_timeout_)->default_value(DEFAULT_TERMINAL_TIMEOUT), "");
}

option::~option() {
}

void option::load() {
	std::ifstream ifs(CFG_FILE);
	bpo::store(bpo::parse_config_file(ifs, glossary_), store_);
	bpo::notify(store_);
}

void option::save() {
	std::ofstream ofs(CFG_FILE);	
	ofs << NAME_CLIENT_PORT << "=" << client_port_ << std::endl;
	ofs << NAME_TERMINAL_PORT << "=" << terminal_port_ << std::endl;
	ofs << NAME_TERMINAL_TIMEOUT << "=" << terminal_timeout_ << std::endl;
}
