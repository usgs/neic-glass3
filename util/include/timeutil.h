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
 * Convert the given epoch time (seconds from 1970) from decimal seconds to an
 * ISO8601 time string in the format YYYY-MM-DDTHH:MM:SS.SSSZ, see
 * https://en.wikipedia.org/wiki/ISO_8601 for a definition of ISO8601
 *
 * \param epochtime - A double containing the epoch time (seconds from 1970)
 * \return returns a std::string containing the ISO8601 time string in the format
 * YYYY-MM-DDTHH:MM:SS.SSSZ
 */
std::string convertEpochTimeToISO8601(double epochtime);

/**
 * \brief Convert time from epoch time to ISO8601
 *
 * Convert the given epoch time (seconds from 1970) from decimal seconds to an
 * ISO8601 time string
 * \param epochtime - A time_t containing the epoch time (seconds from 1970)
 * \param decimalseconds - A an optional double containing the decimal seconds,
 * default is 0
 * \return returns a std::string containing the ISO8601 time string in the format
 * YYYY-MM-DDTHH:MM:SS.SSSZ
 */
std::string convertEpochTimeToISO8601(std::time_t epochtime,
										double decimalseconds = 0);

/**
 * \brief Convert time from date time format to ISO8601 format
 *
 * Convert the given DateTime time string in the format YYYYMMDDHHMMSS.SSS to an
 * ISO8601 time string in the format YYYY-MM-DDTHH:MM:SS.SSSZ
 *
 * \param TimeString - A std::string containing the date time in the format
 * YYYYMMDDHHMMSS.SS
 * \return returns a std::string containing the ISO8601 time string in the format
 * YYYY-MM-DDTHH:MM:SS.SSSZ
 */
std::string convertDateTimeToISO8601(const std::string &TimeString);

/**
 * \brief Convert time from date time to epoch time
 *
 * Convert the given DateTime time string in the format YYYYMMDDHHMMSS.SSS to an
 * epoch time (seconds from 1970)
 *
 * \param TimeString - A std::string containing the date time in the format
 * YYYYMMDDHHMMSS.SSS
 * \return returns a double variable containing the epoch time (seconds from
 * 1970)
 */
double convertDateTimeToEpochTime(const std::string &TimeString);

/**
 * \brief Convert time from ISO8601 time to epoch time
 *
 * Convert the given ISO8601 string in the format YYYY-MM-DDTHH:MM:SS.SSSZ to an
 * epoch time
 *
 * \param TimeString - A std::string containing the ISO8601 time in the format
 * YYYY-MM-DDTHH:MM:SS.SSSZ
 * \return returns a double variable containing the epoch time (seconds from
 * 1970)
 */
double convertISO8601ToEpochTime(const std::string &TimeString);
}  // namespace util
#endif  // TIMEUTIL_H
