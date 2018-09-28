#include <date.h>

#include <gtest/gtest.h>
#include <string>
#include <logger.h>

#define ISO8601TIME "2015-12-28T21:32:24.017Z"
#define DATETIME "20151228213224.017"
#define GREGORIANTIME 3660327144.017
#define EPOCHTIME 1451338344.017

#define ISO8601TIME2 "2015-12-28T21:32:24.500Z"
#define EPOCHTIME2 1451338344.50

#define ISO8601TIME3 "2015-12-28T21:32:25.000Z"
#define EPOCHTIME3 1451338344.9999997

#define ISO8601TIME4 "2015-12-28T21:32:24.000Z"
#define EPOCHTIME4 1451338344.00000000001

#define TIMET 1451338344
#define DECIMALSEC .017

#define YEAR 2015
#define MONTH 12
#define DAY 28
#define HOUR 21
#define MINUTE 32
#define SECONDS 24.017

// tests to see if we can convert Gregorian time to iso8601
TEST(DateTest, Construction) {
	// default constru tor
	glass3::util::Date dt;

	// year()
	ASSERT_EQ(0, dt.year())<< "Default year() Check";

	// month()
	ASSERT_EQ(0, dt.month())<< "Default month() Check";

	// day()
	ASSERT_EQ(0, dt.day())<< "Default day() Check";

	// hour()
	ASSERT_EQ(0, dt.hour())<< "Default hour() Check";

	// minute()
	ASSERT_EQ(0, dt.minute())<< "Default minute() Check";

	// seconds()
	ASSERT_EQ(0, dt.seconds())<< "Default seconds() Check";

	// time()
	ASSERT_EQ(0, dt.time())<< "Default time() Check";

	// Gregorian time constructor
	glass3::util::Date dt2(GREGORIANTIME);

	// year()
	ASSERT_EQ(YEAR, dt2.year())<< "Gregorian year() Check";

	// month()
	ASSERT_EQ(MONTH, dt2.month())<< "Gregorian month() Check";

	// day()
	ASSERT_EQ(DAY, dt2.day())<< "Gregorian day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt2.hour())<< "Gregorian hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt2.minute())<< "Gregorian minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt2.seconds(), 0.001)<< "Gregorian seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt2.time(), 0.001)<< "Gregorian time() Check";

	// full time constructor
	glass3::util::Date dt3(YEAR, MONTH, DAY, HOUR, MINUTE, SECONDS);

	// year()
	ASSERT_EQ(YEAR, dt3.year())<< "Full year() Check";

	// month()
	ASSERT_EQ(MONTH, dt3.month())<< "Full month() Check";

	// day()
	ASSERT_EQ(DAY, dt3.day())<< "Full day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt3.hour())<< "Full hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt3.minute())<< "Full minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt3.seconds(), 0.001)<< "Full seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt3.time(), 0.001)<< "Full time() Check";
}

// tests to see if we can convert Gregorian time to iso8601
TEST(DateTest, Initialize) {
	glass3::util::Logger::disable();

	// Gregorian init
	glass3::util::Date dt;
	dt.initialize(GREGORIANTIME);

	// year()
	ASSERT_EQ(YEAR, dt.year())<< "Gregorian year() Check";

	// month()
	ASSERT_EQ(MONTH, dt.month())<< "Gregorian month() Check";

	// day()
	ASSERT_EQ(DAY, dt.day())<< "Gregorian day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt.hour())<< "Gregorian hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt.minute())<< "Gregorian minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt.seconds(), 0.001)<< "Gregorian seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt.time(), 0.001)<< "Gregorian time() Check";

	// full init
	glass3::util::Date dt2;

	dt2.initialize(YEAR, MONTH, DAY, HOUR, MINUTE, SECONDS);

	// year()
	ASSERT_EQ(YEAR, dt2.year())<< "Full year() Check";

	// month()
	ASSERT_EQ(MONTH, dt2.month())<< "Full month() Check";

	// day()
	ASSERT_EQ(DAY, dt2.day())<< "Full day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt2.hour())<< "Full hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt2.minute())<< "Full minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt2.seconds(), 0.001)<< "Full seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt2.time(), 0.001)<< "Full time() Check";

	// iso string init
	glass3::util::Date dt3;

	dt3.initialize(std::string(ISO8601TIME));

	// year()
	ASSERT_EQ(YEAR, dt3.year())<< "ISO String year() Check";

	// month()
	ASSERT_EQ(MONTH, dt3.month())<< "ISO String month() Check";

	// day()
	ASSERT_EQ(DAY, dt3.day())<< "ISO String day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt3.hour())<< "ISO String hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt3.minute())<< "ISO String minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt3.seconds(), 0.001)<< "ISO String seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt3.time(), 0.001)<< "ISO String time() Check";

	// date string init
	glass3::util::Date dt4;

	dt4.initialize(std::string(DATETIME));

	// year()
	ASSERT_EQ(YEAR, dt4.year())<< "date String year() Check";

	// month()
	ASSERT_EQ(MONTH, dt4.month())<< "date String month() Check";

	// day()
	ASSERT_EQ(DAY, dt4.day())<< "date String day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt4.hour())<< "date String hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt4.minute())<< "date String minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt4.seconds(), 0.001)<< "date String seconds() Check";

	// time()
	ASSERT_NEAR(GREGORIANTIME, dt4.time(), 0.001)<< "date String time() Check";
}

// tests to see if we can convert Gregorian time to iso8601
TEST(DateTest, ConvertGregorianTimeToISO8601) {
	glass3::util::Logger::disable();

	glass3::util::Date dt;

	std::string ExpectedISO8601 = std::string(ISO8601TIME);

	// test decimal Gregorian time to ISO8601 conversion
	std::string ConvertedISO8601 = dt.encodeISO8601Time(GREGORIANTIME);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToISO8601 is functional
TEST(DateTest, ConvertGregorianTimeToDateTime) {
	glass3::util::Logger::disable();

	glass3::util::Date dt;

	std::string ExpectedDateTime = std::string(DATETIME);

	// test decimal Gregorian time to DateTime conversion
	std::string ConvertedDateTime = dt.encodeDateTime(GREGORIANTIME);
	ASSERT_STREQ(ConvertedDateTime.c_str(), ExpectedDateTime.c_str());
}

// tests to see if ConvertDateTimeToEpochTime is functional
TEST(DateTest, ConvertDateTimeToGregorianTime) {
	glass3::util::Logger::disable();

	glass3::util::Date dt;

	double ExpectedJulieanTime = GREGORIANTIME;
	std::string DateTime = std::string(DATETIME);

	// test DateTime to Gregorian time conversion
	double ConvertedGregorianTime = dt.decodeDateTime(DateTime);
	ASSERT_NEAR(ConvertedGregorianTime, ExpectedJulieanTime, 0.0001);
}

// tests to see if ConvertISO8601ToEpochTime is functional
TEST(DateTest, ConvertISO8601ToGregorianTime) {
	glass3::util::Logger::disable();

	glass3::util::Date dt;

	double ExpectedJulieanTime = GREGORIANTIME;
	std::string ISO8601Time = std::string(ISO8601TIME);

	// test ISO8601 to Gregorian time conversion
	double ConvertedGregorianTime = dt.decodeISO8601Time(ISO8601Time);
	ASSERT_NEAR(ConvertedGregorianTime, ExpectedJulieanTime, 0.0001);
}

// tests to see if ConvertEpochTimeToISO8601 is functional
TEST(DateTest, ConvertEpochTimeToISO8601) {
	std::string ExpectedISO8601 = std::string(ISO8601TIME);

	// test decimal epoch time to ISO8601 conversion
	double dEpochTime = EPOCHTIME;
	std::string ConvertedISO8601 =
			glass3::util::Date::convertEpochTimeToISO8601(dEpochTime);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());

	// test time_t style epoch time to ISO8601 conversion
	time_t tEpochTime = TIMET;
	double dDecimalSec = DECIMALSEC;
	ConvertedISO8601 = glass3::util::Date::convertEpochTimeToISO8601(
			tEpochTime, dDecimalSec);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());

	// edge case 1
	double dEpochTime2 = EPOCHTIME2;
	ExpectedISO8601 = std::string(ISO8601TIME2);
	ConvertedISO8601 = glass3::util::Date::convertEpochTimeToISO8601(
			dEpochTime2);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());

	// edge case 2
	double dEpochTime3 = EPOCHTIME3;
	ExpectedISO8601 = std::string(ISO8601TIME3);
	ConvertedISO8601 = glass3::util::Date::convertEpochTimeToISO8601(
			dEpochTime3);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());

	// edge case 3
	double dEpochTime4 = EPOCHTIME4;
	ExpectedISO8601 = std::string(ISO8601TIME4);
	ConvertedISO8601 = glass3::util::Date::convertEpochTimeToISO8601(
			dEpochTime4);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToISO8601 is functional
TEST(DateTest, ConvertDateTimeToISO8601) {
	std::string ExpectedISO8601 = std::string(ISO8601TIME);
	std::string DateTime = std::string(DATETIME);

	// test decimal epoch time to ISO8601 conversion
	std::string ConvertedISO8601 = glass3::util::Date::convertDateTimeToISO8601(
			DateTime);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToEpochTime is functional
TEST(DateTest, ConvertDateTimeToEpochTime) {
	double ExpectedEpochTime = EPOCHTIME;
	std::string DateTime = std::string(DATETIME);

	// test DateTime to epoch time conversion
	double ConvertedEpochTime = glass3::util::Date::convertDateTimeToEpochTime(
			DateTime);
	ASSERT_EQ(ConvertedEpochTime, ExpectedEpochTime);
}

// tests to see if ConvertISO8601ToEpochTime is functional
TEST(DateTest, ConvertISO8601ToEpochTime) {
	double ExpectedEpochTime = EPOCHTIME;
	std::string ISO8601Time = std::string(ISO8601TIME);

	// test ISO8601 to  epoch time conversion
	double ConvertedEpochTime = glass3::util::Date::convertISO8601ToEpochTime(
			ISO8601Time);
	ASSERT_EQ(ConvertedEpochTime, ExpectedEpochTime);
}

// fail tests
TEST(DateTest, FailTests) {
	// test various bad strings
	ASSERT_EQ(glass3::util::Date::convertISO8601ToEpochTime(""), -1);
	ASSERT_EQ(
			glass3::util::Date::convertISO8601ToEpochTime(
					"12345678901234567890"),
			-1);
	ASSERT_EQ(
			glass3::util::Date::convertISO8601ToEpochTime(
					"AAAAAAAAAAAAAAAAAAAAAAAA"),
			-1);
	ASSERT_EQ(glass3::util::Date::convertDateTimeToEpochTime(""), -1);
	ASSERT_EQ(
			glass3::util::Date::convertDateTimeToEpochTime("1234567890123456"),
			-1);
	ASSERT_EQ(
			glass3::util::Date::convertDateTimeToEpochTime(
					"AAAAAAAAAAAAAAAAAAAAAAAA"),
			-1);
}
