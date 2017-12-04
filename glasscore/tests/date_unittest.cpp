#include <gtest/gtest.h>
#include <string>
#include "Date.h"
#include "Logit.h"

#define ISO8601TIME "2015-12-28T21:32:24.017Z"
#define DATETIME "20151228213224.017"
#define JULIANTIME 3660327144.017

// tests to see if we can convert julian time to iso8601
TEST(DateTest, ConvertJulianTimeToISO8601) {
	glassutil::CLogit::disable();

	glassutil::CDate dt;

	std::string ExpectedISO8601 = std::string(ISO8601TIME);

	// test decimal julian time to ISO8601 conversion
	std::string ConvertedISO8601 = dt.encodeISO8601Time(JULIANTIME);
	ASSERT_STREQ(ConvertedISO8601.c_str(), ExpectedISO8601.c_str());
}

// tests to see if ConvertDateTimeToISO8601 is functional
TEST(DateTest, ConvertJulianTimeToDateTime) {
	glassutil::CLogit::disable();

	glassutil::CDate dt;

	std::string ExpectedDateTime = std::string(DATETIME);

	// test decimal julian time to DateTime conversion
	std::string ConvertedDateTime = dt.encodeDateTime(JULIANTIME);
	ASSERT_STREQ(ConvertedDateTime.c_str(), ExpectedDateTime.c_str());
}

// tests to see if ConvertDateTimeToEpochTime is functional
TEST(DateTest, ConvertDateTimeToJulianTime) {
	glassutil::CLogit::disable();

	glassutil::CDate dt;

	double ExpectedJulieanTime = JULIANTIME;
	std::string DateTime = std::string(DATETIME);

	// test DateTime to julian time conversion
	double ConvertedJulianTime = dt.decodeDateTime(DateTime);
	ASSERT_NEAR(ConvertedJulianTime, ExpectedJulieanTime, 0.0001);
}

// tests to see if ConvertISO8601ToEpochTime is functional
TEST(DateTest, ConvertISO8601ToJulianTime) {
	glassutil::CLogit::disable();

	glassutil::CDate dt;

	double ExpectedJulieanTime = JULIANTIME;
	std::string ISO8601Time = std::string(ISO8601TIME);

	// test ISO8601 to julian time conversion
	double ConvertedJulianTime = dt.decodeISO8601Time(ISO8601Time);
	ASSERT_NEAR(ConvertedJulianTime, ExpectedJulieanTime, 0.0001);
}
