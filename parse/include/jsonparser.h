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

namespace parse {
/**
 * \brief json parser class
 *
 * The glass json parser class is a class encapsulating the logic for
 * converting a string to a json message and validating the results.
 *
 * This class inherits from the parser class
 */
class JSONParser : public Parser {
 public:
	/**
	 * \brief jsonparser constructor
	 *
	 * The constructor for the jsonparser class.
	 * Initializes members to provided values.
	 *
	 * \param newAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param newAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	JSONParser(std::string newAgencyID, std::string newAuthor);

	/**
	 * \brief jsonparser destructor
	 *
	 * The jsonparser for the gpickparser class.
	 */
	~JSONParser();

	/**
	 * \brief json parsing function
	 *
	 * Parsing function that parses json formatted strings
	 * into json::Objects
	 *
	 * \param input - The global pick formatted std::string to parse
	 * \return Returns a pointer to the json::Object containing
	 * the data.
	 */
	json::Object* parse(std::string input) override;

	/**
	 * \brief json validation function
	 *
	 * Validates a json object
	 *
	 * \param input - A pointer to a json::Object containing the data to
	 * validate.
	 * \return Returns true if valid, false otherwise.
	 */
	bool validate(json::Object* input) override;
};
}  // namespace parse
#endif  // JSONPARSER_H
