#include <ccparser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

// Input CC data that should work.
#define TESTCCSTRING "2015/03/23 23:53:47.630 36.769 -98.019 5.0 1.2677417 mblg GS OK032 HHZ 00 P 2015/03/23 23:53:50.850 0.7663822 0.65" // NOLINT

// Input CC data that should fail missing time.
#define TESTFAILSTRING "2015/03/23 -98.019 5.0 1.2677417 mblg GS OK032 HHZ 00 P 2015/03/23 23:53:50.850 0.7663822 0.65" // NOLINT

// agency/m_Author for testing
#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

// create a testing class to allocate and host the cc parser
class CCParser : public ::testing::Test {
 protected:
	virtual void SetUp() {
		m_AgencyId = std::string(TESTAGENCYID);
		m_Author = std::string(TESTAUTHOR);

		m_Parser = new glass3::parse::CCParser(m_AgencyId, m_Author);
	}

	virtual void TearDown() {
		// cleanup
		delete (m_Parser);
	}

	std::string m_AgencyId;
	std::string m_Author;
	glass3::parse::CCParser * m_Parser;
};

// tests to see correlation parser constructs correctly
TEST_F(CCParser, Construction) {
	// assert that m_AgencyId is ok
	ASSERT_STREQ(m_Parser->getDefaultAgencyId().c_str(), m_AgencyId.c_str())<<
	"AgencyID check";

	// assert that m_Author is ok
	ASSERT_STREQ(m_Parser->getDefaultAuthor().c_str(), m_Author.c_str())
	<< "Author check";
}

// test correlations
TEST_F(CCParser, CorrelationParsing) {
	std::string ccstring = std::string(TESTCCSTRING);

	std::shared_ptr<json::Object> CCObject = m_Parser->parse(ccstring);

	// parse the origin
	ASSERT_FALSE(CCObject == NULL)<< "Parsed cross correlation not null.";
}

// test failure
TEST_F(CCParser, FailTest) {
	std::string failstring = std::string(TESTFAILSTRING);

	std::shared_ptr<json::Object> FailObject = m_Parser->parse(failstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed fail string is null.";

	// parse empty string
	FailObject = m_Parser->parse("");

	// parse the empty string
	ASSERT_TRUE(FailObject == NULL)<< "Parsed empty string is null.";
}
