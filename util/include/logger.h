/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief logger.h contains the interface to the neic-glass3 logging library
 *
 * logger.h contains the functions that neic-glass3 uses to log various debug,
 * informational, and error messages to console and disk file
 *
 * logger uses a slightly customized version of the spdlog logging library,
 * located in /lib/ spdlog is available at https://github.com/gabime/spdlog
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog.h>
#include <string>

namespace glass3 {
namespace util {

/**
 * \brief convert string level to spdlog level
 *
 * This function converts the provided string log level to
 *
 * Supported logging level strings are:
 * criticalerror - log critical error messages prior to program exit
 * error - log algorithmic error messages
 * warning - log warning messages
 * info - log informational messages
 * debug - log debugging messages, used to debug algorithms
 * trace - log tracing messages, used to trace code execution
 *
 * \param levelString - A std::string containing the logging level to convert
 * \return Returns a spdlog::level::level_enum containing logging level
 */
spdlog::level::level_enum string_to_log_level(const std::string &levelString);

/**
 * \brief initialize logging
 *
 * Initialize the logging system for the application, setting whether to log to
 * disk and/or console, and setting the desired logging level
 *
 * \see string_to_log_level
 *
 * \param programName - A std::string containing the name of the program
 * \param logLevel - A std::string representing the desired log level,
 * Supported levelString are: "info", "trace", "debug", "warning", "error",
 * and "critical_error"
 * \param logConsole - A boolean flag representing whether to log to console
 * \param logPath - A std::string containing the name of path for the log file,
 * empty string disables logging to file.
 */
void log_init(const std::string &programName,
				const std::string &logLevel,
				const std::string &logPath, bool logConsole = true);

/**
 * \brief update log level
 *
 * Update the current log level to the provided spdlog level enum
 *
 * \param loglevel - A spdlog::level::level_enum representing the desired log
 * level.
 */
void log_update_level(spdlog::level::level_enum loglevel);

/**
 * \brief update log level
 *
 * Update the current log level to the provided level string, primarily used
 * to update the logging system to a configured logging level after startup
 *
 * \see string_to_log_level
 *
 * \param levelString - A std::string representing the desired log level,
 * Supported levelString are: "info", "trace", "debug", "warning", "error",
 * and "critical_error"
 */
void log_update_level(const std::string &levelString);

/**
 * \brief log a message
 *
 * Log a message with the provided level
 *
 * \see string_to_log_level
 *
 * \param level - A std::string representing the desired log level.
 * Supported levels are: "info", "trace", "debug", "warning", "error",
 * and "critical_error"
 * \param message - A std::string representing the message to log.
 */
void log(const std::string &level, const std::string &message);

/**
 * \brief log a message at info level
 *
 * Log a message at the info log level
 *
 * \param message - A std::string representing the message to log.
 */
void logInfo(const std::string &message);

/**
 * \brief log a message at trace level
 *
 * Log a message at the trace log level
 *
 * \param message - A std::string representing the message to log.
 */
void logTrace(const std::string &message);

/**
 * \brief log a message at debug level
 *
 * Log a message at the debug log level
 *
 * \param message - A std::string representing the message to log.
 */
void logDebug(const std::string &message);

/**
 * \brief log a message at warning level
 *
 * Log a message at the warning log level
 *
 * \param message - A std::string representing the message to log.
 */
void logWarning(const std::string &message);

/**
 * \brief log a message at error level
 *
 * Log a message at the error log level
 *
 * \param message - A std::string representing the message to log.
 */
void logError(const std::string &message);

/**
 * \brief log a message at critical error level
 *
 * Log a message at the critical error log level
 *
 * \param message - A std::string representing the message to log.
 */
void logCriticalError(const std::string &message);
}  // namespace util
}  // namespace glass3
#endif  // LOGGER_H
