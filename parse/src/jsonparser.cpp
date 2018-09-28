#include <jsonparser.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <vector>
#include <memory>

namespace glass3 {
namespace parse {
// -------------------------------------------------------------------JSONParser
JSONParser::JSONParser(const std::string &defaultAgencyID,
						const std::string &defaultAuthor)
		: glass3::parse::Parser::Parser(defaultAgencyID, defaultAuthor) {
}

// ------------------------------------------------------------------~JSONParser
JSONParser::~JSONParser() {
}

// ------------------------------------------------------------------------parse
std::shared_ptr<json::Object> JSONParser::parse(const std::string &input) {
	// make sure we got something
	if (input.length() == 0) {
		return (NULL);
	}

	glass3::util::Logger::log("trace",
						"jsonparser::parse: Input String: " + input + ".");

	// Note: Detection formats does not provide generic parsing or validation
	// functions. This makes this function more awkward than it could be.
	rapidjson::Document detectionDocument;
	std::string jsonString = "";

	// get the message type from detection formats
	int messageType = detectionformats::GetDetectionType(input);

	// use the appropriate detection formats parser and validator based on
	// the type
	if (messageType == detectionformats::picktype) {
		// convert to detectionformats::pick
		detectionformats::pick newPick(
				detectionformats::FromJSONString(input, detectionDocument));

		// let detection formats validate
		if (newPick.isvalid() == false) {
			glass3::util::Logger::log("warning", "JSONParser::parse: Pick invalid.");
			return (NULL);
		}

		// since detection formats and glass3 use different json libraries,
		// take the parsed and validated message back out to a string
		jsonString = detectionformats::ToJSONString(
				newPick.tojson(detectionDocument,
								detectionDocument.GetAllocator()));
	} else if (messageType == detectionformats::correlationtype) {
		// convert to detectionformats::correlation
		detectionformats::correlation newCorrelation(
				detectionformats::FromJSONString(input, detectionDocument));

		// let detection formats validate
		if (newCorrelation.isvalid() == false) {
			glass3::util::Logger::log("warning",
								"JSONParser::parse: Correlation invalid.");
			return (NULL);
		}

		// since detection formats and glass3 use different json libraries,
		// take the parsed and validated message back out to a string
		jsonString = detectionformats::ToJSONString(
				newCorrelation.tojson(detectionDocument,
										detectionDocument.GetAllocator()));
	} else if (messageType == detectionformats::detectiontype) {
		// convert to detectionformats::detection
		detectionformats::detection newDetection(
				detectionformats::FromJSONString(input, detectionDocument));

		// let detection formats validate
		if (newDetection.isvalid() == false) {
			glass3::util::Logger::log("warning",
								"JSONParser::parse: Detection invalid.");
			return (NULL);
		}

		// since detection formats and glass3 use different json libraries,
		// take the parsed and validated message back out to a string
		jsonString = detectionformats::ToJSONString(
				newDetection.tojson(detectionDocument,
									detectionDocument.GetAllocator()));
	} else if (messageType == detectionformats::stationinfotype) {
		// convert to detectionformats::stationInfo
		detectionformats::stationInfo newStation(
				detectionformats::FromJSONString(input, detectionDocument));

		// let detection formats validate
		if (newStation.isvalid() == false) {
			glass3::util::Logger::log("warning", "JSONParser::parse: Station invalid.");
			return (NULL);
		}

		// for stationInfo, we have an additional validation step
		// if present, we need to check to see if our glass instance
		// requested this information. Otherwise glass would try to add this
		// station even if it was not relevant
		if (newStation.informationRequestor.isempty() == false) {
			if ((newStation.informationRequestor.agencyid
					!= getDefaultAgencyId())
					|| (newStation.informationRequestor.author
							!= getDefaultAuthor())) {
				glass3::util::Logger::log(
						"debug", "jsonparser::parse: Station is not for this "
						"instance by agencyID and author.");
				return (NULL);
			}
		}

		// since detection formats and glass3 use different json libraries,
		// take the parsed and validated message back out to a string
		jsonString = detectionformats::ToJSONString(
				newStation.tojson(detectionDocument,
									detectionDocument.GetAllocator()));
	} else {
		// we don't recognize this type
		return (NULL);
	}

	// convert the string to glass3's json library (SuperEasyJSON)
	try {
		// Convert into a SuperEasyJSON value object
		json::Value deserializedValue = json::Deserialize(jsonString);

		// make sure we got a valid json value object
		if (deserializedValue.GetType() != json::ValueType::NULLVal) {
			// create a shared pointer to the JSON object
			std::shared_ptr<json::Object> newObject = std::make_shared<
					json::Object>(json::Object(deserializedValue.ToObject()));

			glass3::util::Logger::log(
					"trace",
					"JSONParser::parse: Output JSON: "
							+ json::Serialize(*newObject) + ".");

			return (newObject);
		}
	} catch (const std::runtime_error &e) {
		// oopse
		std::string exceptionstring = e.what();
		glass3::util::Logger::log(
				"error",
				"jsonparser::parse: json::Deserialize encountered error "
						+ exceptionstring + ", returning.");
	}

	return (NULL);
}
}  // namespace parse
}  // namespace glass3
