#include <timeutil.h>
#include <logger.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <wtypes.h>
#endif

#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

namespace glass3 {
namespace util {

// maximum size of the environment varible TZ
#define MAXENV 128

/**
 * \brief A character array used to store the environment variable
 * TZ will be stored after the first call to ConvertISO8601ToEpochTime()
 */
char envTZ[MAXENV];

// ----------------------------------------------------convertEpochTimeToISO8601
std::string convertEpochTimeToISO8601(double epochtime) {
	double integerpart;
	double fractionpart;

	fractionpart = modf(epochtime, &integerpart);

	time_t time = static_cast<int>(integerpart);

	return (convertEpochTimeToISO8601(time, fractionpart));
}

// ----------------------------------------------------convertEpochTimeToISO8601
std::string convertEpochTimeToISO8601(time_t epochtime, double decimalseconds) {
	// build the time portion, all but the seconds which are
	// separate since time_t can't do decimal seconds
	char timebuf[sizeof "2011-10-08T07:07:"];

#ifdef _WIN32
	struct tm timestruct;
	gmtime_s(&timestruct, &epochtime);

#else
	struct tm timestruct;
	gmtime_r(&epochtime, &timestruct);
#endif

	strftime(timebuf, sizeof timebuf, "%Y-%m-%dT%H:%M:", &timestruct);
	std::string timestring = timebuf;

	// build the seconds portion
	char secbuf[sizeof "00.000Z"];
	if ((timestruct.tm_sec + decimalseconds) < 10) {
		snprintf(secbuf, sizeof(secbuf), "0%1.3f",
					timestruct.tm_sec + decimalseconds);
	} else {
		snprintf(secbuf, sizeof(secbuf), "%2.3f",
					timestruct.tm_sec + decimalseconds);
	}
	std::string secondsstring = secbuf;

	// return the combined ISO8601 string
	return (timestring + secondsstring + "Z");
}

// ----------------------------------------------------convertDateTimeToISO8601
std::string convertDateTimeToISO8601(const std::string &TimeString) {
	double epoch_time = convertDateTimeToEpochTime(TimeString);

	return (convertEpochTimeToISO8601(epoch_time));
}

// ----------------------------------------------------convertISO8601ToEpochTime
double convertISO8601ToEpochTime(const std::string &TimeString) {
	// make sure we got something
	if (TimeString.length() == 0) {
		logger::log("error",
					"ConvertISO8601ToEpochTime: Time string is empty.");
		return (-1.0);
	}

	// time string is too short
	if (TimeString.length() < 24) {
		logger::log("error",
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
		timeinfo.tm_year = std::stoi(TimeString.substr(0, 4)) - 1900;

		// month (5-6 in ISO8601 string)
		// struct tm stores month  as number from 0 to 11, January = 0
		timeinfo.tm_mon = std::stoi(TimeString.substr(5, 2)) - 1;

		// day (8-9 in ISO8601 string)
		timeinfo.tm_mday = std::stoi(TimeString.substr(8, 2));

		// hour (11-12 in ISO8601 string)
		timeinfo.tm_hour = std::stoi(TimeString.substr(11, 2));

		// minute (14-15 in ISO8601 string)
		timeinfo.tm_min = std::stoi(TimeString.substr(14, 2));

		// seconds (17-22 in ISO8601 string)
		double seconds = std::stod(TimeString.substr(17, 6));

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
		double usabletime = static_cast<double>(mktime(&timeinfo));

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
		logger::log(
				"warning",
				"ConvertISO8601ToEpochTime: Problem converting time string: "
						+ TimeString);
	}

	return (-1.0);
}

// --------------------------------------------------convertDateTimeToEpochTime
double convertDateTimeToEpochTime(const std::string &TimeString) {
	// make sure we got something
	if (TimeString.length() == 0) {
		logger::log("error",
					"ConvertDTStringToEpochTime: Time string is empty.");
		return (-1.0);
	}
	// time string is too short
	if (TimeString.length() < 18) {
		logger::log(
				"error",
				"ConvertDTStringToEpochTime: Time string: " + TimeString
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
		timeinfo.tm_year = std::stoi(TimeString.substr(0, 4)) - 1900;

		// month (4-5 in DTS string)
		// struct tm stores month  as number from 0 to 11, January = 0
		timeinfo.tm_mon = std::stoi(TimeString.substr(4, 2)) - 1;

		// day (6-7 in DTS string)
		timeinfo.tm_mday = std::stoi(TimeString.substr(6, 2));

		// hour (8-9 in DTS string)
		timeinfo.tm_hour = std::stoi(TimeString.substr(8, 2));

		// minute (10-11 in DTS string)
		timeinfo.tm_min = std::stoi(TimeString.substr(10, 2));

		// decimal seconds (12-17 in ISO8601 string)
		double seconds = std::stod(TimeString.substr(12, 6));

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
		logger::log(
				"warning",
				"ConvertDTStringToEpochTime: Problem converting time string: "
						+ TimeString);
	}

	return (-1.0);
}
}  // namespace util
}  // namespace glass3
