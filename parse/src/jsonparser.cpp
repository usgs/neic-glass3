#include <jsonparser.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <vector>

namespace parse {
JSONParser::JSONParser(std::string newAgencyID, std::string newAuthor)
		: Parser::Parser(newAgencyID, newAuthor) {
}
JSONParser::~JSONParser() {
}

// parse a json object from an input string
json::Object* JSONParser::parse(std::string input) {
	// make sure we got something
	if (input.length() == 0) {
		return (NULL);
	}

	logger::log("trace", "jsonparser::parse: Input String: " + input + ".");

	// JSON format
	// JSON doesn't have a fixed format
	// try to convert the string to a json object
	json::Object *newobject = NULL;
	try {
		json::Value deserializedvalue = json::Deserialize(input);

		// make sure we got valid json
		if (deserializedvalue.GetType() == json::ValueType::NULLVal) {
			logger::log("error",
						"jsonparser::parse: json::Deserialize returned null, "
						"returning.");
			return (NULL);
		}

		// convert our resulting value to a json object
		newobject = new json::Object(deserializedvalue.ToObject());
	} catch (const std::runtime_error &e) {
		// oopse
		std::string exceptionstring = e.what();
		logger::log(
				"error",
				"jsonparser::parse: json::Deserialize encountered error "
						+ exceptionstring + ", returning.");
		return (NULL);
	}

	// make sure Deserialize gave us something
	if (newobject == NULL) {
		logger::log("error",
					"jsonparser::parse: json::Deserialize returned null, "
					"returning.");
		return (NULL);
	}

	logger::log(
			"trace",
			"jsonparser::parse: Output JSON: " + json::Serialize(*newobject)
					+ ".");

	return (newobject);
}

// validate a json object
bool JSONParser::validate(json::Object* input) {
	// nullcheck
	if (input == NULL) {
		return (false);
	}

	// convert to string
	std::string inputstring = json::Serialize(*input);

	// validate based on the provided type
	int type = detectionformats::GetDetectionType(inputstring);

	// pick
	if (type == detectionformats::picktype) {
		// convert to detectionformats::pick
		rapidjson::Document pickdocument;
		detectionformats::pick pickobject(
				detectionformats::FromJSONString(inputstring, pickdocument));

		// let detection formats validate
		bool result = pickobject.isvalid();

		if (result == false) {
			std::vector<std::string> errors = pickobject.geterrors();
			std::string errorstring = "";

			for (int i = 0; i < errors.size(); i++) {
				errorstring += errors[i] + " ";
			}

			logger::log(
					"error",
					"jsonparser::validate: Pick validation errors:"
							+ errorstring + " for input:" + inputstring);
		}

		return (result);
	} else if (type == detectionformats::correlationtype) {
		// correlation
		// convert to detectionformats::correlation
		rapidjson::Document correlationdocument;
		detectionformats::correlation correlationobject(
				detectionformats::FromJSONString(inputstring,
													correlationdocument));

		// let detection formats validate
		bool result = correlationobject.isvalid();

		if (result == false) {
			std::vector<std::string> errors = correlationobject.geterrors();
			std::string errorstring = "";

			for (int i = 0; i < errors.size(); i++) {
				errorstring += errors[i] + " ";
			}

			logger::log(
					"error",
					"jsonparser::validate: Correlation validation errors:"
							+ errorstring + " for input:" + inputstring);
		}

		return (result);
	} else if (type == detectionformats::detectiontype) {
		// detection
		// convert to detectionformats::detection
		rapidjson::Document detectiondocument;
		detectionformats::detection detectionobject(
				detectionformats::FromJSONString(inputstring,
													detectiondocument));

		// let detection formats validate
		bool result = detectionobject.isvalid();

		if (result == false) {
			std::vector<std::string> errors = detectionobject.geterrors();
			std::string errorstring = "";

			for (int i = 0; i < errors.size(); i++) {
				errorstring += errors[i] + " ";
			}

			logger::log(
					"error",
					"jsonparser::validate: detection validation errors:"
							+ errorstring + " for input:" + inputstring);
		}

		return (result);
	} else if (type == detectionformats::stationinfotype) {
		// stationInfo
		// convert to detectionformats::stationInfo
		rapidjson::Document stationdocument;
		detectionformats::stationInfo stationObject(
				detectionformats::FromJSONString(inputstring, stationdocument));

		// let detection formats validate
		bool result = stationObject.isvalid();

		if (result == false) {
			std::vector<std::string> errors = stationObject.geterrors();
			std::string errorstring = "";

			for (int i = 0; i < errors.size(); i++) {
				errorstring += errors[i] + " ";
			}

			logger::log(
					"error",
					"jsonparser::validate: stationInfo validation errors:"
							+ errorstring + " for input:" + inputstring);
		} else {
			// check information requestor if it's present
			if (stationObject.informationRequestor.isempty() == false) {
				if ((stationObject.informationRequestor.agencyid != m_AgencyID)
						|| (stationObject.informationRequestor.author != m_Author)) {
					logger::log(
							"debug",
							"jsonparser::validate: stationInfo is not for this "
							"glass by agencyID and author.");
					result = false;
				}
			}
		}

		return (result);
	}

	// unknown type
	return (false);
}
}  // namespace parse
