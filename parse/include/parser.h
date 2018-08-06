/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef PARSER_H
#define PARSER_H

#include <json.h>
#include <baseclass.h>
#include <string>
#include <memory>

namespace glass3 {

/**parse
 * \namespace util
 * \brief neic-glass3 namespace containing parsing classes and functions
 *
 * The neic-glass3 parse namespace contains various classes, and functions used
 * by other components of neic-glass3 to parse data.
 */
namespace parse {

/**
 * \brief glass3 parser class
 *
 * The glass3 parser class is a class encapsulating basic parsing
 * logic and validation for neic-glass3.
 *
 * This class uses the external SuperEasyJSON library to return parsed messages
 *
 * This class extends from glass3::util::BaseClass
 *
 * This class is intended to be extended by derived classes.
 */
class Parser : public glass3::util::BaseClass {
 public:
	Parser();

	/**
	 * \brief parser constructor
	 *
	 * The constructor for the parser class.
	 * Initializes members to provided values.
	 *
	 * \param defaultAgencyID - A std::string containing the agency id to
	 * use if one is not provided by the parsed message.
	 * \param defaultAuthor - A std::string containing the author to
	 * use if one is not provided by the parsed message.
	 */
	Parser(const std::string &defaultAgencyID,
			const std::string &defaultAuthor);

	/**
	 * \brief parser destructor
	 *
	 * The destructor for the parser class.
	 */
	virtual ~Parser();

	/**
	 * \brief Virtual parsing function to be overridden by deriving classes,
	 * used to parse input
	 *
	 * A pure virtual function used to parse a given string (from a file, broker,
	 * or other source) into a SuperEasyJSON object (the common data interchange
	 * object) which is returned as a shared_ptr for  use in other neic-glass3
	 * functions and classes (such as glasscore).
	 *
	 * \param input - The std::string to parse
	 * \return Returns a shared_ptr to the json::Object containing the data.
	 */
	virtual std::shared_ptr<json::Object> parse(const std::string &input) = 0;
};
}  // namespace parse
}  // namespace glass3
#endif  // PARSER_H
