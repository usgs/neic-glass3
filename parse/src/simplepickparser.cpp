#include <simplepickparser.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <date.h>
#include <detection-formats.h>
#include <string>
#include <vector>
#include <memory>

/*
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

 * Since the global pick format does not include source agency information, the
 * default agency ID will be used to provide source attribution combined with
 * the author/logo.
 *
 * The phase can be defined as P or S, or left blank for unknown.
 */

#define PICKID_INDEX 0
#define NETWORK_INDEX 1
#define STATION_INDEX 2
#define CHANNEL_INDEX 3
#define LOCATION_INDEX 4
#define ARRIVALTIME_INDEX 5
#define PHASETYPE_INDEX 6
#define SIMPLEPICK_MSG_MAX_INDEX 6

namespace glass3 {
namespace parse {
// ------------------------------------------------------------------SimplePickParser
SimplePickParser::SimplePickParser(const std::string &defaultAgencyID,
									const std::string &defaultAuthor)
		: glass3::parse::Parser::Parser(defaultAgencyID, defaultAuthor) {
}

// -----------------------------------------------------------------~SimplePickParser
SimplePickParser::~SimplePickParser() {
}

// ------------------------------------------------------------------------parse
std::shared_ptr<json::Object> SimplePickParser::parse(const std::string &input) {
	// make sure we got something
	if (input.length() == 0)
		return (NULL);

	glass3::util::Logger::log("trace",
						"simplepickparser::parse: Input String: " + input + ".");

	try {
		// split the simplepick, the simplepick is space delimited
		std::vector<std::string> splitInput = glass3::util::split(input, ' ');

		// make sure we split the response into at
		// least as many elements as we need
		if (splitInput.size() < SIMPLEPICK_MSG_MAX_INDEX ) {
			glass3::util::Logger::log(
					"error",
					"simplepickparser::parse: Provided input did not split into at "
							"least the 6 elements needed for a simple pick "
							"(split into " + std::to_string(splitInput.size())
							+ ") , returning.");
			return (NULL);
		}

		// create the new pick
		// build the json pick object
		detectionformats::pick newPick;
		newPick.id = splitInput[PICKID_INDEX];

		// build the site object
		newPick.site.network = splitInput[NETWORK_INDEX];
		newPick.site.station = splitInput[STATION_INDEX];
		newPick.site.channel = splitInput[CHANNEL_INDEX];
		newPick.site.location = splitInput[LOCATION_INDEX];

		// load time in epoch
		newPick.time =  std::stod(splitInput[ARRIVALTIME_INDEX]);

		// make the author and agency blank
		newPick.source.agencyid = "None";
		newPick.source.author = "None";

		// if phase is set, define in as classified
		if(splitInput.size() >= SIMPLEPICK_MSG_MAX_INDEX+1) {
				newPick.phase = splitInput[PHASETYPE_INDEX];
				newPick.classificationinfo.phase = splitInput[PHASETYPE_INDEX];
				newPick.classificationinfo.phaseprobability = 1.0;
		}

		// validate
		if (newPick.isvalid() == false) {
			glass3::util::Logger::log("warning", "simplepickparser::parse: Pick invalid.");
			return (NULL);
		}

		// since detection formats and glass3 use different json libraries,
		// take the parsed and validated message back out to a string
		rapidjson::Document pickdocument;
		std::string pickstring = detectionformats::ToJSONString(
				newPick.tojson(pickdocument, pickdocument.GetAllocator()));

		// and then back into a SuperEasyJSON value object
		json::Value deserializedJSON = json::Deserialize(pickstring);

		// make sure we got a valid json value object
		if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
			// create a shared pointer to the JSON object
			std::shared_ptr<json::Object> newjsonpick = std::make_shared<
					json::Object>(json::Object(deserializedJSON.ToObject()));

			glass3::util::Logger::log(
					"trace",
					"simplepickparser::parse: Output JSON: "
							+ json::Serialize(*newjsonpick) + ".");

			return (newjsonpick);
		}
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"warning",
				"simplepickparser::parse: Exception parsing global pick: "
						+ std::string(e.what()));
	}

	return (NULL);
}
}  // namespace parse
}  // namespace glass3
