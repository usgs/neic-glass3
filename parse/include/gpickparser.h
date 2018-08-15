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

namespace glass3 {
namespace parse {
/**
 * \brief glass global pick parser class
 *
 * The glass global pick parser class is a class encapsulating the logic for
 * parsing a global pick message and validating the results.
 *
 * The global pick message is space delimited and has the following format:
 * 228041013 22637620 1 GLI BHZ AK -- 20150302235859.307 P -1.0000 U  ? r 1.050 2.650 0.0 0.000000 5.00 0.000000 0.000000  // NOLINT
 *
 * Where:
 * index 0 is the author/logo
 * index 1 is the pick id
 * index 2 is the global pick message version
 * index 3 is the station code
 * index 4 is the channel code
 * index 5 is the network code
 * index 6 is the location code
 * index 7 is the arrival time
 * index 8 is the phase
 * index 9 is the error window half width
 * index 10 is the polarity
 * index 11 is the onset
 * index 12 is the picker type
 * index 13 is the high pass frequency
 * index 14 is the low pass frequency
 * index 15 is the back azimuth
 * index 16 is the slowness
 * index 17 is the SNR
 * index 18 is the amplitude
 * index 19 is the period
 *
 * Since the global pick format does not include source agency information, the
 * default agency ID will be used to provide source attribution combined with
 * the author/logo
 *
 * This class depends on the external earthquake detection formats library to
 * format message strings.
 *
 * This class uses the external SuperEasyJSON library to return parsed messages
 *
 * This class inherits from the glass3::parse::Parser class
 */
class GPickParser : public glass3::parse::Parser {
 public:
	/**
	 * \brief gpickparser constructor
	 *
	 * The constructor for the gpickparser class.
	 * Initializes members to provided values.
	 *
	 * \param defaultAgencyID - A std::string containing the agency id to
	 * use if one is not provided.
	 * \param defaultAuthor - A std::string containing the author to
	 * use if one is not provided.
	 */
	GPickParser(const std::string &defaultAgencyID,
				const std::string &defaultAuthor);

	/**
	 * \brief gpickparser destructor
	 *
	 * The destructor for the gpickparser class.
	 */
	~GPickParser();

	/**
	 * \brief global pick parsing function
	 *
	 * An overridden function (from Parser) used to parse a given global
	 * pick string (from a file, broker, or other source) into a
	 * SuperEasyJSON object (the common data interchange object) which is
	 * returned as a shared_ptr for  use in other neic-glass3 functions and
	 * classes (such as glasscore).
	 *
	 * \param input - The global pick formatted std::string to parse
	 * \return Returns a shared_ptr to the json::Object containing
	 * the data.
	 */
	std::shared_ptr<json::Object> parse(const std::string &input) override;
};
}  // namespace parse
}  // namespace glass3
#endif  // GPICKPARSER_H
