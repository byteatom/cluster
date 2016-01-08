#include <string>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

class option {
public:	
	bpo::options_description glossary_;
	bpo::variables_map store_;

	static const unsigned short DEFAULT_CLIENT_PORT = 5050;
	static const unsigned short DEFAULT_TERMINAL_PORT = 5050;
	static const long DEFAULT_TERMINAL_TIMEOUT = 120;
	
	unsigned short client_port_ = DEFAULT_CLIENT_PORT;
	unsigned short terminal_port_ = DEFAULT_TERMINAL_PORT;
	long terminal_timeout_ = DEFAULT_TERMINAL_TIMEOUT;
	
	option();
	~option();

	void load();
	void save();	
};

