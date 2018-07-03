#include <convert.h>
#include <json.h>
#include <logger.h>
#include <timeutil.h>
#include <stringutil.h>
#include <detection-formats.h>
#include <string>
#include <ctime>
#include <vector>
#include <memory>

namespace glass3 {
namespace parse {

// ---------------------------------------------------------hypoToJSONDetection
std::string hypoToJSONDetection(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor) {
	if (data == NULL) {
		logger::log("error",
					"hypoToJSONDetection(): Null json data object passed in.");
		return ("");
	}

	if (!(data->HasKey("Type"))) {
		logger::log("error",
					"hypoToJSONDetection(): Bad json data object passed in.");
		return ("");
	}

	if ((*data)["Type"].ToString() != "Hypo") {
		return ("");
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		logger::log("error",
					"hypoToJSONDetection(): Bad json data object passed in, no "
					"ID.");
		return ("");
	}

	logger::log(
			"info",
			"hypoToJSONDetection(): Converting a hypo message with id: " + ID
					+ ".");

	logger::log(
			"debug",
			"hypoToJSONDetection(): data = |" + json::Serialize(*data) + "|.");

	// what time is it now
	time_t tNow;
	std::time(&tNow);

	std::string OutputData = "";

	// build detection message
	detectionformats::detection detection;

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

					logger::log("error", "Error validating pick.");
					for (int errorcount = 0;
							errorcount < static_cast<int>(errors.size());
							errorcount++) {
						logger::log(
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

				// phase
				correlation.correlationvalue = (dataobject)["Correlation"]
						.ToDouble();

				// optional values
				// phase
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
					logger::log(
							"error",
							"hypoToJSONDetection: Error validating correlation.");
					for (int errorcount = 0;
							errorcount < static_cast<int>(errors.size());
							errorcount++) {
						logger::log(
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
	} catch (const std::exception &e) {
		logger::log(
				"warning",
				"hypoToJSONDetection: Problem building detection message: "
						+ std::string(e.what()));
	}

	// need to check if detection is valid

	rapidjson::Document detectiondocument;
	OutputData = detectionformats::ToJSONString(
			detection.tojson(detectiondocument,
								detectiondocument.GetAllocator()));

	// done
	return (OutputData);
}

// ---------------------------------------------------------cancelToJSONRetract
std::string cancelToJSONRetract(std::shared_ptr<json::Object> data,
								const std::string &outputAgencyID,
								const std::string &outputAuthor) {
	if (data == NULL) {
		logger::log("error",
					"cancelToJSONRetract(): Null json data object passed in.");
		return ("");
	}

	if (!(data->HasKey("Type"))) {
		logger::log("error",
					"cancelToJSONRetract(): Bad json data object passed in.");
		return ("");
	}

	if ((*data)["Type"].ToString() != "Cancel") {
		return ("");
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		logger::log("error",
					"hypoToJSONDetection(): Bad json data object passed in, no "
					"ID.");
		return ("");
	}

	logger::log(
			"info",
			"cancelToJSONRetract(): Converting a cancel message with id: " + ID
					+ ".");

	logger::log(
			"debug",
			"cancelToJSONRetract(): data = |" + json::Serialize(*data) + "|.");

	std::string OutputData = "";

	// build retract message
	detectionformats::retract retract;

	try {
		// required values
		retract.id = ID;
		retract.source.agencyid = outputAgencyID;
		retract.source.author = outputAuthor;
	} catch (const std::exception &e) {
		logger::log(
				"warning",
				"cancelToJSONRetract: Problem building retract message: "
						+ std::string(e.what()));
	}

	rapidjson::Document retractdocument;
	OutputData = detectionformats::ToJSONString(
			retract.tojson(retractdocument, retractdocument.GetAllocator()));

	// done
	return (OutputData);
}

// --------------------------------------------------------siteListToStationList
std::string siteListToStationList(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log(
				"error",
				"siteListToStationList(): Null json data object passed in.");
		return ("");
	}

	if (!(data->HasKey("Cmd"))) {
		logger::log("error",
					"siteListToStationList(): Bad json data object passed in.");
		return ("");
	}

	if ((*data)["Cmd"].ToString() != "SiteList") {
		logger::log(
				"error",
				"siteListToStationList(): Wrong json data object passed in.");
		return ("");
	}

	logger::log(
			"debug",
			"siteListToStationList(): data = |" + json::Serialize(*data)
					+ "|.");

	std::string OutputData = "";

	json::Object stationListObj;
	stationListObj["Type"] = "StationInfoList";

	// array to hold data
	json::Array stationListArray;

	json::Array siteListArray = (*data)["SiteList"];
	for (int i = 0; i < siteListArray.size(); i++) {
		json::Object siteObject = siteListArray[i];

		detectionformats::stationInfo stationobject;

		// build stationInfo object
		// site subobject
		stationobject.site.station = (siteObject)["Sta"].ToString();

		if (siteObject.HasKey("Comp")) {
			stationobject.site.channel = (siteObject)["Comp"].ToString();
		}

		stationobject.site.network = (siteObject)["Net"].ToString();

		if (siteObject.HasKey("Loc")) {
			stationobject.site.location = (siteObject)["Loc"].ToString();
		}

		stationobject.latitude = (siteObject)["Lat"].ToDouble();
		stationobject.longitude = (siteObject)["Lon"].ToDouble();
		stationobject.elevation = (siteObject)["Z"].ToDouble();

		// NOTE: Need to get these from metadata server eventually
		stationobject.quality = (siteObject)["Qual"].ToDouble();
		stationobject.enable = (siteObject)["Use"].ToBool();
		stationobject.useforteleseismic = (siteObject)["UseForTele"].ToBool();

		// build json string
		rapidjson::Document stationdocument;
		std::string stationjson = detectionformats::ToJSONString(
				stationobject.tojson(stationdocument,
										stationdocument.GetAllocator()));

		json::Value deserializedJSON = json::Deserialize(stationjson);

		// make sure we got valid json
		if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
			// create the new object
			json::Object* newStation = new json::Object(
					deserializedJSON.ToObject());
			stationListArray.push_back(*newStation);
		}
	}

	stationListObj["StationList"] = stationListArray;

	OutputData = json::Serialize(stationListObj);

	// done
	return (OutputData);
}

// -----------------------------------------------siteLookupToStationInfoRequest
std::string siteLookupToStationInfoRequest(std::shared_ptr<json::Object> data,
											const std::string &outputAgencyID,
											const std::string &outputAuthor) {
	if (data == NULL) {
		logger::log("error",
					"siteLookupToStationInfoRequest(): Null json data object "
					"passed in.");
		return ("");
	}

	if (!(data->HasKey("Type"))) {
		logger::log("error",
					"siteLookupToStationInfoRequest(): Bad json data object "
					"passed in.");
		return ("");
	}

	if ((*data)["Type"].ToString() != "SiteLookup") {
		logger::log("error",
					"siteLookupToStationInfoRequest(): Wrong json data object "
					"passed in.");
		return ("");
	}

	logger::log(
			"debug",
			"siteLookupToStationInfoRequest(): data = |"
					+ json::Serialize(*data) + "|.");

	std::string OutputString = "";
	std::string site = "";
	std::string comp = "";
	std::string net = "";
	std::string loc = "";

	// get the scnl from the message
	if (data->HasKey("Site")) {
		site = (*data)["Site"].ToString();
	} else {
		logger::log(
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
		logger::log(
				"error",
				"siteLookupToStationInfoRequest(): Missing required Net key");
		return ("");
	}

	if (data->HasKey("Loc")) {
		loc = (*data)["Loc"].ToString();
	}

	// build station info request message
	detectionformats::stationInfoRequest stationInfoRequest;

	try {
		// required values
		stationInfoRequest.site.station = site;
		stationInfoRequest.site.channel = comp;
		stationInfoRequest.site.network = net;
		stationInfoRequest.site.location = loc;
		stationInfoRequest.source.agencyid = outputAgencyID;
		stationInfoRequest.source.author = outputAuthor;
	} catch (const std::exception &e) {
		logger::log(
				"warning",
				"siteLookupToStationInfoRequest: Problem building station info "
						"request message: " + std::string(e.what()));
	}

	// check if valid
	if (stationInfoRequest.isvalid() == true) {
		// build string
		rapidjson::Document requestdocument;
		OutputString = detectionformats::ToJSONString(
				stationInfoRequest.tojson(requestdocument,
											requestdocument.GetAllocator()));
	} else {
		// get errors
		std::vector<std::string> errorlist = stationInfoRequest.geterrors();
		std::string errorstring = "";
		for (int i = 0; i < errorlist.size(); i++) {
			errorstring += errorlist[i];
		}

		logger::log(
				"error",
				"siteLookupToStationInfoRequest: Invalid station info request "
						"message: " + errorstring);
	}

	// done
	return (OutputString);
}
}  // namespace parse
}  // namespace glass3
