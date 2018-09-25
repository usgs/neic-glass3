#include <date.h>
#include <logger.h>

#include <stdio.h>
#include <sys/timeb.h>
#include <cstdio>
#include <ctime>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <wtypes.h>
#endif



namespace glass3 {
namespace util {

// maximum size of the environment variable TZ
#define MAXENV 128

/**
 * \brief A character array used to store the environment variable
 * TZ.  It will be stored after the first call to ConvertISO8601ToEpochTime()
 */
char envTZ[MAXENV];

static int mo[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 0,
		31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
static char const *cmo[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" };

static int base = 1900;	 // Base year

// ---------------------------------------------------------Date
Date::Date() {
	clear();
}

// ---------------------------------------------------------Date
Date::Date(double time) {
	clear();

	initialize(time);
}

// ---------------------------------------------------------Date
Date::Date(unsigned int year, unsigned int month, unsigned int day,
				unsigned int hour, unsigned int minute, double second) {
	clear();

	initialize(year, month, day, hour, minute, second);
}

// ---------------------------------------------------------Date
Date::Date(std::string time) {
	clear();

	initialize(time);
}

// ---------------------------------------------------------~Date
Date::~Date() {
}

// ---------------------------------------------------------clear
void Date::clear() {
	m_nYear = 0;
	m_nMonth = 0;
	m_nDay = 0;
	m_nHour = 0;
	m_nMinute = 0;
	m_dSeconds = 0;
	m_dTime = 0;
}

// ---------------------------------------------------------now
double Date::now() {
	// get the epoch time
	// what time is it
	time_t epochTime = ::time(NULL);

	// get the time struct
#ifdef _WIN32
	struct tm timestruct;
	gmtime_s(&timestruct, &epochTime);

#else
	struct tm timestruct;
	gmtime_r(&epochTime, &timestruct);
#endif

	// convert using Date
	Date dt = Date(timestruct.tm_year + 1900, timestruct.tm_mon + 1,
						timestruct.tm_mday, timestruct.tm_hour,
						timestruct.tm_min, timestruct.tm_sec);

	return (dt.time());
}

// ---------------------------------------------------------initialize
bool Date::initialize(double time) {
	int leap;
	int yr;
	int days;

	// Elapsed days
	int64_t jul = static_cast<int64_t>(time / 86400.0);

	// compute seconds
	double secs = time - jul * 86400.0;

	for (yr = base; yr < 2100; yr++) {
		days = 365;

		// handle leap year
		if (yr % 4 == 0) {
			days = 366;
		}
		if (yr % 100 == 0) {
			days = 365;
		}
		if (yr % 400 == 0) {
			days = 366;
		}
		if (days > jul) {
			break;
		}
		jul -= days;
	}

	// handle leap year
	leap = 0;
	if (yr % 4 == 0) {
		leap = 12;
	}
	if (yr % 100 == 0) {
		leap = 0;
	}
	if (yr % 400 == 0) {
		leap = 12;
	}

	// compute month
	int mon = 0;
	for (int i = 0; i < 12; i++) {
		if (mo[mon + leap] <= jul) {
			mon++;
		}
	}

	// compute day
	int day = static_cast<int>((jul - mo[mon + leap - 1] + 1));

	// compute hour
	int hr = static_cast<int>((secs / 3600.0));

	// adjust seconds for hour
	secs -= hr * 3600.0;

	// compute minute
	int mn = static_cast<int>((secs / 60.0));

	// adjust seconds for minute
	secs -= mn * 60.0;

	// store values
	m_nYear = (unsigned int) yr;
	m_nMonth = mon;
	m_nDay = day;
	m_nHour = hr;
	m_nMinute = mn;
	m_dSeconds = secs;
	m_dTime = time;

	// success
	return (true);
}

// ---------------------------------------------------------initialize
bool Date::initialize(unsigned int year, unsigned int month, unsigned int day,
						unsigned int hour, unsigned int minute, double second) {
	int64_t jul = 0;
	int leap;

	// Calculate days from base year
	for (unsigned int yr = base; yr < year; yr++) {
		jul += 365;

		// handle leap year
		leap = 0;
		if (yr % 4 == 0) {
			leap = 1;
		}
		if (yr % 100 == 0) {
			leap = 0;
		}
		if (yr % 400 == 0) {
			leap = 1;
		}
		jul += leap;
	}

	// handle leap year
	leap = 0;
	if (year % 4 == 0) {
		leap = 12;
	}
	if (year % 100 == 0) {
		leap = 0;
	}
	if (year % 400 == 0) {
		leap = 12;
	}

	// calculate julian days
	jul += mo[month + leap - 1] + day - 1;

	// calculate julian minutes
	int64_t jmin = 1440L * jul + 60L * hour + minute;

	// calculate julian seconds
	m_dTime = 60.0 * jmin + second;

	// store values
	m_nYear = year;
	m_nMonth = month;
	m_nDay = day;
	m_nHour = hour;
	m_nMinute = minute;
	m_dSeconds = second;

	// success
	return (true);
}

// ---------------------------------------------------------initialize
bool Date::initialize(std::string time) {
	// choose decode based on length of time string
	if (time.length() == 24) {
		// iso8601
		Date(decodeISO8601Time(time));

	} else if ((time.length() > 16) && (time.length() < 19)) {
		// date time
		Date(decodeDateTime(time));
	} else {
		// invalid format
		return (false);
	}

	return (true);
}

// ---------------------------------------------------------year
unsigned int Date::year() {
	return (m_nYear);
}

// ---------------------------------------------------------month
unsigned int Date::month() {
	return (m_nMonth);
}

// ---------------------------------------------------------day
unsigned int Date::day() {
	return (m_nDay);
}

// ---------------------------------------------------------hour
unsigned int Date::hour() {
	return (m_nHour);
}

// ---------------------------------------------------------minute
unsigned int Date::minute() {
	return (m_nMinute);
}

// ---------------------------------------------------------seconds
double Date::seconds() {
	return (m_dSeconds);
}

// ---------------------------------------------------------time
double Date::time() {
	return (m_dTime);
}

// ---------------------------------------------------------date18
std::string Date::date18() {
	char c18[80];

	// calculate hour+minutes value
	int hrmn = 100 * m_nHour + m_nMinute;

	// handle y2k
	int y2k;
	if (m_nYear < 2000) {
		y2k = m_nYear - 1900;
	} else {
		y2k = m_nYear - 2000;
	}

	// Generate 18 char date in the form 88Jan23 1234 12.21
	// from the julian seconds.  Remember to leave space for the
	// string termination (NUL).
	snprintf(c18, sizeof(c18), "%02d%3s%02d %04d%6.2f", y2k, cmo[m_nMonth - 1],
				m_nDay, hrmn, m_dSeconds);
	return (std::string(c18));
}

// ---------------------------------------------------------date20
std::string Date::date20() {
	char c20[80];

	// calculate hour+minutes value
	int hrmn = 100 * m_nHour + m_nMinute;

	// Generate 20 char date in the form 1988Jan23 1234 12.21
	// from the julian seconds.  Remember to leave space for the
	// string termination (NUL).
	snprintf(c20, sizeof(c20), "%04d%3s%02d %04d %6.2f", m_nYear,
				cmo[m_nMonth - 1], m_nDay, hrmn, m_dSeconds);
	return (std::string(c20));
}

// ---------------------------------------------------------ISO8601
std::string Date::ISO8601() {
	char s[320];

	// ISO8601 format:
	// 000000000011111111112222
	// 012345678901234567890123
	// YYYY-MM-DDTHH:mm:SS.SSSZ
	snprintf(s, sizeof(s), "%4d-%02d-%02dT%02d:%02d:%06.3fZ", m_nYear, m_nMonth,
				m_nDay, m_nHour, m_nMinute, m_dSeconds);

	return (std::string(s));
}

// ---------------------------------------------------------dateTime
std::string Date::dateTime() {
	char s[320];

	// DT format:
	// 000000000011111111
	// 012345678901234567
	// YYYYMMDDHHmmSS.SSS
	snprintf(s, sizeof(s), "%4d%02d%02d%02d%02d%06.3f", m_nYear, m_nMonth,
				m_nDay, m_nHour, m_nMinute, m_dSeconds);

	return (std::string(s));
}

// ---------------------------------------------------------decodeTime
double Date::decodeDateTime(std::string datetime) {
	// check string lenght
	if ((datetime.length() < 17) || (datetime.length() > 18)) {
		return (0);
	}

	// Time string is in DT format:
	// 000000000011111111
	// 012345678901234567
	// YYYYMMDDHHmmSS.SSS
	// where YYYY is the year,
	// MM is the month (Jan = 1), DD is the day of the month,
	// HH is the hour of the day (0-23), mm is the minutes of
	// the hour, and SS.SS is the seconds.
	// note that seconds can also be SS.SSS

	// check string length to determine seconds
	// decimal precision
	int secLength = 6;
	if (datetime.length() == 17) {
		secLength = 5;
	}

	// parse time components from string
	int iyr = std::stoi(datetime.substr(0, 4));
	int imo = std::stoi(datetime.substr(4, 2));
	int ida = std::stoi(datetime.substr(6, 2));
	int ihr = std::stoi(datetime.substr(8, 2));
	int imn = std::stoi(datetime.substr(10, 2));
	double sec = std::stod(datetime.substr(12, secLength));

	// convert
	Date dt = Date(iyr, imo, ida, ihr, imn, sec);
	m_dTime = dt.time();
	m_nYear = dt.year();
	m_nMonth = dt.month();
	m_nDay = dt.day();
	m_nHour = dt.hour();
	m_nMinute = dt.minute();
	m_dSeconds = dt.seconds();
	return (m_dTime);
}

// ---------------------------------------------------------decodeISO8601Time
double Date::decodeISO8601Time(std::string iso8601) {
	// check string length
	if (iso8601.length() != 24) {
		return (0);
	}

	// Time string is in ISO8601 format:
	// 000000000011111111112222
	// 012345678901234567890123
	// YYYY-MM-DDTHH:mm:SS.SSSZ
	// where YYYY is the year,
	// MM is the month (Jan = 1), DD is the day of the month,
	// HH is the hour of the day (0-23), mm is the minutes of
	// the hour, and SS.SSS is the seconds.

	// parse time components from string
	int iyr = std::stoi(iso8601.substr(0, 4));
	int imo = std::stoi(iso8601.substr(5, 2));
	int ida = std::stoi(iso8601.substr(8, 2));
	int ihr = std::stoi(iso8601.substr(11, 2));
	int imn = std::stoi(iso8601.substr(14, 2));
	double sec = std::stod(iso8601.substr(17, 6));

	// convert
	Date dt = Date(iyr, imo, ida, ihr, imn, sec);
	m_dTime = dt.time();
	m_nYear = dt.year();
	m_nMonth = dt.month();
	m_nDay = dt.day();
	m_nHour = dt.hour();
	m_nMinute = dt.minute();
	m_dSeconds = dt.seconds();
	return (m_dTime);
}

// ---------------------------------------------------------encodeDateTime
std::string Date::encodeDateTime(double t) {
	Date dt(t);
	return (dt.dateTime());
}

// ---------------------------------------------------------encodeISO8601Time
std::string Date::encodeISO8601Time(double t) {
	Date dt(t);
	return (dt.ISO8601());
}

// ----------------------------------------------------convertEpochTimeToISO8601
std::string Date::convertEpochTimeToISO8601(double epochTime) {
	double integerpart;
	double fractionpart;

	// split the double
	fractionpart = modf(epochTime, &integerpart);

	// modf returns only the integer part (3.14159265 -> 3.000000)
	// so a simple cast is sufficient
	std::time_t time = static_cast<int>(
			integerpart >= 0.0 ? (integerpart + 0.1) : (integerpart - 0.1));

	return (convertEpochTimeToISO8601(time, fractionpart));
}

// ----------------------------------------------------convertEpochTimeToISO8601
std::string Date::convertEpochTimeToISO8601(std::time_t epochTime,
										double decimalSeconds) {
	// build the time portion, all but the seconds which are
	// separate since time_t can't do decimal seconds
	char timebuf[sizeof "2011-10-08T07:07:"];

#ifdef _WIN32
	struct tm timestruct;
	gmtime_s(&timestruct, &epochTime);

#else
	struct tm timestruct;
	gmtime_r(&epochTime, &timestruct);
#endif

	strftime(timebuf, sizeof timebuf, "%Y-%m-%dT%H:%M:", &timestruct);
	std::string timestring = timebuf;

	// build the seconds portion
	char secbuf[sizeof "00.000Z"];
	if ((timestruct.tm_sec + decimalSeconds) < 10) {
		snprintf(secbuf, sizeof(secbuf), "0%1.3f",
					timestruct.tm_sec + decimalSeconds);
	} else {
		snprintf(secbuf, sizeof(secbuf), "%2.3f",
					timestruct.tm_sec + decimalSeconds);
	}
	std::string secondsstring = secbuf;

	// return the combined ISO8601 string
	return (timestring + secondsstring + "Z");
}

// ----------------------------------------------------convertDateTimeToISO8601
std::string Date::convertDateTimeToISO8601(const std::string &timeString) {
	double epoch_time = convertDateTimeToEpochTime(timeString);

	return (convertEpochTimeToISO8601(epoch_time));
}

// ----------------------------------------------------convertISO8601ToEpochTime
double Date::convertISO8601ToEpochTime(const std::string &timeString) {
	// make sure we got something
	if (timeString.length() == 0) {
		glass3::util::Logger::log("error",
					"ConvertISO8601ToEpochTime: Time string is empty.");
		return (-1.0);
	}

	// time string is too short
	if (timeString.length() < 24) {
		glass3::util::Logger::log("error",
					"ConvertISO8601ToEpochTime: Time string is too short.");
		return (-1.0);
	}

	try {
		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(struct tm));

		// Time string is in ISO8601 format:
		// 000000000011111111112222
		// 012345678901234567890123
		// YYYY-MM-DDTHH:MM:SS.SSSZ

		// year (0-3 in ISO8601 string)
		// struct tm stores year as "current year minus 1900"
		timeinfo.tm_year = std::stoi(timeString.substr(0, 4)) - 1900;

		// month (5-6 in ISO8601 string)
		// struct tm stores month  as number from 0 to 11, January = 0
		timeinfo.tm_mon = std::stoi(timeString.substr(5, 2)) - 1;

		// day (8-9 in ISO8601 string)
		timeinfo.tm_mday = std::stoi(timeString.substr(8, 2));

		// hour (11-12 in ISO8601 string)
		timeinfo.tm_hour = std::stoi(timeString.substr(11, 2));

		// minute (14-15 in ISO8601 string)
		timeinfo.tm_min = std::stoi(timeString.substr(14, 2));

		// seconds (17-22 in ISO8601 string)
		double seconds = std::stod(timeString.substr(17, 6));

#ifdef _WIN32
		// windows specific timezone handling
		char * pTZ;
		char szTZExisting[32] = "TZ=";

		// Save current TZ settings locally
		int timezoneExisting = _timezone;
		int daylightExisting = _daylight;
		pTZ = getenv("TZ");

		if (pTZ) {
			/* remember we have "TZ=" stored as the first 3 chars of the array */
			strncpy(&szTZExisting[3], pTZ, sizeof(szTZExisting) - 4);
			szTZExisting[sizeof(szTZExisting) - 1] = 0x00;
		}

		// Change time zone to GMT
		_putenv("TZ=UTC");

		// set the timezone-offset to 0
		_timezone = 0;

		// set the DST-offset to 0
		_daylight = 0;

		// change the timezone settings
		_tzset();

		// convert to epoch time
		double usabletime = static_cast<double>(mktime(&timeinfo));

		// Restore original TZ settings
		_putenv(szTZExisting);
		_timezone = timezoneExisting;
		_daylight = daylightExisting;
		_tzset();

#else
		// linux and other timezone handling
		char *tz;
		char TZorig[MAXENV];

		// Save current TZ setting locally
		tz = getenv("TZ");
		if (tz != reinterpret_cast<char *>(NULL)) {
			if (strlen(tz) > MAXENV - 4) {
				printf("ConvertISO8601ToEpochTime: unable to store current TZ "
						"environment variable.\n");
				return (-1);
			}
		}
		snprintf(TZorig, sizeof(TZorig), "TZ=%s", tz);

		// Change time zone to GMT
		snprintf(envTZ, sizeof(envTZ), "TZ=GMT");
		if (putenv(envTZ) != 0) {
			printf("ConvertISO8601ToEpochTime: putenv: unable to set TZ "
					"environment variable.\n");
			return (-1);
		}

		// convert to epoch time
		double usabletime = static_cast<double>(mktime(&timeinfo));

		// Restore original TZ setting
		snprintf(envTZ, sizeof(envTZ), "%s", TZorig);
		if (putenv(envTZ) != 0) {
			printf("ConvertISO8601ToEpochTime: putenv: unable to restore TZ "
					"environment variable.\n");
		}
		tzset();
#endif

		// add decimal seconds and return
		return (usabletime + seconds);
	} catch (const std::exception &) {
		glass3::util::Logger::log(
				"warning",
				"ConvertISO8601ToEpochTime: Problem converting time string: "
						+ timeString);
	}

	return (-1.0);
}

// --------------------------------------------------convertDateTimeToEpochTime
double Date::convertDateTimeToEpochTime(const std::string &timeString) {
	// make sure we got something
	if (timeString.length() == 0) {
		glass3::util::Logger::log(
				"error", "ConvertDTStringToEpochTime: Time string is empty.");
		return (-1.0);
	}
	// time string is too short
	if (timeString.length() < 18) {
		glass3::util::Logger::log(
				"error",
				"ConvertDTStringToEpochTime: Time string: " + timeString
						+ " is too short.");
		return (-1.0);
	}

	try {
		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(struct tm));

		// Time string is in DT format:
		// 000000000011111111
		// 012345678901234567
		// YYYYMMDDHHMMSS.SSS

		// year (0-3 in DTS string)
		// struct tm stores year as "current year minus 1900"
		timeinfo.tm_year = std::stoi(timeString.substr(0, 4)) - 1900;

		// month (4-5 in DTS string)
		// struct tm stores month  as number from 0 to 11, January = 0
		timeinfo.tm_mon = std::stoi(timeString.substr(4, 2)) - 1;

		// day (6-7 in DTS string)
		timeinfo.tm_mday = std::stoi(timeString.substr(6, 2));

		// hour (8-9 in DTS string)
		timeinfo.tm_hour = std::stoi(timeString.substr(8, 2));

		// minute (10-11 in DTS string)
		timeinfo.tm_min = std::stoi(timeString.substr(10, 2));

		// decimal seconds (12-17 in ISO8601 string)
		double seconds = std::stod(timeString.substr(12, 6));

#ifdef _WIN32
		// windows specific timezone handling
		char * pTZ;
		char szTZExisting[32] = "TZ=";

		// Save current TZ setting locally
		pTZ = getenv("TZ");

		if (pTZ) {
			/* remember we have "TZ=" stored as the first 3 chars of the array */
			strncpy(&szTZExisting[3], pTZ, sizeof(szTZExisting) - 4);
			szTZExisting[sizeof(szTZExisting) - 1] = 0x00;
		}

		// Change time zone to GMT
		_putenv("TZ=UTC");

		// set the timezone-offset to 0
		_timezone = 0;

		// set the DST-offset to 0
		_daylight = 0;

		// ensure _tzset() has been called, so that it isn't called again.
		_tzset();

		// convert to epoch time
		double usabletime = static_cast<double>((mktime(&timeinfo)));

		// Restore original TZ setting
		_putenv(szTZExisting);
		_tzset();

#else
		// linux and other timezone handling
		char *tz;
		char TZorig[MAXENV];

		// Save current TZ setting locally
		tz = getenv("TZ");
		if (tz != reinterpret_cast<char *>(NULL)) {
			if (strlen(tz) > MAXENV - 4) {
				printf("ConvertISO8601ToEpochTime: unable to store current TZ "
						"environment variable.\n");
				return (-1);
			}
		}
		snprintf(TZorig, sizeof(TZorig), "TZ=%s", tz);

		// Change time zone to GMT
		snprintf(envTZ, sizeof(envTZ), "TZ=GMT");
		if (putenv(envTZ) != 0) {
			printf("ConvertISO8601ToEpochTime: putenv: unable to set TZ "
					"environment variable.\n");
			return (-1);
		}

		// convert to epoch time
		double usabletime = static_cast<double>(mktime(&timeinfo));

		// Restore original TZ setting
		snprintf(envTZ, sizeof(envTZ), "%s", TZorig);
		if (putenv(envTZ) != 0) {
			printf("ConvertISO8601ToEpochTime: putenv: unable to restore TZ "
					"environment variable.\n");
		}
		tzset();
#endif

		// add decimal seconds and return
		return (static_cast<double>(usabletime) + seconds);
	} catch (const std::exception &) {
		glass3::util::Logger::log(
				"warning",
				"ConvertDTStringToEpochTime: Problem converting time string: "
						+ timeString);
	}

	return (-1.0);
}
}  // namespace util
}  // namespace glass3
