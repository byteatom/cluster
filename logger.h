#ifndef __PRINT_SINK__
#define __PRINT_SINK__

#include <string>
#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>

#include <g3log/logmessage.hpp>
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>

struct CoutSink {
	
	void ReceiveLogMessage(g3::LogMessageMover logEntry) {		
		std::cout << logEntry.get().toString() << std::endl;
	}
};

class Logger {
	std::unique_ptr<g3::LogWorker> worker;
public:
	Logger(const std::string& prefix, const std::string& directory):
		worker{ g3::LogWorker::createLogWorker() }
	{
		if (!boost::filesystem::exists(directory)) {
			boost::filesystem::create_directories(directory);
		}

		worker->addSink(std::make_unique<CoutSink>(),
			&CoutSink::ReceiveLogMessage);
		worker->addDefaultLogger(prefix, directory);

		g3::initializeLogging(worker.get());
	};
};

#endif

