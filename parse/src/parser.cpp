#include <parser.h>
#include <string>

namespace glass3 {
namespace parse {

// ---------------------------------------------------------Parser
Parser::Parser()
		: glass3::util::BaseClass() {
	setAgencyId("");
	setAuthor("");
}

// ---------------------------------------------------------Parser
Parser::Parser(const std::string &defaultAgencyID,
				const std::string &defaultAuthor)
		: glass3::util::BaseClass() {
	setAgencyId(defaultAgencyID);
	setAuthor(defaultAuthor);
}

// ---------------------------------------------------------~Parser
Parser::~Parser() {
}

// ---------------------------------------------------------getAgencyId
const std::string& Parser::getAgencyId() {
	return (m_AgencyID);
}

// ---------------------------------------------------------setAgencyId
void Parser::setAgencyId(std::string id) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_AgencyID = id;
}

// ---------------------------------------------------------getAuthor
const std::string& Parser::getAuthor() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_Author);
}

// ---------------------------------------------------------setAuthor
void Parser::setAuthor(std::string author) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_Author = author;
}
}  // namespace parse
}  // namespace glass3
