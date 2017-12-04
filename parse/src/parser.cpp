#include <parser.h>
#include <string>

namespace parse {
Parser::Parser(std::string newAgencyID, std::string newAuthor) {
	m_AgencyID = newAgencyID;
	m_Author = newAuthor;
}

Parser::~Parser() {
}
}  // namespace parse
