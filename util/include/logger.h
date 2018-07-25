/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog.h>
#include <string>

/**
 * \namespace logger
 * \brief glass namespace containing error logging functions
 *
 * The namespace containing a collection of error logging functions that
 * utilize spdlog.
 */
namespace glass3 {
namespace util {

/**
 * \brief initialize logging
 *
 * Initialize the logging system
 *
 * \param programname - A std::string containing the name of the program
 * \param loglevel - A spdlog::level::level_enum representing the desired log
 * level.
 * \param logConsole - A boolean flag representing whether to log to console
 * \param logpath - A std::string containing the name of path for the log file,
 * empty string disables logging to file.
 */
void log_init(const std::string &programname,
				spdlog::level::level_enum loglevel,
				const std::string &logpath, bool logConsole = true);

/**
 * \brief update log level
 *
 * Set the current log level to the provided level
 *
 * \param loglevel - A spdlog::level::level_enum representing the desired log
 * level.
 */
void log_update_level(spdlog::level::level_enum loglevel);

/**
 * \brief update log level
 *
 * Set the current log level to the provided level
 *
 * \param logstring - A std::string representing the desired log level,
 * Supported logstrings are: "info", "trace", "debug", "warning", "error",
 * and "critical_error"
 */
void log_update_level(const std::string &logstring);

/**
 * \brief log a message
 *
 * Log a message with the provided level
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
