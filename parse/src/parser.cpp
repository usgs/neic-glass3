#include <parser.h>
#include <string>

namespace glass3 {
namespace parse {

// ---------------------------------------------------------Parser
Parser::Parser() {
	setAgencyId("");
	setAuthor("");
}

// ---------------------------------------------------------Parser
Parser::Parser(const std::string &newAgencyID, const std::string &newAuthor) {
	setAgencyId(newAgencyID);
	setAuthor(newAuthor);
}

// ---------------------------------------------------------~Parser
Parser::~Parser() {
}

// ---------------------------------------------------------getAgencyId
const std::string& Parser::getAgencyId() {
	std::lock_guard<std::mutex> guard(getMutex());
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
