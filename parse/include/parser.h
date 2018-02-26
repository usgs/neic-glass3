/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef PARSER_H
#define PARSER_H

#include <json.h>
#include <string>
#include <memory>

namespace parse {
/**
 * \brief glass parser class
 *
 * The glass parser class is a class encapsulating basic parsing
 * logic and validation.
 *
 * This class is intended to be extended by derived classes.
 */
class Parser {
 public:
	/**
	 * \brief parser constructor
	 *
	 * The constructor for the parser class.
	 * Initializes members to provided values.
	 *
	 * \param newAgencyID - A std::string containing the agency id to
	 * use if one is not provided by the parsed message.
	 * \param newAuthor - A std::string containing the author to
	 * use if one is not provided by the parsed message.
	 */
	Parser(const std::string &newAgencyID, const std::string &newAuthor);

	/**
	 * \brief parser destructor
	 *
	 * The destructor for the parser class.
	 */
	virtual ~Parser();

	/**
	 * \brief parsing function
	 *
	 * Virtual parsing function to be overridden by deriving classes
	 *
	 * \param input - The std::string to parse
	 * \return Returns a pointer to the json::Object containing
	 * the data.
	 */
	virtual std::shared_ptr<json::Object> parse(const std::string &input) = 0;

	/**
	 * \brief validation function
	 *
	 * Virtual validation function to be overridden by deriving classes
	 *
	 * \param input - A pointer to a json::Object containing the data to
	 * validate.
	 * \return Returns true if valid, false otherwise.
	 */
	virtual bool validate(std::shared_ptr<json::Object> &input) = 0; // NOLINT

	/**
	 * \brief getter for the agencyid configuration variable
	 */
	const std::string& getAgencyid() const {
		return (m_AgencyID);
	}

	/**
	 * \brief getter for the author configuration variable
	 */
	const std::string& getAuthor() const {
		return (m_Author);
	}

 protected:
	/**
	 * \brief A std::string containing the agency id to
	 * use if one is not provided.
	 */
	std::string m_AgencyID;

	/**
	 * \brief A std::string containing the author to
	 * use if one is not provided.
	 */
	std::string m_Author;
};
}  // namespace parse
#endif  // PARSER_H
