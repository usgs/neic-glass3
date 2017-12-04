/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include <ctime>
#include <string>

namespace util {
/**
 * \brief Convert time from epoch time to ISO8601
 *
 * Convert the given epoch time from decimal seconds to an
 * ISO8601 time string
 * \param epochtime - A double containing the epoch time
 * \return returns an ISO8601 formatted time std::string
 */
std::string convertEpochTimeToISO8601(double epochtime);

/**
 * \brief Convert time from epoch time to ISO8601
 *
 * Convert the given epoch time from decimal seconds to an
 * ISO8601 time string
 * \param now - A time_t containing the epoch time
 * \param decimalseconds - A an optional double containing the decimal seconds
 * \return returns an ISO8601 formatted time std::string
 */
std::string convertEpochTimeToISO8601(time_t now, double decimalseconds = 0);

/**
 * \brief Convert time from date time to ISO8601
 *
 * Convert the given date time string to an
 * ISO8601 time string
 * \param TimeString - A std::string containing the date time
 * \return returns an ISO8601 formatted time std::string
 */
std::string convertDateTimeToISO8601(const std::string &TimeString);

/**
 * \brief Convert time from date time to epoch time
 *
 * Convert the given datetime string to an epoch time
 * \param TimeString - A std::string containing the date time
 * \return returns a double variable containing the epochtime
 */
double convertDateTimeToEpochTime(const std::string &TimeString);

/**
 * \brief Convert time from ISO8601 time to epoch time
 *
 * Convert the given ISO8601 string to an epoch time
 * \param TimeString - A std::string containing the ISO8601 time
 * \return returns a double variable containing the epochtime
 */
double convertISO8601ToEpochTime(const std::string &TimeString);
}  // namespace util
#endif  // TIMEUTIL_H
