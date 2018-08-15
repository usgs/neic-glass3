/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <json.h>
#include <parser.h>
#include <string>
#include <memory>

namespace glass3 {
namespace parse {
/**
 * \brief json parser class
 *
 * The glass json parser class is a class encapsulating the logic for
 * converting a string to a json message and validating the results.
 *
 * JSON messages don't have a single format. Any of the valid detection-formats
 * messages are accepted by this parser.
 *
 * This class depends on the earthquake detection formats library to parse and
 * format message strings.
 *
 * This class uses the external SuperEasyJSON library to return parsed messages
 *
 * This class inherits from the glass3::parse::Parser class
 */
class JSONParser : public glass3::parse::Parser {
 public:
	/**
	 * \brief jsonparser constructor
	 *
	 * The constructor for the jsonparser class.
	 * Initializes members to provided values.
	 *
	 * \param defaultAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param defaultAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	JSONParser(const std::string &defaultAgencyID,
				const std::string &defaultAuthor);

	/**
	 * \brief jsonparser destructor
	 *
	 * The jsonparser for the gpickparser class.
	 */
	~JSONParser();

	/**
	 * \brief json parsing function
	 *
	 * An overridden function (from Parser) used to parse a given json
	 * formatted string (from a file, broker, or other source) into a
	 * SuperEasyJSON object (the common data interchange object) which is
	 * returned as a shared_ptr for  use in other neic-glass3 functions and
	 * classes (such as glasscore).
	 *
	 * \param input - The json formatted std::string to parse
	 * \return Returns a shared_ptr to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;  // NOLINT
};
}  // namespace parse
}  // namespace glass3
#endif  // JSONPARSER_H
