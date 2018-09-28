#include <logger.h>
#include <string>
#include <iostream>
#include <memory>
#include <vector>

namespace glass3 {
namespace util {

bool Logger::m_bDisable = false;
// ---------------------------------------------------------CLogit
Logger::Logger() {
}

// ---------------------------------------------------------~CLogit
Logger::~Logger() {
}

// ---------------------------------------------------------disable
void Logger::disable() {
	m_bDisable = true;
}

// ---------------------------------------------------------enable
void Logger::enable() {
	m_bDisable = false;
}

// ----------------------------------------------------------string_to_log_level
spdlog::level::level_enum Logger::string_to_log_level(
		const std::string &levelString) {
	// update current log level
	if (levelString == "debug") {
		return (spdlog::level::debug);
	} else if (levelString == "trace") {
		return (spdlog::level::trace);
	} else if (levelString == "info") {
		return (spdlog::level::info);
	} else if (levelString == "warning") {
		return (spdlog::level::warn);
	} else if (levelString == "error") {
		return (spdlog::level::err);
	} else if (levelString == "criticalerror") {
		return (spdlog::level::critical);
	} else if (levelString == "critical") {
		return (spdlog::level::critical);
	} else {
		return (spdlog::level::info);
	}
}

// -------------------------------------------------------------log_init
void Logger::log_init(const std::string &programName,
						const std::string &logLevel, const std::string &logPath,
						bool logConsole) {
	// inits logging
	try {
		// create sink vector
		std::vector<spdlog::sink_ptr> sinks;

		// only log to console
		if (logConsole) {
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
		}

		// only log to disk if asked. Log rolls over at 0 hours and 0 minutes.
		if (logPath != "") {
			std::string logfile = logPath + "/" + programName;
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
		spdlog::set_level(string_to_log_level(logLevel));

		std::string startupmessage = "***** " + programName
				+ ": Logger startup; ";

		if (logConsole) {
			startupmessage += "logging enabled to console; ";
		}

		if (logPath != "") {
			startupmessage += "logging enabled to disk; ";
		}

		startupmessage += "*****";

		// say hello
		logger->info("************************************************");
		logger->info(startupmessage);
		logger->info("************************************************");
	} catch (spdlog::spdlog_ex& ex) {
		std::cout << programName << ": logging initialization failed: "
					<< ex.what() << std::endl;
	}
}

// -------------------------------------------------------------log_update_level
void Logger::log_update_level(spdlog::level::level_enum loglevel) {
	// update current log level
	try {
		// set logging level
		spdlog::set_level(loglevel);
		auto logger = spdlog::get("logger");
		logger->set_level(loglevel);

		std::string levelString = spdlog::level::to_str(loglevel);
		log("info", "logging set to level: " + levelString);
	} catch (spdlog::spdlog_ex& ex) {
		std::cout << "Exception setting logging level: " << ex.what()
					<< std::endl;
	}
}

// -------------------------------------------------------------log_update_level
void Logger::log_update_level(const std::string &levelString) {
	// update current log level
	log_update_level(string_to_log_level(levelString));
}

// ---------------------------------------------------------log
void Logger::log(const char * logMessage) {
	log("debug", std::string(logMessage));
}

// ---------------------------------------------------------log
void Logger::log(const std::string &level, const char * logMessage) {
	log(level, std::string(logMessage));
}

// ---------------------------------------------------------log
void Logger::log(std::string logMessage) {
	log("debug", logMessage);
}

// -------------------------------------------------------------log
void Logger::log(const std::string &level, const std::string &message) {
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
	} else if (level == "critical") {
		logCriticalError(message);
	}
}

// -------------------------------------------------------------logInfo
void Logger::logInfo(const std::string &message) {
	// log an info message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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

// -------------------------------------------------------------logTrace
void Logger::logTrace(const std::string &message) {
	// log a debug message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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

// -------------------------------------------------------------logDebug
void Logger::logDebug(const std::string &message) {
	// log a debug message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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

// -------------------------------------------------------------logWarning
void Logger::logWarning(const std::string &message) {
	// log a warning message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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

// -------------------------------------------------------------logError
void Logger::logError(const std::string &message) {
	// log an error message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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

// -------------------------------------------------------------logCriticalError
void Logger::logCriticalError(const std::string &message) {
	// log a critical error message
	if (message == "") {
		return;
	}

	// don't bother if logging is disabled
	if (m_bDisable == true) {
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
}  // namespace util
}  // namespace glass3
