#include <parser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class parserstub : public parse::Parser {
 public:
	parserstub(const std::string &newagencyid, const std::string &newauthor)
			: parse::Parser(newagencyid, newauthor) {
	}

	~parserstub() {
	}

	std::shared_ptr<json::Object> parse(const std::string &input) override {
		return (NULL);
	}

	bool validate(std::shared_ptr<json::Object> &input) override {
		return (true);
	}
};

// tests to see if parser construts correctly
TEST(ParserTest, Construction) {
	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	parserstub * Parser = new parserstub(agencyid, author);

	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getAgencyid().c_str(), agencyid.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getAuthor().c_str(), author.c_str())<< "Author check";

	// cleanup
	delete (Parser);
}
