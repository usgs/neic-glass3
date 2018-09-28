#include <gpickparser.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <date.h>
#include <detection-formats.h>
#include <string>
#include <vector>
#include <memory>

/*
 * The global pick message has the following format:
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
 */
#define LOGO_INDEX 0
#define PICKID_INDEX 1
#define MESSAGEVERSION_INDEX 2
#define STATION_INDEX 3
#define CHANNEL_INDEX 4
#define NETWORK_INDEX 5
#define LOCATION_INDEX 6
#define ARRIVALTIME_INDEX 7
#define PHASETYPE_INDEX 8
#define ERRORWINDOW_INDEX 9
#define POLARITY_INDEX 10
#define ONSET_INDEX 11
#define PICKERTYPE_INDEX 12
#define HIGHPASS_INDEX 13
#define LOWPASS_INDEX 14
#define BACKAZIMUTH_INDEX 15
#define SLOWNESS_INDEX 16
#define SNR_INDEX 17
#define AMPLITUDE_INDEX 18
#define PERIOD_INDEX 19
#define GPICK_MSG_MAX_INDEX 20

namespace glass3 {
namespace parse {
// ------------------------------------------------------------------GPickParser
GPickParser::GPickParser(const std::string &defaultAgencyID,
							const std::string &defaultAuthor)
		: glass3::parse::Parser::Parser(defaultAgencyID, defaultAuthor) {
}

// -----------------------------------------------------------------~GPickParser
GPickParser::~GPickParser() {
}

// ------------------------------------------------------------------------parse
std::shared_ptr<json::Object> GPickParser::parse(const std::string &input) {
	// make sure we got something
	if (input.length() == 0)
		return (NULL);

	glass3::util::Logger::log("trace",
						"gpickparser::parse: Input String: " + input + ".");

	try {
		// split the gpick, the gpick is space delimited
		std::vector<std::string> splitInput = glass3::util::split(input, ' ');

		// make sure we split the response into at
		// least as many elements as we need
		if (splitInput.size() < GPICK_MSG_MAX_INDEX) {
			glass3::util::Logger::log(
					"error",
					"gpickparser::parse: Provided input did not split into at "
							"least the 20 elements needed for a global pick "
							"(split into " + std::to_string(splitInput.size())
							+ ") , returning.");
			return (NULL);
		}

		// create the new pick
		// build the json pick object
		detectionformats::pick newPick;
		newPick.id = splitInput[PICKID_INDEX];

		// build the site object
		newPick.site.station = splitInput[STATION_INDEX];
		newPick.site.channel = splitInput[CHANNEL_INDEX];
		newPick.site.network = splitInput[NETWORK_INDEX];
		newPick.site.location = splitInput[LOCATION_INDEX];

		// convert the global pick "DateTime" into epoch time
		newPick.time = glass3::util::Date::convertDateTimeToEpochTime(
				splitInput[ARRIVALTIME_INDEX]);

		// build the source object
		// need to think more about this one
		// as far as ew logos are concerned....
		newPick.source.agencyid = getDefaultAgencyId();
		newPick.source.author = splitInput[LOGO_INDEX];

		// phase
		newPick.phase = splitInput[PHASETYPE_INDEX];

		// polarity
		if (splitInput[POLARITY_INDEX] == "U") {
			newPick.polarity =
					detectionformats::polarityvalues[detectionformats::polarityindex::up];  // NOLINT
		} else if (splitInput[POLARITY_INDEX] == "D") {
			newPick.polarity =
					detectionformats::polarityvalues[detectionformats::polarityindex::down];  // NOLINT
		}

		// onset
		if (splitInput[ONSET_INDEX] == "i") {
			newPick.onset =
					detectionformats::onsetvalues[detectionformats::onsetindex::impulsive];  // NOLINT
		} else if (splitInput[ONSET_INDEX] == "e") {
			newPick.onset =
					detectionformats::onsetvalues[detectionformats::onsetindex::emergent];  // NOLINT
		} else if (splitInput[ONSET_INDEX] == "q") {
			newPick.onset =
					detectionformats::onsetvalues[detectionformats::onsetindex::questionable];  // NOLINT
		}

		// type
		if (splitInput[PICKERTYPE_INDEX] == "m") {
			newPick.picker =
					detectionformats::pickervalues[detectionformats::pickerindex::manual];  // NOLINT
		} else if (splitInput[PICKERTYPE_INDEX] == "r") {
			newPick.picker =
					detectionformats::pickervalues[detectionformats::pickerindex::raypicker];  // NOLINT
		} else if (splitInput[PICKERTYPE_INDEX] == "l") {
			newPick.picker =
					detectionformats::pickervalues[detectionformats::pickerindex::filterpicker];  // NOLINT
		} else if (splitInput[PICKERTYPE_INDEX] == "e") {
			newPick.picker =
					detectionformats::pickervalues[detectionformats::pickerindex::earthworm];  // NOLINT
		} else if (splitInput[PICKERTYPE_INDEX] == "U") {
			newPick.picker =
					detectionformats::pickervalues[detectionformats::pickerindex::other];  // NOLINT
		}

		// convert Filter values to a json object
		double HighPass = -1.0;
		double LowPass = -1.0;
		try {
			HighPass = std::stod(splitInput[HIGHPASS_INDEX]);
			LowPass = std::stod(splitInput[LOWPASS_INDEX]);
		} catch (const std::exception &) {
			glass3::util::Logger::log(
					"warning",
					"gpickparser::parse: Problem converting optional filter "
					"values to doubles.");
		}

		// make sure we got some sort of valid numbers
		if ((HighPass != -1.0) && (LowPass != -1.0)) {
			detectionformats::filter filterobject;
			filterobject.highpass = HighPass;
			filterobject.lowpass = LowPass;
			newPick.filterdata.push_back(filterobject);
		}

		// convert Beam values to a json object
		double BackAzimuth = -1.0;
		double Slowness = -1.0;
		try {
			BackAzimuth = std::stod(splitInput[BACKAZIMUTH_INDEX]);
			Slowness = std::stod(splitInput[SLOWNESS_INDEX]);
		} catch (const std::exception &) {
			glass3::util::Logger::log(
					"warning",
					"gpickparser::parse: Problem converting optional beam "
					"values to doubles.");
		}

		// make sure we got some sort of valid numbers
		// NOTE: This is a little sketchy, since a 0 backazimuth is valid,
		// but gpick doesn't seem to have backazimuth and slowness populated
		// (always 0) this is where we are....
		if ((BackAzimuth > 0) && (Slowness > 0)) {
			newPick.beam.backazimuth = BackAzimuth;
			newPick.beam.slowness = Slowness;
		}

		// convert amplitude values to a json object
		double Amplitude = -1.0;
		double Period = -1.0;
		double SNR = -1.0;
		try {
			Amplitude = std::stod(splitInput[AMPLITUDE_INDEX]);
			Period = std::stod(splitInput[PERIOD_INDEX]);
			SNR = std::stod(splitInput[SNR_INDEX]);
		} catch (const std::exception &) {
			glass3::util::Logger::log(
					"warning",
					"gpickparser::parse: Problem converting optional amplitude "
					"values to doubles.");
		}

		// make sure we got some sort of valid numbers
		if ((Amplitude != -1.0) && (Period != -1.0) && (SNR != -1.0)) {
			// create amplitude object
			newPick.amplitude.ampvalue = Amplitude;
			newPick.amplitude.period = Period;
			newPick.amplitude.snr = SNR;
		}

		// validate
		if (newPick.isvalid() == false) {
			glass3::util::Logger::log("warning", "gpickparser::parse: Pick invalid.");
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
					"gpickparser::parse: Output JSON: "
							+ json::Serialize(*newjsonpick) + ".");

			return (newjsonpick);
		}
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"warning",
				"gpickparser::parse: Exception parsing global pick: "
						+ std::string(e.what()));
	}

	return (NULL);
}
}  // namespace parse
}  // namespace glass3
