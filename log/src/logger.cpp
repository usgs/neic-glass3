#include <logger.h>
#include <string>
#include <iostream>
#include <memory>
#include <vector>

namespace logger {

void log_init(const std::string &programname,
				spdlog::level::level_enum loglevel,
				const std::string &logpath, bool logConsole) {
	// inits logging
	try {
		// create sink vector
		std::vector<spdlog::sink_ptr> sinks;

		// only log to console
		if (logConsole) {
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
		}

		// only log to disk if asked. Log rolls over at 0 hours and 0 minutes.
		if (logpath != "") {
			std::string logfile = logpath + "/" + programname;
			sinks.push_back(
					std::make_shared<spdlog::sinks::daily_file_sink_mt>(logfile,
																		"log",
																		0, 0));
		}
		// create the combined logger
		auto logger = std::make_shared<spdlog::logger>("logger", begin(sinks),
														end(sinks));

		// register the logger
		spdlog::register_logger(logger);

		// set logger format
		spdlog::set_pattern("%Y%m%d_%H:%M:%S.%e <%t> [%l] %v");

		// set logging level
		spdlog::set_level(loglevel);

		std::string startupmessage = "***** " + programname
				+ ": Logger startup; ";

		if (logConsole) {
			startupmessage += "logging enabled to console; ";
		}

		if (logpath != "") {
			startupmessage += "logging enabled to disk; ";
		}

		startupmessage += "*****";

		// say hello
		logger->info("************************************************");
		logger->info(startupmessage);
		logger->info("************************************************");
	} catch (spdlog::spdlog_ex& ex) {
		std::cout << programname << ": logging initialization failed: "
					<< ex.what() << std::endl;
	}
}

void log_update_level(spdlog::level::level_enum loglevel) {
	// update current log level
	try {
		// set logging level
		spdlog::set_level(loglevel);

		std::string logstring = spdlog::level::to_str(loglevel);
		log("info", "logging set to level: " + logstring);
	} catch (spdlog::spdlog_ex& ex) {
		std::cout << "Exception setting logging level: " << ex.what()
					<< std::endl;
	}
}

void log_update_level(const std::string &logstring) {
	// update current log level
	if (logstring == "debug") {
		log_update_level(spdlog::level::debug);
	} else if (logstring == "trace") {
		log_update_level(spdlog::level::trace);
	} else if (logstring == "info") {
		log_update_level(spdlog::level::info);
	} else if (logstring == "warning") {
		log_update_level(spdlog::level::warn);
	} else if (logstring == "error") {
		log_update_level(spdlog::level::err);
	} else if (logstring == "criticalerror") {
		log_update_level(spdlog::level::critical);
	} else {
		log_update_level(spdlog::level::info);
	}
}

/* Available Logging levels, we're only interested in 5 of them
 * details::line_logger trace();
 * details::line_logger debug();
 * details::line_logger info();
 * details::line_logger notice();
 * details::line_logger warn();
 * details::line_logger error();
 * details::line_logger critical();
 * details::line_logger alert();
 * details::line_logger emerg();
 */

void log(const std::string &level, const std::string &message) {
	// log a message
	if (level == "info") {
		logInfo(message);
	} else if (level == "trace") {
		logTrace(message);
	} else if (level == "debug") {
		logDebug(message);
	} else if (level == "warning") {
		logWarning(message);
	} else if (level == "error") {
		logError(message);
	} else if (level == "criticalerror") {
		logCriticalError(message);
	}
}

void logInfo(const std::string &message) {
	// log an info message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->info(message);
		}
	} catch (spdlog::spdlog_ex&) {
	}
}

void logTrace(const std::string &message) {
	// log a debug message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->trace(message);
		}
	} catch (spdlog::spdlog_ex&) {
	}
}

void logDebug(const std::string &message) {
	// log a debug message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->debug(message);
		}
	} catch (spdlog::spdlog_ex&) {
	}
}

void logWarning(const std::string &message) {
	// log a warning message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->warn(message);
		}
	} catch (spdlog::spdlog_ex&) {
	}
}

void logError(const std::string &message) {
	// log an error message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->error(message);
			logger->flush();
		}
	} catch (spdlog::spdlog_ex&) {
	}
}

void logCriticalError(const std::string &message) {
	// log a critical error message
	if (message == "") {
		return;
	}

	try {
		auto logger = spdlog::get("logger");

		if (logger != nullptr) {
			logger->critical(message);
			logger->flush();
		}
	} catch (spdlog::spdlog_ex&) {
	}
}
}  // namespace logger
