#include <gtest/gtest.h>
#include <timeutil.h>
#include <ctime>
#include <string>

#define ISO8601TIME "2015-12-28T21:32:24.017Z"
#define DATETIME "20151228213224.017"
#define EPOCHTIME 1451338344.017
#define TIMET 1451338344
#define DECIMALSEC .017

// tests to see if ConvertEpochTimeToISO8601 is functional
TEST(TimeUtil, ConvertEpochTimeToISO8601) {
	std::string ExpectedISO8601 = std::string(ISO8601TIME);

	// test decimal epoch time to ISO8601 conversion
	double dEpochTime = EPOCHTIME;
	std::string ConvertedISO8601 = util::convertEpochTimeToISO8601(dEpochTime);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());

	// test time_t style epoch time to ISO8601 conversion
	time_t tEpochTime = TIMET;
	double dDecimalSec = DECIMALSEC;
	ConvertedISO8601 = util::convertEpochTimeToISO8601(tEpochTime, dDecimalSec);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToISO8601 is functional
TEST(TimeUtil, ConvertDateTimeToISO8601) {
	std::string ExpectedISO8601 = std::string(ISO8601TIME);
	std::string DateTime = std::string(DATETIME);

	// test decimal epoch time to ISO8601 conversion
	std::string ConvertedISO8601 = util::convertDateTimeToISO8601(DateTime);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToEpochTime is functional
TEST(TimeUtil, ConvertDateTimeToEpochTime) {
	double ExpectedEpochTime = EPOCHTIME;
	std::string DateTime = std::string(DATETIME);

	// test DateTime to epoch time conversion
	double ConvertedEpochTime = util::convertDateTimeToEpochTime(DateTime);
	ASSERT_EQ(ConvertedEpochTime, ExpectedEpochTime);
}

// tests to see if ConvertISO8601ToEpochTime is functional
TEST(TimeUtil, ConvertISO8601ToEpochTime) {
	double ExpectedEpochTime = EPOCHTIME;
	std::string ISO8601Time = std::string(ISO8601TIME);

	// test ISO8601 to  epoch time conversion
	double ConvertedEpochTime = util::convertISO8601ToEpochTime(ISO8601Time);
	ASSERT_EQ(ConvertedEpochTime, ExpectedEpochTime);
}

// fail tests
TEST(TimeUtil, FailTests) {
	// test various bad strings
	ASSERT_EQ(util::convertISO8601ToEpochTime(""), -1);
	ASSERT_EQ(util::convertISO8601ToEpochTime("12345678901234567890"), -1);
	ASSERT_EQ(util::convertISO8601ToEpochTime("AAAAAAAAAAAAAAAAAAAAAAAA"), -1);
	ASSERT_EQ(util::convertDateTimeToEpochTime(""), -1);
	ASSERT_EQ(util::convertDateTimeToEpochTime("1234567890123456"), -1);
	ASSERT_EQ(util::convertDateTimeToEpochTime("AAAAAAAAAAAAAAAAAAAAAAAA"), -1);
}

