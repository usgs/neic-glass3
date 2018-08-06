#include <parser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class parserstub : public glass3::parse::Parser {
 public:
	parserstub()
			: glass3::parse::Parser() {
	}

	parserstub(const std::string &newagencyid, const std::string &newauthor)
			: glass3::parse::Parser(newagencyid, newauthor) {
	}

	~parserstub() {
	}

	std::shared_ptr<json::Object> parse(const std::string &input) override {
		return (NULL);
	}
};

// tests to see if parser construts correctly
TEST(ParserTest, Construction) {
	// default constructor
	parserstub * Parser = new parserstub();
	std::string emptyString = "";

	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getDefaultAgencyId().c_str(), emptyString.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getDefaultAuthor().c_str(), emptyString.c_str())<<
	"Author check";

	ASSERT_TRUE(Parser->parse(emptyString) == NULL)<< "parse returns null";

	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	// advanced constructor
	parserstub * Parser2 = new parserstub(agencyid, author);

	// assert that agencyid is ok
	ASSERT_STREQ(Parser2->getDefaultAgencyId().c_str(), agencyid.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser2->getDefaultAuthor().c_str(), author.c_str())<<
	"Author check";

	// cleanup
	delete (Parser);
	delete (Parser2);
}
