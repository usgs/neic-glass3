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
#include <memory>

namespace glass3 {
namespace parse {

/**
 * \brief json hypo conversion function
 *
 * The function is used to convert a glasscore Hypo message to a valid detection
 * formats Detection message
 *
 * The glasscore cancel message is defined at:
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Hypo.md  // NOLINT
 *
 * The detection formats Detection message is defined at:
 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/Detection.md  // NOLINT
 *
 * outputAgencyID and outputAuthor are required since Detection requires it and
 * Hypo does not provide it
 *
 * \param data - A shared_ptr to a json::Object containing the Hypo message
 * to be converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted and validated detection
 * formats Detection message, empty string otherwise
 */
std::string hypoToJSONDetection(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor);

/**
 * \brief json cancel conversion function
 *
 * The function is used to convert a glasscore Cancel message to a valid detection
 * formats Retract message.
 *
 * The glasscore Cancel message is defined at:
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Cancel.md  // NOLINT
 *
 * The detection formats Retract message is defined at:
 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/Retract.md  // NOLINT
 *
 * outputAgencyID and outputAuthor are required since Retract requires it and
 * Cancel does not provide it
 *
 * \param data - A shared_ptr to a json::Object containing the Cancel message
 * to be converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted and validated detection
 * formats Cancel message, empty string otherwise
 */
std::string cancelToJSONRetract(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor);

/**
 * \brief json station list conversion function
 *
 * The function is used to convert a glasscore SiteList message to a
 * StationInfoList messages
 *
 * The glasscore SiteList message is defined at:
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteList.md  // NOLINT
 *
 * The glasscore station info list is defined at:
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/StationInfoList.md  // NOLINT/ NOLINT
 *
 * \param data - A pointer to a json::Object containing the SiteList to be
 * converted.
 * \return Returns a string containing an array of converted and validated
 * detection formats StationInfo objects, as a StationInfoList message, empty
 * string otherwise
 */
std::string siteListToStationList(std::shared_ptr<json::Object> data);

/**
 * \brief json station list conversion function
 *
 * The function is used to convert a glasscore SiteLookup message to a valid
 * detection formats StationInfoRequest message.
 *
 * The glasscore SiteLookup message is defined at:
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteLookup.md  // NOLINT
 *
 * The detection formats StationInfoRequest message is defined at:
 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/StationInfoRequest.md  // NOLINT
 *
 * outputAgencyID and outputAuthor are required since StationInfoRequest requires
 * it and SiteLookup does not provide it
 *
 * \param data - A shared_ptr to a json::Object containing the SiteLookup message
 * to be converted.
 * \param outputAgencyID - A std::string containing the agency id to use for
 * output
 * \param outputAuthor - A std::string containing the author to use for output
 * \return Returns a string containing the converted and validated detection
 * formats StationInfoRequest message, empty string otherwise
 */
std::string siteLookupToStationInfoRequest(std::shared_ptr<json::Object> data,
											const std::string &outputAgencyID,
											const std::string &outputAuthor);
}  // namespace parse
}  // namespace glass3
#endif  // CONVERT_H
