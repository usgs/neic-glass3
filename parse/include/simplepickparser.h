/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SIMPLEPICKPARSER_H
#define SIMPLEPICKPARSER_H

#include <json.h>
#include <parser.h>
#include <string>
#include <memory>

namespace glass3 {
namespace parse {
/**
 * \brief glass simple pick parser class
 *
 * The glass simple pick parser class is a class encapsulating the logic for
 * parsing a simple pick message and validating the results.
 *
  * The simple pick message has the following format:
 *  57647 AK GLI BHZ -- 1568999913.12 P
 *
 * Where:
 * index 0 is the pick id
 * index 1 is the network code
 * index 2 is the station code
 * index 3 is the channel code
 * index 4 is the location code
 * index 5 is the arrival time
 * index 6 is the phase

 * Since the simple pick format does not include source agency information, the
 * default agency ID will be used to provide source attribution combined with
 * the author/logo.
 *
 * The phase can be defined as P or S, or left blank for unknown.
 *
 * This class depends on the external earthquake detection formats library to
 * format message strings.
 *
 * This class uses the external SuperEasyJSON library to return parsed messages
 *
 * This class inherits from the glass3::parse::Parser class
 */
class SimplePickParser : public glass3::parse::Parser {
 public:
	/**
	 * \brief simplepickparser constructor
	 *
	 * The constructor for the simplepickparser class.
	 * Initializes members to provided values.
	 *

	 */
	SimplePickParser(const std::string &defaultAgencyID,
						const std::string &defaultAuthor);

	/**
	 * \brief simplepickparser destructor
	 *
	 * The destructor for the simplepickparser class.
	 */
	~SimplePickParser();

	/**
	 * \brief simple pick parsing function
	 *
	 * An overridden function (from Parser) used to parse a given simple
	 * pick string (from a file, broker, or other source) into a
	 * SuperEasyJSON object (the common data interchange object) which is
	 * returned as a shared_ptr for  use in other neic-glass3 functions and
	 * classes (such as glasscore).
	 *
	 * \param input - The simple pick formatted std::string to parse
	 * \return Returns a shared_ptr to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;
};
}  // namespace parse
}  // namespace glass3
#endif  // SIMPLEPICKPARSER_H
