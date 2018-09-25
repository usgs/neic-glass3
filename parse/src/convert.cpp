#include <convert.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <ctime>
#include <vector>
#include <memory>

// JSON Keys
#define TYPE_KEY "Type"
#define STATIONLIST_KEY "StationList"

namespace glass3 {
namespace parse {

// ---------------------------------------------------------hypoToJSONDetection
std::string hypoToJSONDetection(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor) {
	/**
	 * The glasscore cancel message is defined at:
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Hypo.md  // NOLINT
	 *
	 * The glasscore station info list is defined at:
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/StationInfoList.md  // NOLINT
	 */

	// nullcheck
	if (data == NULL) {
		glass3::util::Logger::log(
				"error",
				"hypoToJSONDetection(): Null json data object passed in.");
		return ("");
	}

	// type check
	if (!(data->HasKey("Type"))) {
		glass3::util::Logger::log(
				"error",
				"hypoToJSONDetection(): Bad json data object passed in.");
		return ("");
	}
	if ((*data)["Type"].ToString() != "Hypo") {
		return ("");
	}

	// some messages have IDs, others have Pids
	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		glass3::util::Logger::log(
				"error",
				"hypoToJSONDetection(): Bad json data object passed in, no "
				"ID.");
		return ("");
	}

	glass3::util::Logger::log(
			"info",
			"hypoToJSONDetection(): Converting a hypo message with id: " + ID
					+ ".");

	glass3::util::Logger::log(
			"debug",
			"hypoToJSONDetection(): data = |" + json::Serialize(*data) + "|.");

	// what time is it now
	time_t tNow;
	std::time(&tNow);

	std::string outputString = "";

	// build detection message
	detectionformats::detection detection;

	// since detection formats and glass3 use different json libraries,
	// need to take the values out of the SuperEasyJSON object and add them
	// to the detection formats object
	try {
		// required values
		detection.id = ID;
		detection.source.agencyid = outputAgencyID;
		detection.source.author = outputAuthor;
		detection.hypocenter.latitude = (*data)["Latitude"];
		detection.hypocenter.longitude = (*data)["Longitude"];
		detection.hypocenter.time = detectionformats::ConvertISO8601ToEpochTime(
				(*data)["Time"]);
		detection.hypocenter.depth = (*data)["Depth"];

		if (data->HasKey("IsUpdate") == true) {
			bool isUpdate = (*data)["IsUpdate"].ToBool();

			if (isUpdate == false) {
				detection.detectiontype =
						detectionformats::detectiontypevalues[detectionformats::detectiontypeindex::newdetection];  // NOLINT
			} else {
				detection.detectiontype =
						detectionformats::detectiontypevalues[detectionformats::detectiontypeindex::update];  // NOLINT
			}
		} else {
			detection.detectiontype =
					detectionformats::detectiontypevalues[detectionformats::detectiontypeindex::newdetection];  // NOLINT
		}

		// optional values that we have
		detection.minimumdistance = (*data)["MinimumDistance"];
		detection.gap = (*data)["Gap"];
		detection.bayes = (*data)["Bayes"];
		detection.detectiontime = tNow;

		// optional data
		std::vector<detectionformats::pick> pickdata;
		detection.pickdata.clear();

		std::vector<detectionformats::correlation> correlationdata;
		detection.correlationdata.clear();

		json::Array dataarray = (*data)["Data"];
		for (int i = 0; i < dataarray.size(); i++) {
			json::Object dataobject = dataarray[i];

			json::Object siteobj = (dataobject)["Site"].ToObject();
			json::Object sourceobj = (dataobject)["Source"].ToObject();
			json::Object assocobj = (dataobject)["AssociationInfo"].ToObject();

			// add the contributing data
			std::string typestring = (dataobject)["Type"].ToString();
			if (typestring == "Pick") {
				detectionformats::pick pick;

				// base pick
				// required values
				// id
				pick.id = (dataobject)["ID"].ToString();

				// station and network are required
				pick.site.station = (siteobj)["Station"].ToString();
				pick.site.network = (siteobj)["Network"].ToString();

				// channel and location are optional
				if (siteobj.HasKey("Channel")) {
					pick.site.channel = (siteobj)["Channel"].ToString();
				}
				if (siteobj.HasKey("Location")) {
					pick.site.location = (siteobj)["Location"].ToString();
				}

				// source
				pick.source.agencyid = (sourceobj)["AgencyID"].ToString();
				pick.source.author = (sourceobj)["Author"].ToString();

				// time
				pick.time = detectionformats::ConvertISO8601ToEpochTime(
						(dataobject)["Time"]);

				// optional values
				// phase
				if (dataobject.HasKey("Phase")) {
					pick.phase = (dataobject)["Phase"].ToString();
				}

				// picker
				if (dataobject.HasKey("Picker")) {
					pick.picker = (dataobject)["Picker"].ToString();
				}

				// polarity
				if (dataobject.HasKey("Polarity")) {
					pick.polarity = (dataobject)["Polarity"].ToString();
				}

				// onset
				if (dataobject.HasKey("Onset")) {
					pick.onset = (dataobject)["Onset"].ToString();
				}

				// filter
				if (dataobject.HasKey("Filter")) {
					// get the array of filters
					json::Array filterarray = (dataobject)["Filter"].ToArray();

					// for each filter
					for (int filtercount = 0; filtercount < filterarray.size();
							filtercount++) {
						// get the object
						json::Object filterobject = filterarray[filtercount];

						// create the new filter
						detectionformats::filter newfilter;
						newfilter.highpass =
								(filterobject)["HighPass"].ToDouble();
						newfilter.lowpass =
								(filterobject)["LowPass"].ToDouble();

						// add it to the list
						pick.filterdata.push_back(newfilter);
					}
				}

				// amplitude
				if (dataobject.HasKey("Amplitude")) {
					// get the object
					json::Object amplitudeobject = (dataobject)["Amplitude"]
							.ToObject();

					pick.amplitude.ampvalue = (amplitudeobject)["Amplitude"]
							.ToDouble();
					pick.amplitude.period =
							(amplitudeobject)["Period"].ToDouble();
					pick.amplitude.snr = (amplitudeobject)["SNR"].ToDouble();
				}

				// add association info
				std::string phasename = (assocobj)["Phase"].ToString();

				// handle unknown phase "?"
				if (phasename.compare("?") != 0) {
					pick.associationinfo.phase = phasename;
					pick.associationinfo.residual = (assocobj)["Residual"]
							.ToDouble();
					pick.associationinfo.sigma = (assocobj)["Sigma"].ToDouble();
				} else {
					pick.associationinfo.phase = "";
					pick.associationinfo.residual = -1.0;
					pick.associationinfo.sigma = -1.0;
				}
				// handle everything else
				pick.associationinfo.distance =
						(assocobj)["Distance"].ToDouble();
				pick.associationinfo.azimuth = (assocobj)["Azimuth"].ToDouble();

				if (pick.isvalid() == false) {
					std::vector<std::string> errors = pick.geterrors();

					glass3::util::Logger::log("error", "Error validating pick.");
					for (int errorcount = 0;
							errorcount < static_cast<int>(errors.size());
							errorcount++) {
						glass3::util::Logger::log(
								"error",
								"hypoToJSONDetection: Pick Error: "
										+ errors[errorcount]);
					}
				} else {
					pickdata.push_back(pick);
				}
			} else if (typestring == "Correlation") {
				detectionformats::correlation correlation;

				// base correlation
				// required values
				// id
				correlation.id = (dataobject)["ID"].ToString();

				// station and network are required
				correlation.site.station = (siteobj)["Station"].ToString();
				correlation.site.network = (siteobj)["Network"].ToString();

				// channel and location are optional
				if (siteobj.HasKey("Channel")) {
					correlation.site.channel = (siteobj)["Channel"].ToString();
				}
				if (siteobj.HasKey("Location")) {
					correlation.site.location =
							(siteobj)["Location"].ToString();
				}

				// source
				correlation.source.agencyid =
						(sourceobj)["AgencyID"].ToString();
				correlation.source.author = (sourceobj)["Author"].ToString();

				// time
				correlation.time = detectionformats::ConvertISO8601ToEpochTime(
						(dataobject)["Time"]);

				// phase
				correlation.phase = (dataobject)["Phase"].ToString();

				// hypo
				json::Object hypoobj = (dataobject)["Hypocenter"].ToObject();
				correlation.hypocenter.latitude =
						(hypoobj)["Latitude"].ToDouble();
				correlation.hypocenter.longitude = (hypoobj)["Longitude"]
						.ToDouble();
				correlation.hypocenter.depth = (hypoobj)["Depth"].ToDouble();
				correlation.hypocenter.time =
						detectionformats::ConvertISO8601ToEpochTime(
								(hypoobj)["Time"]);

				if (hypoobj.HasKey("LatitudeError")) {
					correlation.hypocenter.latitudeerror =
							(hypoobj)["LatitudeError"].ToDouble();
				}
				if (hypoobj.HasKey("LongitudeError")) {
					correlation.hypocenter.longitudeerror =
							(hypoobj)["LongitudeError"].ToDouble();
				}
				if (hypoobj.HasKey("DepthError")) {
					correlation.hypocenter.deptherror = (hypoobj)["DepthError"]
							.ToDouble();
				}
				if (hypoobj.HasKey("TimeError")) {
					correlation.hypocenter.timeerror = (hypoobj)["TimeError"]
							.ToDouble();
				}

				// correlation value
				correlation.correlationvalue = (dataobject)["Correlation"]
						.ToDouble();

				// optional values
				// type of event
				if (dataobject.HasKey("EventType")) {
					correlation.eventtype =
							(dataobject)["EventType"].ToString();
				}

				// magnitude
				if (dataobject.HasKey("Magnitude")) {
					correlation.magnitude =
							(dataobject)["Magnitude"].ToDouble();
				}

				// snr
				if (dataobject.HasKey("SNR")) {
					correlation.snr = (dataobject)["SNR"].ToDouble();
				}

				// zscore
				if (dataobject.HasKey("ZScore")) {
					correlation.zscore = (dataobject)["ZScore"].ToDouble();
				}

				// detectionthreshold
				if (dataobject.HasKey("DetectionThreshold")) {
					correlation.detectionthreshold =
							(dataobject)["DetectionThreshold"].ToDouble();
				}

				// thresholdtype
				if (dataobject.HasKey("ThresholdType")) {
					correlation.thresholdtype = (dataobject)["ThresholdType"]
							.ToString();
				}

				// add association info
				std::string assocphasename = (assocobj)["Phase"].ToString();

				// handle unknown phase "?"
				if (assocphasename.compare("?") != 0) {
					correlation.associationinfo.phase = assocphasename;
					correlation.associationinfo.residual =
							(assocobj)["Residual"].ToDouble();
					correlation.associationinfo.sigma = (assocobj)["Sigma"]
							.ToDouble();
				} else {
					correlation.associationinfo.phase = "";
					correlation.associationinfo.residual = -1.0;
					correlation.associationinfo.sigma = -1.0;
				}

				// handle everything else
				correlation.associationinfo.distance = (assocobj)["Distance"]
						.ToDouble();
				correlation.associationinfo.azimuth = (assocobj)["Azimuth"]
						.ToDouble();

				if (correlation.isvalid() == false) {
					std::vector<std::string> errors = correlation.geterrors();
					glass3::util::Logger::log(
							"error",
							"hypoToJSONDetection: Error validating correlation.");
					for (int errorcount = 0;
							errorcount < static_cast<int>(errors.size());
							errorcount++) {
						glass3::util::Logger::log(
								"error",
								"hypoToJSONDetection: Correlation Error: "
										+ errors[errorcount]);
					}
				} else {
					correlationdata.push_back(correlation);
				}
			}
		}

		// set the vectors
		detection.pickdata = pickdata;
		detection.correlationdata = correlationdata;

		// check validity
		if (detection.isvalid() == false) {
			return ("");
		}
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"warning",
				"hypoToJSONDetection: Problem building detection message: "
						+ std::string(e.what()));
	}

	// convert to string
	rapidjson::Document detectiondocument;
	outputString = detectionformats::ToJSONString(
			detection.tojson(detectiondocument,
								detectiondocument.GetAllocator()));

	// done
	return (outputString);
}

// ---------------------------------------------------------cancelToJSONRetract
std::string cancelToJSONRetract(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor) {
	/*
	 * The glasscore Cancel message is defined at:
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Cancel.md  // NOLINT
	 *
	 * The detection formats Retract message is defined at:
	 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/Retract.md  // NOLINT
	 */

	// nullcheck
	if (data == NULL) {
		glass3::util::Logger::log(
				"error",
				"cancelToJSONRetract(): Null json data object passed in.");
		return ("");
	}

	// type check
	if (!(data->HasKey("Type"))) {
		glass3::util::Logger::log(
				"error",
				"cancelToJSONRetract(): Bad json data object passed in.");
		return ("");
	}
	if ((*data)["Type"].ToString() != "Cancel") {
		return ("");
	}

	// some messages have IDs, others have Pids
	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		glass3::util::Logger::log(
				"error",
				"hypoToJSONDetection(): Bad json data object passed in, no "
				"ID.");
		return ("");
	}

	glass3::util::Logger::log(
			"info",
			"cancelToJSONRetract(): Converting a cancel message with id: " + ID
					+ ".");

	glass3::util::Logger::log(
			"debug",
			"cancelToJSONRetract(): data = |" + json::Serialize(*data) + "|.");

	// build retract message
	detectionformats::retract retract;

	// since detection formats and glass3 use different json libraries,
	// need to take the values out of the SuperEasyJSON object and add them
	// to the detection formats object
	try {
		// required values
		retract.id = ID;
		retract.source.agencyid = outputAgencyID;
		retract.source.author = outputAuthor;

		// check validity
		if (retract.isvalid() == false) {
			return ("");
		}
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"warning",
				"cancelToJSONRetract: Problem building retract message: "
						+ std::string(e.what()));
	}

	// convert to string
	rapidjson::Document retractdocument;
	std::string outputString = detectionformats::ToJSONString(
			retract.tojson(retractdocument, retractdocument.GetAllocator()));

	// done
	return (outputString);
}

// --------------------------------------------------------siteListToStationList
std::string siteListToStationList(std::shared_ptr<json::Object> data) {
	/*
	 * The glasscore SiteList message is defined at:
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteList.md  // NOLINT
	 *
	 * The detection formats StationInfo message is defined at:
	 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/StationInfo.md  // NOLINT
	 */

	// nullcheck
	if (data == NULL) {
		glass3::util::Logger::log(
				"error",
				"siteListToStationList(): Null json data object passed in.");
		return ("");
	}

	// type check
	if (!(data->HasKey("Cmd"))) {
		glass3::util::Logger::log(
				"error",
				"siteListToStationList(): Bad json data object passed in.");
		return ("");
	}
	if ((*data)["Cmd"].ToString() != "SiteList") {
		glass3::util::Logger::log(
				"error",
				"siteListToStationList(): Wrong json data object passed in.");
		return ("");
	}

	glass3::util::Logger::log(
			"debug",
			"siteListToStationList(): data = |" + json::Serialize(*data)
					+ "|.");

	// get the site list from the SuperEasyJSON message
	//
	json::Array siteListArray = (*data)["SiteList"];

	// Detection formats doesn't have a "StationInfoList" format, but it's
	// simply an array of StationInfo messages, so build it using rapidJSON
	rapidjson::Document stationList(rapidjson::kObjectType);

	// must pass an allocator when the object may need to allocate memory
	rapidjson::Document::AllocatorType& allocator = stationList.GetAllocator();

	// create a rapidjson array
	rapidjson::Value stationListArray(rapidjson::kArrayType);

	// for each site in the array
	for (int i = 0; i < siteListArray.size(); i++) {
		// get the current site
		json::Object siteObject = siteListArray[i];

		detectionformats::stationInfo stationObject;

		// since detection formats and glass3 use different json libraries,
		// need to take the values out of the SuperEasyJSON object and add them
		// to the detection formats object
		try {
			// build stationInfo object
			// first site sub-object
			// station is required
			stationObject.site.station = (siteObject)["Sta"].ToString();

			// comp is optional
			if (siteObject.HasKey("Comp")) {
				stationObject.site.channel = (siteObject)["Comp"].ToString();
			}

			// network is required
			stationObject.site.network = (siteObject)["Net"].ToString();

			// lcation is optional
			if (siteObject.HasKey("Loc")) {
				stationObject.site.location = (siteObject)["Loc"].ToString();
			}

			// site location
			stationObject.latitude = (siteObject)["Lat"].ToDouble();
			stationObject.longitude = (siteObject)["Lon"].ToDouble();
			stationObject.elevation = (siteObject)["Z"].ToDouble();

			// site quality metrics
			stationObject.quality = (siteObject)["Qual"].ToDouble();
			stationObject.enable = (siteObject)["Use"].ToBool();
			stationObject.useforteleseismic =
					(siteObject)["UseForTele"].ToBool();

			// check for validity
			if (stationObject.isvalid() == true) {
				// add to array
				rapidjson::Document stationJSON(rapidjson::kObjectType);
				stationObject.tojson(stationJSON, allocator);
				stationListArray.PushBack(stationJSON, allocator);
			}
		} catch (const std::exception &e) {
			glass3::util::Logger::log(
					"warning",
					"siteListToStationList: Problem building StationInfo message: "
							+ std::string(e.what()));
		}
	}

	// Add the type and array to the message
	stationList.AddMember(TYPE_KEY, "StationInfoList", allocator);
	stationList.AddMember(STATIONLIST_KEY, stationListArray, allocator);

	// convert to a string to return
	std::string outputString = detectionformats::ToJSONString(stationList);

	// done
	return (outputString);
}

// -----------------------------------------------siteLookupToStationInfoRequest
std::string siteLookupToStationInfoRequest(std::shared_ptr<json::Object> data,
											const std::string &outputAgencyID,
											const std::string &outputAuthor) {
	/*
	 * The glasscore SiteLookup message is defined at:
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteLookup.md  // NOLINT
	 *
	 * The detection formats StationInfoRequest message is defined at:
	 * https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/StationInfoRequest.md  // NOLINT
	 */
	// nullcheck
	if (data == NULL) {
		glass3::util::Logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Null json data object "
				"passed in.");
		return ("");
	}

	// type check
	if (!(data->HasKey("Type"))) {
		glass3::util::Logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Bad json data object "
				"passed in.");
		return ("");
	}
	if ((*data)["Type"].ToString() != "SiteLookup") {
		glass3::util::Logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Wrong json data object "
				"passed in.");
		return ("");
	}

	glass3::util::Logger::log(
			"debug",
			"siteLookupToStationInfoRequest(): data = |"
					+ json::Serialize(*data) + "|.");

	std::string outputString = "";
	std::string site = "";
	std::string comp = "";
	std::string net = "";
	std::string loc = "";

	// get the scnl from the message
	if (data->HasKey("Site")) {
		site = (*data)["Site"].ToString();
	} else {
		glass3::util::Logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Missing required Site key");
		return ("");
	}

	if (data->HasKey("Comp")) {
		comp = (*data)["Comp"].ToString();
	}

	if (data->HasKey("Net")) {
		net = (*data)["Net"].ToString();
	} else {
		glass3::util::Logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Missing required Net key");
		return ("");
	}

	if (data->HasKey("Loc")) {
		loc = (*data)["Loc"].ToString();
	}

	// build station info request message
	detectionformats::stationInfoRequest stationInfoRequest;

	// since detection formats and glass3 use different json libraries,
	// need to take the values out of the SuperEasyJSON object and add them
	// to the detection formats object
	try {
		// required values
		stationInfoRequest.site.station = site;
		stationInfoRequest.site.channel = comp;
		stationInfoRequest.site.network = net;
		stationInfoRequest.site.location = loc;
		stationInfoRequest.source.agencyid = outputAgencyID;
		stationInfoRequest.source.author = outputAuthor;
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"warning",
				"siteLookupToStationInfoRequest: Problem building station info "
						"request message: " + std::string(e.what()));
	}

	// check if valid
	if (stationInfoRequest.isvalid() == false) {
		return ("");
	}

	// build string
	rapidjson::Document requestdocument;
	outputString = detectionformats::ToJSONString(
			stationInfoRequest.tojson(requestdocument,
										requestdocument.GetAllocator()));

	// done
	return (outputString);
}
}  // namespace parse
}  // namespace glass3
