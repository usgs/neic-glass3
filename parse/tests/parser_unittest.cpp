#include <parser.h>
#include <gtest/gtest.h>

#include <string>

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class parserstub : public parse::Parser {
 public:
	parserstub(std::string newagencyid, std::string newauthor)
			: parse::Parser(newagencyid, newauthor) {
	}

	~parserstub() {
	}

	json::Object* parse(std::string input) override {
		return (NULL);
	}

	bool validate(json::Object* input) override {
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
