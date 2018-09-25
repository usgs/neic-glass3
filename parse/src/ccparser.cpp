#include <ccparser.h>
#include <json.h>
#include <logger.h>
#include <date.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <vector>
#include <memory>

/* The correlation  message is space delimited and has the following format:
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
 */
#define ORIGINDATE_INDEX 0
#define ORIGINTIME_INDEX 1
#define LATITUDE_INDEX 2
#define LONGITUDE_INDEX 3
#define DEPTH_INDEX 4
#define MAGNITUDE_INDEX 5
#define MAGNITUDETYPE_INDEX 6
#define NETWORK_INDEX 7
#define STATION_INDEX 8
#define CHANNEL_INDEX 9
#define LOCATION_INDEX 10
#define PHASETYPE_INDEX 11
#define ARRIVALDATE_INDEX 12
#define ARRIVALTIME_INDEX 13
#define CORRELATION_INDEX 14
#define CORRELATIONTHRESHOLD_INDEX 15
#define CC_MSG_MAX_INDEX 15

namespace glass3 {
namespace parse {
// ---------------------------------------------------------------------CCParser
CCParser::CCParser(const std::string &defaultAgencyID,
					const std::string &defaultAuthor)
		: glass3::parse::Parser::Parser(defaultAgencyID, defaultAuthor) {
}

// --------------------------------------------------------------------~CCParser
CCParser::~CCParser() {
}

// -----------------------------------------------------------------------parse
std::shared_ptr<json::Object> CCParser::parse(const std::string &input) {
	// make sure we got something
	if (input.length() == 0) {
		return (NULL);
	}

	glass3::util::Logger::log(
			"trace", "ccparser::parse: Input String: " + input + ".");

	try {
		// split the ccpick, the format is space delimited
		std::vector<std::string> splitInput = glass3::util::split(input, ' ');

		// make sure we split the response into at
		// least as many elements as we need (16 since the pick is on the
		// end of the message)
		if (splitInput.size() < CC_MSG_MAX_INDEX) {
			glass3::util::Logger::log(
					"error",
					"ccparser::parse: ccpick did not split into at least 15 "
					"elements, returning.");
			return (NULL);
		}

		// create the new correlation
		// build the json correlation object
		detectionformats::correlation newCorrelation;

		// Make up a PID based on the type of pick (cc) + SCNL + the pick time.
		// I *think* this is unique enough...
		newCorrelation.id = "CC" + splitInput[STATION_INDEX]
				+ splitInput[CHANNEL_INDEX] + splitInput[NETWORK_INDEX]
				+ splitInput[LOCATION_INDEX]
				+ glass3::util::removeChars(splitInput[ARRIVALDATE_INDEX], "/")
				+ glass3::util::removeChars(
						glass3::util::removeChars(splitInput[ARRIVALTIME_INDEX],
													":"),
						".");

		// build the site object
		newCorrelation.site.station = splitInput[STATION_INDEX];
		newCorrelation.site.channel = splitInput[CHANNEL_INDEX];
		newCorrelation.site.network = splitInput[NETWORK_INDEX];
		newCorrelation.site.location = splitInput[LOCATION_INDEX];

		// build the source object
		// NOTE: Since the format does not provide this information,
		// use the defaults
		newCorrelation.source.agencyid = getDefaultAgencyId();
		newCorrelation.source.author = getDefaultAuthor();

		// phase
		newCorrelation.phase = splitInput[PHASETYPE_INDEX];

		// convert the phase time into epoch time
		// if you remove the spaces, '/', and ':' you get "DateTime"
		newCorrelation.time = glass3::util::Date::convertDateTimeToEpochTime(
				glass3::util::removeChars(splitInput[ARRIVALDATE_INDEX], "/")
						+ glass3::util::removeChars(
								splitInput[ARRIVALTIME_INDEX], ":"));

		// correlation
		newCorrelation.correlationvalue = std::stod(
				splitInput[CORRELATION_INDEX]);

		// latitude
		newCorrelation.hypocenter.latitude = std::stod(
				splitInput[LATITUDE_INDEX]);

		// longitude
		newCorrelation.hypocenter.longitude = std::stod(
				splitInput[LONGITUDE_INDEX]);

		// convert the origin time into epoch time
		// if you remove the spaces, '/', and ':' you get "DateTime"
		newCorrelation.hypocenter.time =
				glass3::util::Date::convertDateTimeToEpochTime(
						glass3::util::removeChars(splitInput[ORIGINDATE_INDEX],
													"/")
								+ glass3::util::removeChars(
										splitInput[ORIGINTIME_INDEX], ":"));

		// depth
		newCorrelation.hypocenter.depth = std::stod(splitInput[DEPTH_INDEX]);

		// event type is not specified, default to earthquake
		newCorrelation.eventtype = "earthquake";

		// magnitude
		newCorrelation.magnitude = std::stod(splitInput[MAGNITUDE_INDEX]);

		// validate
		if (newCorrelation.isvalid() == false) {
			glass3::util::Logger::log(
					"warning", "ccparser::parse: Correlation invalid.");
			return (NULL);
		}

		// convert to our json implementation.
		rapidjson::Document correlationdocument;
		std::string correlationstring = detectionformats::ToJSONString(
				newCorrelation.tojson(correlationdocument,
										correlationdocument.GetAllocator()));

		// and then back into a SuperEasyJSON value object
		json::Value deserializedJSON = json::Deserialize(correlationstring);

		// make sure we got a valid json value object
		if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
			// create a shared pointer to the JSON object
			std::shared_ptr<json::Object> newjsoncorrelation = std::make_shared<
					json::Object>(json::Object(deserializedJSON.ToObject()));

			glass3::util::Logger::log(
					"trace",
					"ccparser::parse: Output JSON: "
							+ json::Serialize(*newjsoncorrelation) + ".");

			return (newjsoncorrelation);
		}
	} catch (const std::exception &) {
		glass3::util::Logger::log(
				"warning",
				"ccparser::parse: exception parsing correlation data.");
	}

	return (NULL);
}
}  // namespace parse
}  // namespace glass3
