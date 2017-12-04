#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <string>
#include "Date.h"

namespace glassutil {

static int mo[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 0,
		31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
static char const *cmo[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" };

static int base = 1900;	 // Base year

// ---------------------------------------------------------CDate
CDate::CDate() {
	clear();
}

// ---------------------------------------------------------CDate
CDate::CDate(double time) {
	clear();

	initialize(time);
}

// ---------------------------------------------------------CDate
CDate::CDate(unsigned int year, unsigned int month, unsigned int day,
				unsigned int hour, unsigned int minute, double second) {
	clear();

	initialize(year, month, day, hour, minute, second);
}

// ---------------------------------------------------------CDate
CDate::CDate(std::string time) {
	clear();

	initialize(time);
}

// ---------------------------------------------------------~CDate
CDate::~CDate() {
}

// ---------------------------------------------------------clear
void CDate::clear() {
	m_nYear = 0;
	m_nMonth = 0;
	m_nDay = 0;
	m_nHour = 0;
	m_nMinute = 0;
	m_dSeconds = 0;
	m_dTime = 0;
}

// ---------------------------------------------------------now
double CDate::now() {
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

	// convert using CDate
	CDate dt = CDate(timestruct.tm_year + 1900, timestruct.tm_mon + 1,
						timestruct.tm_mday, timestruct.tm_hour,
						timestruct.tm_min, timestruct.tm_sec);

	return (dt.time());
}

// ---------------------------------------------------------initialize
bool CDate::initialize(double time) {
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
bool CDate::initialize(unsigned int year, unsigned int month, unsigned int day,
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
bool CDate::initialize(std::string time) {
	// choose decode based on length of time string
	if (time.length() == 24) {
		// iso8601
		CDate(decodeISO8601Time(time));

	} else if ((time.length() > 16) && (time.length() < 19)) {
		// date time
		CDate(decodeDateTime(time));
	} else {
		// invalid format
		return (false);
	}

	return (true);
}

// ---------------------------------------------------------year
unsigned int CDate::year() {
	return (m_nYear);
}

// ---------------------------------------------------------month
unsigned int CDate::month() {
	return (m_nMonth);
}

// ---------------------------------------------------------day
unsigned int CDate::day() {
	return (m_nDay);
}

// ---------------------------------------------------------hour
unsigned int CDate::hour() {
	return (m_nHour);
}

// ---------------------------------------------------------minute
unsigned int CDate::minute() {
	return (m_nMinute);
}

// ---------------------------------------------------------seconds
double CDate::seconds() {
	return (m_dSeconds);
}

// ---------------------------------------------------------time
double CDate::time() {
	return (m_dTime);
}

// ---------------------------------------------------------date18
std::string CDate::date18() {
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
std::string CDate::date20() {
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
std::string CDate::ISO8601() {
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
std::string CDate::dateTime() {
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
double CDate::decodeDateTime(std::string datetime) {
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
	CDate dt = CDate(iyr, imo, ida, ihr, imn, sec);
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
double CDate::decodeISO8601Time(std::string iso8601) {
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
	CDate dt = CDate(iyr, imo, ida, ihr, imn, sec);
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
std::string CDate::encodeDateTime(double t) {
	CDate dt(t);
	return (dt.dateTime());
}

// ---------------------------------------------------------encodeISO8601Time
std::string CDate::encodeISO8601Time(double t) {
	CDate dt(t);
	return (dt.ISO8601());
}
}  // namespace glassutil
