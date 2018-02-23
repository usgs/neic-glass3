#include <ccparser.h>
#include <json.h>
#include <logger.h>
#include <timeutil.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <vector>
#include <memory>

namespace parse {
CCParser::CCParser(const std::string &newAgencyID, const std::string &newAuthor)
		: Parser::Parser(newAgencyID, newAuthor) {
}
CCParser::~CCParser() {
}

// parse a json object from an input string
std::shared_ptr<json::Object> CCParser::parse(const std::string &input) {
	// make sure we got something
	if (input.length() == 0)
		return (NULL);

	logger::log("trace", "ccparser::parse: Input String: " + input + ".");

	// cc pick  format
	// 2015/03/23 07:36:32.880 36.769 -98.019 5.0 2.6136482 mblg GS OK032 HHZ 00 P 2015/03/23 07:36:36.100 0.6581729 0.65 // NOLINT
	//
	// indexes 0-1 is the origin time, need
	// index 2 is the lat, need
	// index 3 is the lon, need
	// index 4 is the depth, need
	// indexes 5-6 is the magnitude, need
	// indexes 7-10 is the NSCL, need
	// index 11 is the phase, need
	// indexes 12-13 is the arrival time, need
	// index 14 is the correlation value, need
	// index 15, ignore

	try {
		// split the ccpick, the gpick is space delimited
		std::vector<std::string> splitccpick = util::split(input, ' ');

		// make sure we split the response into at
		// least as many elements as we need (16 since the pick is on the
		// end of the message)
		if (splitccpick.size() < 15) {
			logger::log(
					"error",
					"ccparser::parse: ccpick did not split into at least 15 "
					"elements, returning.");
			return (NULL);
		}

		// parse out the phase date and time first
		std::string phasedate = util::removeChars(splitccpick[12], "/");
		std::string phasetime = util::removeChars(splitccpick[13], ":");

		// create the new correlation
		// build the json correlation object
		detectionformats::correlation newcorrelation;

		// Make up a PID based on the type of pick (cc) + SCNL + the pick time.
		// I *think* this is unique enough...
		newcorrelation.id = "CC" + splitccpick[8] + splitccpick[9]
				+ splitccpick[7] + splitccpick[10]
				+ util::removeChars(splitccpick[12], "/")
				+ util::removeChars(util::removeChars(splitccpick[13], ":"),
									".");

		// build the site object
		newcorrelation.site.station = splitccpick[8];
		newcorrelation.site.channel = splitccpick[9];
		newcorrelation.site.network = splitccpick[7];
		newcorrelation.site.location = splitccpick[10];

		// build the source object
		// need to think more about this one
		newcorrelation.source.agencyid = m_AgencyID;
		newcorrelation.source.author = m_Author;

		// phase
		newcorrelation.phase = splitccpick[11];

		// convert the phase time into epoch time
		// if you remove the spaces, '/', and ':' you get "DateTime"
		newcorrelation.time = util::convertDateTimeToEpochTime(
				phasedate + phasetime);

		// correlation
		newcorrelation.correlationvalue = std::stod(splitccpick[14]);

		// latitude
		newcorrelation.hypocenter.latitude = std::stod(splitccpick[2]);

		// longitude
		newcorrelation.hypocenter.longitude = std::stod(splitccpick[3]);

		// convert the origin time into epoch time
		// if you remove the spaces, '/', and ':' you get "DateTime"
		newcorrelation.hypocenter.time = util::convertDateTimeToEpochTime(
				util::removeChars(splitccpick[0], "/")
						+ util::removeChars(splitccpick[1], ":"));

		// depth
		newcorrelation.hypocenter.depth = std::stod(splitccpick[4]);

		// event type is not specified, default to earthquake
		newcorrelation.eventtype = "earthquake";

		// magnitude
		newcorrelation.magnitude = std::stod(splitccpick[5]);

		// convert to our json implementation.
		rapidjson::Document correlationdocument;
		std::string correlationstring = detectionformats::ToJSONString(
				newcorrelation.tojson(correlationdocument,
										correlationdocument.GetAllocator()));
		json::Value deserializedJSON = json::Deserialize(correlationstring);

		// make sure we got valid json
		if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
			std::shared_ptr<json::Object> newjsoncorrelation = std::make_shared<
								json::Object>(json::Object(deserializedJSON.ToObject()));

			logger::log(
					"trace",
					"ccparser::parse: Output JSON: "
							+ json::Serialize(*newjsoncorrelation) + ".");

			return (newjsoncorrelation);
		}
	} catch (const std::exception &) {
		logger::log("warning", "ccparser::parse: Problem parsing cc pick.");
	}

	return (NULL);
}

// validate a json object
bool CCParser::validate(std::shared_ptr<json::Object> input) {
	// nullcheck
	if (input == NULL) {
		return (false);
	}

	// convert to detectionformats::correlation
	std::string correlationstring = json::Serialize(*input);
	rapidjson::Document correlationdocument;
	detectionformats::correlation correlationobject(
			detectionformats::FromJSONString(correlationstring,
												correlationdocument));

	// let detection formats validate
	return (correlationobject.isvalid());
}
}  // namespace parse
