#include <gtest/gtest.h>
#include <string>
#include "Date.h"
#include "Logit.h"

#define ISO8601TIME "2015-12-28T21:32:24.017Z"
#define DATETIME "20151228213224.017"
#define JULIANTIME 3660327144.017

#define YEAR 2015
#define MONTH 12
#define DAY 28
#define HOUR 21
#define MINUTE 32
#define SECONDS 24.017

// tests to see if we can convert julian time to iso8601
TEST(DateTest, Construction) {
	// default constru tor
	glassutil::CDate dt;

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

	// julian time constructor
	glassutil::CDate dt2(JULIANTIME);

	// year()
	ASSERT_EQ(YEAR, dt2.year())<< "Julian year() Check";

	// month()
	ASSERT_EQ(MONTH, dt2.month())<< "Julian month() Check";

	// day()
	ASSERT_EQ(DAY, dt2.day())<< "Julian day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt2.hour())<< "Julian hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt2.minute())<< "Julian minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt2.seconds(), 0.001)<< "Julian seconds() Check";

	// time()
	ASSERT_NEAR(JULIANTIME, dt2.time(), 0.001)<< "Julian time() Check";

	// full time constructor
	glassutil::CDate dt3(YEAR, MONTH, DAY, HOUR, MINUTE, SECONDS);

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
	ASSERT_NEAR(JULIANTIME, dt3.time(), 0.001)<< "Full time() Check";
}

// tests to see if we can convert julian time to iso8601
TEST(DateTest, Initialize) {
	glassutil::CLogit::disable();

	// julian init
	glassutil::CDate dt;
	dt.initialize(JULIANTIME);

	// year()
	ASSERT_EQ(YEAR, dt.year())<< "Julian year() Check";

	// month()
	ASSERT_EQ(MONTH, dt.month())<< "Julian month() Check";

	// day()
	ASSERT_EQ(DAY, dt.day())<< "Julian day() Check";

	// hour()
	ASSERT_EQ(HOUR, dt.hour())<< "Julian hour() Check";

	// minute()
	ASSERT_EQ(MINUTE, dt.minute())<< "Julian minute() Check";

	// seconds()
	ASSERT_NEAR(SECONDS, dt.seconds(), 0.001)<< "Julian seconds() Check";

	// time()
	ASSERT_NEAR(JULIANTIME, dt.time(), 0.001)<< "Julian time() Check";

	// full init
	glassutil::CDate dt2;

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
	ASSERT_NEAR(JULIANTIME, dt2.time(), 0.001)<< "Full time() Check";

	// iso string init
	glassutil::CDate dt3;

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
	ASSERT_NEAR(JULIANTIME, dt3.time(), 0.001)<< "ISO String time() Check";

	// date string init
	glassutil::CDate dt4;

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
	ASSERT_NEAR(JULIANTIME, dt4.time(), 0.001)<< "date String time() Check";
}

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
