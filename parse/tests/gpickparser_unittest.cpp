#include <gpickparser.h>
#include <gtest/gtest.h>

#include <string>

#define TESTGPICKSTRING "228041013 22637648 1 BOZ BHZ US 00 20150303000044.175 P -1.0000 U  ? r 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTFAILSTRING "228041013 22637648 1 BOZ BHZ US 00 P -1.0000 U  ? r 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class GPickParser : public ::testing::Test {
 protected:
	virtual void SetUp() {
		agencyid = std::string(TESTAGENCYID);
		author = std::string(TESTAUTHOR);

		Parser = new parse::GPickParser(agencyid, author);
	}

	virtual void TearDown() {
		// cleanup
		delete (Parser);
	}

	std::string agencyid;
	std::string author;
	parse::GPickParser * Parser;
};

// tests to see gpick parser constructs correctly
TEST_F(GPickParser, Construction) {
	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getAgencyid().c_str(), agencyid.c_str())<<
			"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getAuthor().c_str(), author.c_str())
	<< "Author check";
}

// test picks
TEST_F(GPickParser, PickParsing) {
	std::string pickstring = std::string(TESTGPICKSTRING);

	json::Object * PickObject = Parser->parse(pickstring);

	// parse the origin
	ASSERT_FALSE(PickObject == NULL)<< "Parsed pick not null.";

	// validate the origin
	ASSERT_TRUE(Parser->validate(PickObject))<< "Parsed origin is valid";
}

// test failure
TEST_F(GPickParser, FailTest) {
	std::string failstring = std::string(TESTFAILSTRING);

	json::Object * FailObject = Parser->parse(failstring);

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
