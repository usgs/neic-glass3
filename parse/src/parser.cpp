#include <parser.h>
#include <string>

namespace glass3 {
namespace parse {

// ---------------------------------------------------------Parser
Parser::Parser()
		: glass3::util::BaseClass() {
}

// ---------------------------------------------------------Parser
Parser::Parser(const std::string &defaultAgencyID,
				const std::string &defaultAuthor)
		: glass3::util::BaseClass() {
	setDefaultAgencyId(defaultAgencyID);
	setDefaultAuthor(defaultAuthor);
}

// ---------------------------------------------------------~Parser
Parser::~Parser() {
}
}  // namespace parse
}  // namespace glass3
