#include <gpickparser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

#define TESTGPICKSTRING1 "228041013 22637648 1 BOZ BHZ US 00 20150303000044.175 P -1.0000 U  ? m 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTGPICKSTRING2 "228041013 22637649 1 BOZ BHZ US 00 20150303000045.175 P -1.0000 D  i r 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTGPICKSTRING3 "228041013 22637650 1 BOZ BHZ US 00 20150303000046.175 P -1.0000 U  e l 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTGPICKSTRING4 "228041013 22637651 1 BOZ BHZ US 00 20150303000047.175 P -1.0000 U  q e 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTGPICKSTRING5 "228041013 22637652 1 BOZ BHZ US 00 20150303000048.175 P -1.0000 U  q U 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT

#define TESTFAILSTRING1 "228041013 22637648 1 BOZ BHZ US 00 P -1.0000 U  ? r 1.050 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTFAILSTRING2 "228041013 22637648 1 BOZ BHZ US 00 20150303000044.175 P -1.0000 U  ? m BBC 2.650 0.0 0.000000 3.49 0.000000 0.000000" // NOLINT
#define TESTFAILSTRING3 "228041013 22637649 1 BOZ BHZ US 00 20150303000045.175 P -1.0000 D  i r 1.050 2.650 0.0 0.000000 AFW 0.000000 0.000000" // NOLINT

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class GPickParser : public ::testing::Test {
 protected:
	virtual void SetUp() {
		agencyid = std::string(TESTAGENCYID);
		author = std::string(TESTAUTHOR);

		Parser = new glass3::parse::GPickParser(agencyid, author);
	}

	virtual void TearDown() {
		// cleanup
		delete (Parser);
	}

	std::string agencyid;
	std::string author;
	glass3::parse::GPickParser * Parser;
};

// tests to see gpick parser constructs correctly
TEST_F(GPickParser, Construction) {
	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getDefaultAgencyId().c_str(), agencyid.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getDefaultAuthor().c_str(), author.c_str())
	<< "Author check";
}

// test picks
TEST_F(GPickParser, PickParsing) {
	std::string pickstring1 = std::string(TESTGPICKSTRING1);
	std::string pickstring2 = std::string(TESTGPICKSTRING2);
	std::string pickstring3 = std::string(TESTGPICKSTRING3);
	std::string pickstring4 = std::string(TESTGPICKSTRING4);
	std::string pickstring5 = std::string(TESTGPICKSTRING5);

	// pick 1
	std::shared_ptr<json::Object> PickObject = Parser->parse(pickstring1);

	// parse the origin
	ASSERT_FALSE(PickObject == NULL)<< "Parsed pick 1 not null.";

	// pick 2
	std::shared_ptr<json::Object> PickObject2 = Parser->parse(pickstring2);

	// parse the origin
	ASSERT_FALSE(PickObject2 == NULL)<< "Parsed pick 2 not null.";

	// pick 3
	std::shared_ptr<json::Object> PickObject3 = Parser->parse(pickstring3);

	// parse the origin
	ASSERT_FALSE(PickObject3 == NULL)<< "Parsed pick 3 not null.";

	// pick 4
	std::shared_ptr<json::Object> PickObject4 = Parser->parse(pickstring4);

	// parse the origin
	ASSERT_FALSE(PickObject4 == NULL)<< "Parsed pick 4 not null.";

	// pick 5
	std::shared_ptr<json::Object> PickObject5 = Parser->parse(pickstring5);

	// parse the origin
	ASSERT_FALSE(PickObject5 == NULL)<< "Parsed pick 2 not null.";
}

// test failure
TEST_F(GPickParser, FailTest) {
	std::string failstring1 = std::string(TESTFAILSTRING1);
	std::string failstring2 = std::string(TESTFAILSTRING2);
	std::string failstring3 = std::string(TESTFAILSTRING3);

	// test missing time
	std::shared_ptr<json::Object> FailObject = Parser->parse(failstring1);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed fail object is null.";

	// test bad filter
	FailObject = Parser->parse(failstring2);

	// parse the bad data
	ASSERT_FALSE(FailObject == NULL)<< "Parsed fail object is not null.";

	// test bad snr
	FailObject = Parser->parse(failstring3);

	// parse the bad data
	ASSERT_FALSE(FailObject == NULL)<< "Parsed fail object is not null.";

	// test empty string
	FailObject = Parser->parse("");

	// parse the empty string
	ASSERT_TRUE(FailObject == NULL)<< "Parsed empty object is null.";
}
