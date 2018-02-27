/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CCPARSER_H
#define CCPARSER_H

#include <json.h>
#include <parser.h>
#include <string>
#include <memory>

namespace parse {
/**
 * \brief glass cross correlation parser class
 *
 * The glass cross correlation parser class is a class encapsulating the logic
 * for parsing a cross correlation message and validating the results.
 *
 * This class inherits from the parser class
 */
class CCParser : public Parser {
 public:
	/**
	 * \brief ccparser constructor
	 *
	 * The constructor for the ccparser class.
	 * Initializes members to provided values.
	 *
	 * \param newAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param newAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	CCParser(const std::string &newAgencyID, const std::string &newAuthor);

	/**
	 * \brief ccparser destructor
	 *
	 * The destructor for the ccparser class.
	 */
	~CCParser();

	/**
	 * \brief cross correlation parsing function
	 *
	 * Parsing function that parses cross correlation formatted strings
	 * into json::Objects
	 *
	 * \param input - The cross correlation formatted std::string to parse
	 * \return Returns a pointer to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;

	/**
	 * \brief cross correlation validation function
	 *
	 * Validates a cross correlation object
	 *
	 * \param input - A pointer to a json::Object containing the data to
	 * validate.
	 * \return Returns true if valid, false otherwise.
	 */
	bool validate(std::shared_ptr<json::Object> &input) override;
};
}  // namespace parse
#endif  // CCPARSER_H
