/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef DATE_H
#define DATE_H

#include <ctime>
#include <string>

namespace glass3 {
namespace util {

/**
 * \brief time translation class
 *
 * The date class is a class that stores
 * time/date information and supports converting time/
 * date information between various formats external (human)
 * and internal (epoch time, ISO8601, etc.) storing date
 * internally as a double containing julian seconds.
 *
 */
class Date {
 public:
	/**
	 * \brief CDate default constructor
	 *
	 * The default constructor for the CDate class.
	 */
	Date();

	/**
	 * \brief CDate advanced constructor
	 *
	 * An advanced constructor for the CDate class, initializes
	 * the class from a provided time in julian seconds.
	 *
	 * \param time - A double value containing the time in julian seconds
	 */
	explicit Date(double time);

	/**
	 * \brief CDate advanced constructor
	 *
	 * An advanced constructor for the CDate class, initializes
	 * the class from a provided gregorian date/time.
	 *
	 * \param year - An unsigned integer value containing the year
	 * \param month - An unsigned integer value containing the month
	 * \param day - An unsigned integer value containing the day
	 * \param hour - An unsigned integer value containing the hour
	 * \param minutes - An unsigned integer value containing the minutes
	 * \param seconds - A double value containing the seconds
	 */
	Date(unsigned int year, unsigned int month, unsigned int day,
			unsigned int hour, unsigned int minutes, double seconds);

	/**
	 * \brief CDate advanced constructor
	 *
	 * An advanced constructor for the CDate class, initializes
	 * the class from a provided time string.
	 *
	 * \param time - A string value containing the time
	 */
	explicit Date(std::string time);

	/**
	 * \brief CDate destructor
	 *
	 * The destructor for the CDate class.
	 */
	~Date();

	/**
	 * \brief CDate clear function
	 *
	 * CDate clear function
	 */
	void clear();

	/**
	 * \brief CDate current time function
	 *
	 * gets the current time in julian seconds
	 * \return Returns a double containing the julian seconds.
	 */
	static double now();

	/**
	 * \brief CDate initialize function
	 *
	 * Initialize function for the CDate class, initializes
	 * the class from a provided time in julian seconds.
	 *
	 * \param time - A double value containing the time in julian seconds
	 * \return returns true if successful, false otherwise
	 */
	bool initialize(double time);

	/**
	 * \brief CDate alternate initialize function
	 *
	 * Initialize function for the CDate class, initializes
	 * the class from a provided gregorian date/time.
	 *
	 * \param year - An unsigned integer value containing the year
	 * \param month - An unsigned integer value containing the month
	 * \param day - An unsigned integer value containing the day
	 * \param hour - An unsigned integer value containing the hour
	 * \param minutes - An unsigned integer value containing the minutes
	 * \param seconds - A double value containing the seconds
	 * \return returns true if successful, false otherwise
	 */
	bool initialize(unsigned int year, unsigned int month, unsigned int day,
					unsigned int hour, unsigned int minutes, double seconds);

	/**
	 * \brief CDate alternate initialize function
	 *
	 * Initialize function for the CDate class, initializes
	 * the class from a provided time  string.
	 *
	 * \param time - A string value containing the time
	 * \return returns true if successful, false otherwise
	 */
	bool initialize(std::string time);

	/**
	 * \brief Get the gregorian year
	 *
	 * Get the current gregorian year
	 * \return Returns an unsigned integer containing the gregorian year.
	 */
	unsigned int year();

	/**
	 * \brief Get the gregorian month
	 *
	 * Get the current gregorian month
	 * \return Returns an unsigned integer containing the gregorian month.
	 */
	unsigned int month();

	/**
	 * \brief Get the gregorian day
	 *
	 * Get the current gregorian day
	 * \return Returns an unsigned integer containing the gregorian day.
	 */
	unsigned int day();

	/**
	 * \brief Get the gregorian hour
	 *
	 * Get the current gregorian hour
	 * \return Returns an unsigned integer containing the gregorian hour.
	 */
	unsigned int hour();

	/**
	 * \brief Get the gregorian minute
	 *
	 * Get the current gregorian minute
	 * \return Returns an unsigned integer containing the gregorian minute.
	 */
	unsigned int minute();

	/**
	 * \brief Get the gregorian seconds
	 *
	 * Get the current gregorian seconds
	 * \return Returns a double containing the gregorian seconds.
	 */
	double seconds();

	/**
	 * \brief Get the julian seconds
	 *
	 * Get the total julian seconds
	 * \return Returns a double containing the julian seconds.
	 */
	double time();

	/**
	 * \brief Calculate the 20 character date string
	 *
	 * Calculate the 20 character date string in the form
	 * 1988Jan23 1234 12.21 from the julian seconds.
	 * \return Returns a std::string containing the date string
	 */
	std::string date18();

	/**
	 * \brief Calculate the 18 character date string
	 *
	 * Calculate the 18 character date string in the form
	 * 88Jan23 1234 12.21 from the julian seconds.
	 * \return Returns a std::string containing the date string
	 */
	std::string date20();

	/**
	 * \brief Calculate the ISO8601 date string
	 *
	 * Calculate the ISO8601 date string in the form
	 * 'YYYY-MM-DDTHH:MM:SS.SSSZ' from the julian seconds assuming
	 * base time is already UTC.
	 * \return Returns a std::string containing the date string
	 */
	std::string ISO8601();

	/**
	 * \brief Calculate the DateTime date string
	 *
	 * Calculate the date time string in the form
	 * 'yyyymmddhhmmss.sss' from the julian seconds assuming
	 * base time is already UTC.
	 * \return Returns a std::string containing the datetime string
	 */
	std::string dateTime();

	/**
	 * \brief Decode the datetime format date string into julian seconds
	 *
	 * Decode the datetime format date string
	 * 'yyyymmddhhmmss.sss' into julian seconds.
	 * \return Returns a double containing the julian seconds
	 */
	double decodeDateTime(std::string datetime);

	/**
	 * \brief Decode the ISO8601 date string into julian seconds
	 *
	 * Decode the ISO8601 format date string
	 * 'YYYY-MM-DDTHH:MM:SS.SSSZ' into julian seconds.
	 * \return Returns a double containing the julian seconds
	 */
	double decodeISO8601Time(std::string iso8601);

	/**
	 * \brief Calculate (encode) the date time date string
	 *
	 * Calculate the date time string in the form
	 * 'yyyymmddhhmmss.sss' from the julian seconds assuming
	 * base time is already UTC.
	 * \return Returns a std::string containing the date string
	 */
	static std::string encodeDateTime(double t);

	/**
	 * \brief Calculate (encode) the ISO8610 date string
	 *
	 * Calculate the ISO8601 date string in the form
	 * 'YYYY-MM-DDTHH:MM:SS.SSSZ' from the julian seconds assuming
	 * base time is already UTC.
	 * \return Returns a std::string containing the date string
	 */
	static std::string encodeISO8601Time(double t);

	/**
	 * \brief Convert time from epoch time to ISO8601
	 *
	 * Convert the given epoch time (seconds from 1970) from decimal seconds to an
	 * ISO8601 time string in the format YYYY-MM-DDTHH:MM:SS.SSSZ
	 *
	 * For more information on epoch time, see
	 * https://en.wikipedia.org/wiki/Unix_time
	 *
	 * For more information on ISO8601, see
	 * https://en.wikipedia.org/wiki/ISO_8601
	 *
	 * \param epochTime - A double containing the epoch time (seconds from 1970)
	 * \return returns a std::string containing the ISO8601 time string in the format
	 * YYYY-MM-DDTHH:MM:SS.SSSZ
	 */
	static std::string convertEpochTimeToISO8601(double epochTime);

	/**
	 * \brief Convert time from epoch time to ISO8601
	 *
	 * Convert the given epoch time (seconds from 1970) from decimal seconds to an
	 * ISO8601 time string
	 *
	 * For more information on epoch time, see
	 * https://en.wikipedia.org/wiki/Unix_time
	 *
	 * For more information on ISO8601, see
	 * https://en.wikipedia.org/wiki/ISO_8601
	 *
	 * \param epochTime - A time_t containing the epoch time (seconds from 1970)
	 * \param decimalSeconds - A an optional double containing the decimal seconds,
	 * default is 0
	 * \return returns a std::string containing the ISO8601 time string in the format
	 * YYYY-MM-DDTHH:MM:SS.SSSZ
	 */
	static std::string convertEpochTimeToISO8601(std::time_t epochTime,
											double decimalSeconds = 0);

	/**
	 * \brief Convert time from date time format to ISO8601 format
	 *
	 * Convert the given DateTime time string in the format YYYYMMDDHHMMSS.SSS to an
	 * ISO8601 time string in the format YYYY-MM-DDTHH:MM:SS.SSSZ
	  *
	 * For more information on ISO8601, see
	 * https://en.wikipedia.org/wiki/ISO_8601
	 *
	 * \param timeString - A std::string containing the date time in the format
	 * YYYYMMDDHHMMSS.SS
	 * \return returns a std::string containing the ISO8601 time string in the format
	 * YYYY-MM-DDTHH:MM:SS.SSSZ
	 */
	static std::string convertDateTimeToISO8601(const std::string &timeString);

	/**
	 * \brief Convert time from date time to epoch time
	 *
	 * Convert the given DateTime time string in the format YYYYMMDDHHMMSS.SSS to an
	 * epoch time (seconds from 1970)
	 *
	 * For more information on epoch time, see
	 * https://en.wikipedia.org/wiki/Unix_time
	 *
	 * \param timeString - A std::string containing the date time in the format
	 * YYYYMMDDHHMMSS.SSS
	 * \return returns a double variable containing the epoch time (seconds from
	 * 1970)
	 */
	static double convertDateTimeToEpochTime(const std::string &timeString);

	/**
	 * \brief Convert time from ISO8601 time to epoch time
	 *
	 * Convert the given ISO8601 string in the format YYYY-MM-DDTHH:MM:SS.SSSZ to an
	 * epoch time
	 *
	 * For more information on epoch time, see
	 * https://en.wikipedia.org/wiki/Unix_time
	 *
	 * For more information on ISO8601, see
	 * https://en.wikipedia.org/wiki/ISO_8601
	 *
	 * \param timeString - A std::string containing the ISO8601 time in the format
	 * YYYY-MM-DDTHH:MM:SS.SSSZ
	 * \return returns a double variable containing the epoch time (seconds from
	 * 1970)
	 */
	static double convertISO8601ToEpochTime(const std::string &timeString);

 protected:
	/**
	 * \brief An unsigned integer variable containing the gregorian year.
	 */
	unsigned int m_nYear;

	/**
	 * \brief An unsigned integer variable containing the gregorian month.
	 */
	unsigned int m_nMonth;

	/**
	 * \brief An unsigned integer variable containing the gregorian day.
	 */
	unsigned int m_nDay;

	/**
	 * \brief An unsigned integer variable containing the gregorian hour.
	 */
	unsigned int m_nHour;

	/**
	 * \brief An unsigned integer variable containing the gregorian minute.
	 */
	unsigned int m_nMinute;

	/**
	 * \brief A double variable containing the gregorian seconds.
	 */
	double m_dSeconds;

	/**
	 * \brief A double variable containing the julian seconds.
	 */
	double m_dTime;
};
}  // namespace util
}  // namespace glass3
#endif  // DATE_H
