/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CONVERT_H
#define CONVERT_H

#include <json.h>
#include <string>

namespace parse {

/**
 * \brief json hypo conversion function
 *
 * The function is used to convert a hypo message to the json detection format
 *
 * \param data - A pointer to a json::Object containing the data to be
 * converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted json detection, empty
 * string otherwise
 */
std::string hypoToJSONDetection(json::Object *data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor);

/**
 * \brief json cancel conversion function
 *
 * The function is used to convert a cancel message to the json retraction
 * format
 *
 * \param data - A pointer to a json::Object containing the data to be
 * converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted json detection, empty
 * string otherwise
 */
std::string cancelToJSONRetract(json::Object *data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor);

/**
 * \brief json station list conversion function
 *
 * The function is used to convert a site list message to the json station list
 * format
 *
 * \param data - A pointer to a json::Object containing the data to be
 * converted.
 * \return Returns a string containing the converted json detection, empty
 * string otherwise
 */
std::string siteListToStationList(json::Object *data);

/**
 * \brief json station list conversion function
 *
 * The function is used to convert a site lookup message to the json station
 * info request format
 *
 * \param data - A pointer to a json::Object containing the data to be
 * converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted json detection, empty
 * string otherwise
 */
std::string siteLookupToStationInfoRequest(json::Object *data,
											const std::string &outputAgencyID,
											const std::string &outputAuthor);

}  // namespace parse
#endif  // CONVERT_H
