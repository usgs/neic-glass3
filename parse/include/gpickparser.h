/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GPICKPARSER_H
#define GPICKPARSER_H

#include <json.h>
#include <parser.h>
#include <string>
#include <memory>

namespace parse {
/**
 * \brief glass global pick parser class
 *
 * The glass global pick parser class is a class encapsulating the logic for
 * parsing a global pick message and validating the results.
 *
 * This class inherits from the parser class
 */
class GPickParser : public Parser {
 public:
	/**
	 * \brief gpickparser constructor
	 *
	 * The constructor for the gpickparser class.
	 * Initializes members to provided values.
	 *
	 * \param newAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param newAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	GPickParser(const std::string &newAgencyID, const std::string &newAuthor);

	/**
	 * \brief gpickparser destructor
	 *
	 * The destructor for the gpickparser class.
	 */
	~GPickParser();

	/**
	 * \brief global pick parsing function
	 *
	 * Parsing function that parses global pick formatted strings
	 * into json::Objects
	 *
	 * \param input - The global pick formatted std::string to parse
	 * \return Returns a pointer to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;

	/**
	 * \brief pick validation function
	 *
	 * Validates a pick object
	 *
	 * \param input - A pointer to a json::Object containing the data to
	 * validate.
	 * \return Returns true if valid, false otherwise.
	 */
	bool validate(std::shared_ptr<json::Object> input) override;
};
}  // namespace parse
#endif  // GPICKPARSER_H
