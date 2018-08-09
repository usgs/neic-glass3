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

namespace glass3 {
namespace parse {
/**
 * \brief glass cross correlation message parser class
 *
 * The CCParser class is a class encapsulating the logic for parsing a cross
 * correlation message and validating the results.
 *
 * The correlation  message is space delimited and has the following format:
 * 2015/03/23 07:36:32.880 36.769 -98.019 5.0 2.6136482 mblg GS OK032 HHZ 00 P 2015/03/23 07:36:36.100 0.6581729 0.65  // NOLINT
 *
 * Where:
 *  index 0 is the origin date
 *  index 1 is the origin time
 *  index 2 is the latitude
 *  index 3 is the longitude
 *  index 4 is the depth
 *  index 5 is the magnitude
 *  index 6 is the magnitude type
 *  index 7 is the network code
 *  index 8 is the station code
 *  index 9 is the channel code
 *  index 10 is the location code
 *  index 11 is the phase type
 *  index 12 is the arrival date
 *  index 13 is the arrival time
 *  index 14 is the correlation value
 *  index 15 is the correlation threshold
 *
 * Since the correlation format does not include any source information, the
 * default author and default agency ID will be used to provide source
 * attribution
 *
 * This class depends on the external earthquake detection formats library to
 * format message strings.
 *
 * This class uses the external SuperEasyJSON library to return parsed messages
 *
 * This class inherits from the glass3::parse::Parser class
 */
class CCParser : public glass3::parse::Parser {
 public:
	/**
	 * \brief ccparser constructor
	 *
	 * The constructor for the ccparser class.
	 * Initializes members to provided values.
	 *
	 * \param defaultAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param defaultAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	CCParser(const std::string &defaultAgencyID,
				const std::string &defaultAuthor);

	/**
	 * \brief ccparser destructor
	 *
	 * The destructor for the ccparser class.
	 */
	~CCParser();

	/**
	 * \brief cross correlation parsing function
	 *
	 * An overridden function (from Parser) used to parse a given cross
	 * correlation string (from a file, broker, or other source) into a
	 * SuperEasyJSON object (the common data interchange object) which is
	 * returned as a shared_ptr for  use in other neic-glass3 functions and
	 * classes (such as glasscore).
	 *
	 * \param input - The cross correlation formatted std::string to parse
	 * \return Returns a shared_ptr to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;
};
}  // namespace parse
}  // namespace glass3
#endif  // CCPARSER_H
