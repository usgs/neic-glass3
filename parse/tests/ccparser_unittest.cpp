#include <ccparser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

#define TESTCCSTRING "2015/03/23 23:53:47.630 36.769 -98.019 5.0 1.2677417 mblg GS OK032 HHZ 00 P 2015/03/23 23:53:50.850 0.7663822 0.65" // NOLINT
#define TESTFAILSTRING "2015/03/23 -98.019 5.0 1.2677417 mblg GS OK032 HHZ 00 P 2015/03/23 23:53:50.850 0.7663822 0.65" // NOLINT
#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class CCParser : public ::testing::Test {
 protected:
	virtual void SetUp() {
		agencyid = std::string(TESTAGENCYID);
		author = std::string(TESTAUTHOR);

		Parser = new parse::CCParser(agencyid, author);
	}

	virtual void TearDown() {
		// cleanup
		delete (Parser);
	}

	std::string agencyid;
	std::string author;
	parse::CCParser * Parser;
};

// tests to see gpick parser constructs correctly
TEST_F(CCParser, Construction) {
	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getAgencyid().c_str(), agencyid.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getAuthor().c_str(), author.c_str())
	<< "Author check";
}

// test correlations
TEST_F(CCParser, CorrelationParsing) {
	std::string ccstring = std::string(TESTCCSTRING);

	std::shared_ptr<json::Object> CCObject = Parser->parse(ccstring);

	// parse the origin
	ASSERT_FALSE(CCObject == NULL)<< "Parsed cross correlation not null.";

	// validate the origin
	ASSERT_TRUE(Parser->validate(CCObject))<<
	"Parsed cross correlation is valid";
}

// test failure
TEST_F(CCParser, FailTest) {
	std::string failstring = std::string(TESTFAILSTRING);

	std::shared_ptr<json::Object> FailObject = Parser->parse(failstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed fail string is null.";

	// validate the bad data
	ASSERT_FALSE(Parser->validate(FailObject))<<
	"Parsed failstring is not valid";

	// parse empty string
	FailObject = Parser->parse("");

	// parse the empty string
	ASSERT_TRUE(FailObject == NULL)<< "Parsed empty string is null.";

	// validate the bad data
	ASSERT_FALSE(Parser->validate(FailObject))<<
	"Parsed empty string is not valid";
}
