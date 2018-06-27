/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef LOGIT_H
#define LOGIT_H

#include <string>
#include <functional>

namespace glassutil {

// static char sLog[1024];

// logging levels
#define LOG_INFO 0
#define LOG_DEBUG 1
#define LOG_WARN 2
#define LOG_ERROR 3

/**
 * \brief glassutil logging levels enum
 *
 * The log_level enum defines the available logging levels.
 */
typedef enum {
	debug = 0,
	info = 1,
	warn = 2,
	error = 3
} log_level;

/**
 * \brief glassutil logging structure
 *
 * The logMessageStruct encapsulates both a log message, and the
 * desired logging level.
 */
struct logMessageStruct {
	/**
	 * \brief An log_level enum containing the level of the log message
	 */
	log_level level;

	/**
	 * \brief A std::string containing the log message
	 */
	std::string message;
};

/**
 * \brief glassutil logging class
 *
 * The CLogit class encapsulates the logic and functionality needed
 * to write logging information to disk.
 */
class CLogit {
 public:
	/**
	 * \brief CLogit constructor
	 *
	 * The constructor for the CLogit class.
	 */
	CLogit();

	/**
	 * \brief CLogit destructor
	 *
	 * The destructor for the CLogit class.
	 */
	~CLogit();

	/**
	 * \brief CLogit disable logging function
	 */
	static void disable();

	/**
	 * \brief CLogit enable logging function
	 */
	static void enable();

	/**
	 * \brief optional logging callback setup function
	 *
	 * \param callback - A std::function<void(std::string)> containing the
	 * callback function
	 */
	static void setLogCallback(std::function<void(logMessageStruct)> callback);

	/**
	 * \brief Write log message
	 *
	 * Write the given log message to disk
	 *
	 * \param s - A pointer to a char array containing the message
	 * to write.
	 */
	static void Out(const char *s);

	/**
	 * \brief logging function
	 *
	 * Function to log error and status messages.  If the optional logging
	 * callback is not set up, this function will use printf.
	 * This function logs all messages at the log_level::debug level.
	 * \param logMessage - A char * containing the logging message
	 */
	static void log(const char * logMessage);

	/**
	 * \brief logging function
	 *
	 * Function to log error and status messages.  If the optional logging
	 * callback is not set up, this function will use printf
	 * \param logLevel - A log_level enum containing the desired logging level.
	 * \param logMessage - A char * containing the logging message
	 */
	static void log(log_level logLevel, const char * logMessage);

	/**
	 * \brief logging function
	 *
	 * Function to log error and status messages.  If the optional logging
	 * callback is not set up, this function will use printf.
	 * This function logs all messages at the log_level::debug level.
	 * \param logMessage - A std::string containing the logging message
	 */
	static void log(std::string logMessage);

	/**
	 * \brief logging function
	 *
	 * Function to log error and status messages.  If the optional logging
	 * callback is not set up, this function will use printf
	 * \param logLevel - A log_level enum containing the desired logging level.
	 * \param logMessage - A std::string containing the logging message
	 */
	static void log(log_level logLevel, std::string logMessage);

#ifndef _WIN32
	/**
	 * \brief A std::function<void(logMessageStruct)> containing the optional
	 * logging callback.
	 */
	static std::function<void(logMessageStruct)> m_logCallback;
#endif  // _WIN32

	/**
	 * \brief A boolean flag to disable all logging
	 */
	static bool bDisable;
};
}  // namespace glassutil
#endif  // LOGIT_H
