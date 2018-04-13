#include <output.h>
#include <json.h>
#include <convert.h>
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <timeutil.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>

namespace glass {

output::output()
		: util::ThreadBaseClass("output", 100) {
	logger::log("debug", "output::output(): Construction.");

	std::time(&tLastWorkReport);
	std::time(&m_tLastSiteRequest);

	// thread will be declared dead
	// if it doesn't report within this interval
	m_iCheckInterval = 120;

	// interval to report performance statistics
	ReportInterval = 60;

	// init performance counters
	m_iMessageCounter = 0;
	m_iEventCounter = 0;
	m_iCancelCounter = 0;
	m_iHypoCounter = 0;
	m_iExpireCounter = 0;
	m_iLookupCounter = 0;
	m_iSiteListCounter = 0;

	// init to null, allocated in clear
	m_TrackingCache = NULL;
	m_MessageQueue = NULL;

	// setup thread pool for output
	m_ThreadPool = new util::ThreadPool("outputpool");

	// init config to defaults and allocate
	clear();
}

output::~output() {
	logger::log("debug", "output::~output(): Destruction.");

	// stop the input thread
	stop();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_TrackingCache != NULL) {
		m_TrackingCache->clearCache();
		delete (m_TrackingCache);
	}

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_MessageQueue != NULL) {
		m_MessageQueue->clearQueue();
		delete (m_MessageQueue);
	}
}

// configuration
bool output::setup(json::Object *config) {
	if (config == NULL) {
		logger::log("error", "output::setup(): NULL configuration passed in.");
		return (false);
	}

	logger::log("debug", "output::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		logger::log("error", "output::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"];
		if (configtype != "GlassOutput") {
			logger::log("error",
						"output::setup(): Wrong configuration provided, "
								"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	// publicationTimes
	if (!(config->HasKey("PublicationTimes"))) {
		// pubdelay is optional, default to 0
		m_PublicationTimes.push_back(0);
		logger::log(
				"info",
				"output::setup(): PublicationTimes not specified, using default "
				"of 0.");
	} else {
		json::Array dataarray = (*config)["PublicationTimes"];
		for (int i = 0; i < dataarray.size(); i++) {
			int pubTime = dataarray[i].ToInt();
			m_PublicationTimes.push_back(pubTime);

			logger::log(
					"info",
					"output::setup(): Using Publication Time #"
							+ std::to_string(i) + ": " + std::to_string(pubTime)
							+ " .");
		}
	}

	// agencyid
	if (!(config->HasKey("OutputAgencyID"))) {
		// agencyid is optional
		m_sOutputAgencyID = "US";
		logger::log("info",
					"output::setup(): Defaulting to US as OutputAgencyID.");
	} else {
		m_sOutputAgencyID = (*config)["OutputAgencyID"].ToString();
		logger::log(
				"info",
				"output::setup(): Using AgencyID: " + m_sOutputAgencyID
						+ " for output.");
	}

	// author
	if (!(config->HasKey("OutputAuthor"))) {
		// agencyid is optional
		m_sOutputAuthor = "glass";
		logger::log("info",
					"output::setup(): Defaulting to glass as OutputAuthor.");
	} else {
		m_sOutputAuthor = (*config)["OutputAuthor"].ToString();
		logger::log(
				"info",
				"output::setup(): Using Author: " + m_sOutputAuthor
						+ " for output.");
	}

	// SiteListDelay
	if (!(config->HasKey("SiteListDelay"))) {
		logger::log("info", "output::setup(): SiteListDelay not specified.");
	} else {
		m_iSiteListDelay = (*config)["SiteListDelay"].ToInt();

		logger::log(
				"info",
				"output::setup(): Using SiteListDelay: "
						+ std::to_string(m_iSiteListDelay) + ".");
	}

	// StationFile
	if (!(config->HasKey("StationFile"))) {
		logger::log("info", "output::setup(): StationFile not specified.");
	} else {
		m_sStationFile = (*config)["StationFile"].ToString();

		logger::log(
				"info",
				"output::setup(): Using StationFile: " + m_sStationFile + ".");
	}

	// unlock our configuration
	m_ConfigMutex.unlock();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_TrackingCache != NULL) {
		delete (m_TrackingCache);
	}
	m_TrackingCache = new util::Cache();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_MessageQueue != NULL) {
		delete (m_MessageQueue);
	}
	m_MessageQueue = new util::Queue();

	logger::log("debug", "output::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	util::BaseClass::setup(config);

	// we're done
	return (true);
}

void output::clear() {
	logger::log("debug", "output::clear(): clearing configuration.");

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	m_PublicationTimes.clear();
	m_iSiteListDelay = -1;
	m_sStationFile = "";

	// unlock our configuration
	m_ConfigMutex.unlock();

	// finally do baseclass clear
	util::BaseClass::clear();
}

void output::sendToOutput(std::shared_ptr<json::Object> message) {
	if (message == NULL) {
		return;
	}

	if (m_MessageQueue != NULL) {
		m_MessageQueue->addDataToQueue(message);
	}
}

bool output::check() {
	// don't check threadpool if it is not created yet
	if (m_ThreadPool != NULL) {
		// check threadpool
		if (m_ThreadPool->check() == false) {
			return (false);
		}
	}

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::check());
}

// add data to output cache
bool output::addTrackingData(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::addtrackingdata(): Bad json object passed in.");
		return (false);
	}

	// get the id
	std::string id = "";
	if ((*data).HasKey("ID")) {
		id = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		id = (*data)["Pid"].ToString();
	} else {
		logger::log("error",
					"output::addtrackingdata(): No ID found data json.");
		return (false);
	}

	// don't do anything if we didn't get an ID
	if (id == "") {
		logger::log("error",
					"output::addtrackingdata(): Bad ID from data json.");
		return (false);
	}

	logger::log(
			"debug",
			"output::addTrackingData(): New tracking data: "
					+ json::Serialize(*data));

	// check to see if this event is already being tracked
	std::shared_ptr<json::Object> existingdata = m_TrackingCache->getFromCache(
			id);
	if (existingdata != NULL) {
		// it is, copy the pub log for an existing event
		if (!(*existingdata).HasKey("PubLog")) {
			logger::log(
					"error",
					"output::addtrackingdata(): existing event missing pub log! :"
							+ json::Serialize(*existingdata));

			return (false);
		} else {
			json::Array pubLog = (*existingdata)["PubLog"].ToArray();
			(*data)["PubLog"] = pubLog;
		}
	} else {
		// it isn't generate the pub log for a new event
		json::Array pubLog;

		// for each pub time
		for (auto pubTime : m_PublicationTimes) {
			// generate a pub log entry
			pubLog.push_back(0);
		}
		(*data)["PubLog"] = pubLog;
	}

	// add, cache handles updates
	return (m_TrackingCache->addToCache(data, id));
}

// remove data from output cache
bool output::removeTrackingData(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::removetrackingdata(): Bad json object passed in.");
		return (false);
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		logger::log(
				"error",
				"output::removetrackingdata(): Bad json hypo object passed in.");

		return (false);
	}

	return (removeTrackingData(ID));
}

// remove data from output cache
bool output::removeTrackingData(std::string ID) {
	if (ID == "") {
		logger::log("error",
					"output::removetrackingdata(): Empty ID passed in.");
		return (false);
	}

	if (m_TrackingCache->isInCache(ID) == true) {
		return (m_TrackingCache->removeFromCache(ID));
	} else {
		return (false);
	}
}

std::shared_ptr<json::Object> output::getTrackingData(std::string id) {
	std::shared_ptr<json::Object> nullObj;
	if (id == "") {
		logger::log("error",
					"output::removetrackingdata(): Empty ID passed in.");
		return (nullObj);
	} else if (id == "null") {
		logger::log("warn",
					"output::removetrackingdata(): Invalid ID passed in.");
		return (nullObj);
	}

	// return the value
	if (m_TrackingCache->isInCache(id) == true) {
		return (m_TrackingCache->getFromCache(id));
	} else {
		return (nullObj);
	}
}

std::shared_ptr<json::Object> output::getNextTrackingData() {
	// get the data
	std::shared_ptr<json::Object> data = m_TrackingCache->getNextFromCache(
			true);

	// loop until we hit the end of the list
	while (data != NULL) {
		// check to see if we can release the data
		if (isDataReady(data) == true) {
			// return the value
			return (data);
		}

		// get the next station
		data = m_TrackingCache->getNextFromCache(false);
	}

	// if we found nothing that we can send out, we're done
	return (NULL);
}

// check if data in output cache
bool output::haveTrackingData(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::havetrackingdata(): Bad json object passed in.");
		return (false);
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		logger::log(
				"error",
				"output::havetrackingdata(): Bad json hypo object passed in.");
		return (false);
	}

	return (haveTrackingData(ID));
}

bool output::haveTrackingData(std::string ID) {
	if (ID == "") {
		logger::log("error", "output::haveTrackingData(): Empty ID passed in.");
		return (false);
	}

	return (m_TrackingCache->isInCache(ID));
}

void output::clearTrackingData() {
	m_TrackingCache->clearCache();
}

bool output::work() {
	// pull data from our config at the start of each loop
	// so that we can have config that changes
	// should I do this?
	m_ConfigMutex.lock();
	int siteListDelay = m_iSiteListDelay;
	m_ConfigMutex.unlock();

	// null check
	if (m_MessageQueue == NULL) {
		// no message queue means we've got big problems
		return (false);
	}

	// first see what we're supposed to do with a new message
	// see if there's anything in the message queue
	std::shared_ptr<json::Object> message = m_MessageQueue->getDataFromQueue();
	int queueSize = m_MessageQueue->size();

	// if we got something
	if (message != NULL) {

		logger::log(
				"debug",
				"associator::dispatch(): got message:"
						+ json::Serialize(*message)
						+ " from associator. (queueSize:"
						+ std::to_string(queueSize) + ")");

		// what time is it
		time_t tNow;
		std::time(&tNow);

		// get the message type
		std::string messagetype;
		if (message->HasKey("Cmd")) {
			messagetype = (*message)["Cmd"].ToString();
		} else if (message->HasKey("Type")) {
			messagetype = (*message)["Type"].ToString();
		} else {
			logger::log(
					"critical",
					"output::work(): BAD message passed in, no Cmd/Type found.");
			return (false);
		}

		std::string messageid;
		if ((*message).HasKey("ID")) {
			messageid = (*message)["ID"].ToString();
		} else if ((*message).HasKey("Pid")) {
			messageid = (*message)["Pid"].ToString();
		} else {
			messageid = "null";
		}

		// count the message
		m_iMessageCounter++;

		// glass has a hypo it wants us to send
		if (messagetype == "Hypo") {
			std::shared_ptr<json::Object> trackingData = getTrackingData(
					messageid);

			if (trackingData == NULL) {
				return (false);
			}

			logger::log(
					"debug",
					"output::work(): Outputting a " + messagetype + " message"
							+ " for " + messageid + " tracking: "
							+ json::Serialize(*trackingData));

			// check to see if we've published this event before
			// for this check, we want to know if the current version
			// has been marked as pub
			if (isDataPublished(trackingData, false) == true) {
				(*message)["IsUpdate"] = true;
			} else {
				(*message)["IsUpdate"] = false;
			}

			// write out the hypo to a disk file,
			// using the threadpool
			m_ThreadPool->addJob(
					std::bind(&output::writeOutput, this, message));
			m_iHypoCounter++;
		} else if (messagetype == "SiteList") {
			// glass has a stationlist to write to disk
			logger::log(
					"debug",
					"output::work(): Outputting a " + messagetype
							+ " message.");

			// write out the stationlist
			// using the threadpool
			m_ThreadPool->addJob(
					std::bind(&output::writeOutput, this, message));
			m_iSiteListCounter++;
		} else if (messagetype == "Event") {
			// add the event to the tracking cache
			addTrackingData(message);

			m_iEventCounter++;
		} else if (messagetype == "Cancel") {
			std::shared_ptr<json::Object> trackingData = getTrackingData(
					messageid);

			// see if we've tracked this event
			if (trackingData != NULL) {
				// we have
				logger::log(
						"debug",
						"output::work(): Canceling event " + messageid
								+ " and removing it from tracking.");

				// check to see if this has been published, we don't care what
				// version
				bool published = isDataPublished(trackingData, true);
				if (published == true) {
					// make sure we have a type
					if (!((*message).HasKey("Type")))
						(*message)["Type"] = messagetype;

					logger::log(
							"debug",
							"output::work(): Generating retraction message for"
									" published event " + messageid + ".");

					// output retraction immediately
					writeOutput(message);
				}

				// remove from the tracking cache
				removeTrackingData(messageid);
			}

			m_iCancelCounter++;
		} else if (messagetype == "Expire") {
			std::shared_ptr<json::Object> trackingData = getTrackingData(
					messageid);
			// see if we've tracked this event
			if (trackingData != NULL) {
				// we have
				// glass has canceled an event we have tracked
				logger::log(
						"debug",
						"output::work(): Expiring event " + messageid
								+ " and removing it from tracking.");

				// first try to remove any pending events
				// from the tracking cache
				removeTrackingData(message);
			}
			m_iExpireCounter++;
		} else if (messagetype == "SiteLookup") {
			// station info request

			// output immediately
			writeOutput(message);

			m_iLookupCounter++;
		} else {
			// got some other message
			logger::log(
					"warning",
					"output::work(): Unknown message from glasslib: "
							+ json::Serialize(*message) + ".");
		}

		// reporting
		if ((tNow - tLastWorkReport) >= ReportInterval) {
			if (m_iMessageCounter == 0)
				logger::log(
						"warning",
						"output::work(): Recieved NO messages from associator "
								"thread in the last "
								+ std::to_string(
										static_cast<int>(tNow)
												- tLastWorkReport)
								+ " seconds.");
			else
				logger::log(
						"info",
						"output::work(): Received "
								+ std::to_string(m_iMessageCounter)
								+ " messages (event messages: "
								+ std::to_string(m_iEventCounter)
								+ "; update messages: "
								+ std::to_string(m_iCancelCounter)
								+ "; expire messages: "
								+ std::to_string(m_iExpireCounter)
								+ "; hypo messages:"
								+ std::to_string(m_iHypoCounter)
								+ "; lookup messages:"
								+ std::to_string(m_iLookupCounter)
								+ "; sitelist messages:"
								+ std::to_string(m_iSiteListCounter) + ")"
								+ " data from associator thread in the last "
								+ std::to_string(
										static_cast<int>(tNow - tLastWorkReport))
								+ " seconds. ("
								+ std::to_string(
										static_cast<double>(m_iMessageCounter)
												/ (static_cast<double>(tNow)
														- tLastWorkReport))
								+ " data per second)");

			tLastWorkReport = tNow;
			m_iMessageCounter = 0;
			m_iEventCounter = 0;
			m_iCancelCounter = 0;
			m_iHypoCounter = 0;
			m_iExpireCounter = 0;
			m_iLookupCounter = 0;
			m_iSiteListCounter = 0;
		}
	}

	// see if there's anything in the tracking cache
	std::shared_ptr<json::Object> data = getNextTrackingData();

	// got something
	if (data != NULL) {
		// get the id
		std::string id;
		if ((*data).HasKey("ID")) {
			id = (*data)["ID"].ToString();
		} else if ((*data).HasKey("Pid")) {
			id = (*data)["Pid"].ToString();
		} else {
			logger::log("warning",
						"output::work(): Bad data object received from "
						"getdatafromcache(), no ID, skipping data.");

			// remove the message we found from the cache, since it is bad
			removeTrackingData(data);

			// keep working
			return (true);
		}

		// get the command
		std::string command;
		if ((*data).HasKey("Cmd")) {
			command = (*data)["Cmd"].ToString();
		} else {
			logger::log("warning",
						"output::work(): Bad data object received from "
						"getdatafromcache(), no Cmd, skipping data.");

			// remove the value we found from the cache, since it is bad
			removeTrackingData(data);

			// keep working
			return (true);
		}

		// process the data based on the tracking message
		if (command == "Event") {
			// Request the hypo from associator
			if (Associator != NULL) {
				// build the request
				std::shared_ptr<json::Object> datarequest = std::make_shared<
						json::Object>(json::Object());
				(*datarequest)["Cmd"] = "ReqHypo";
				(*datarequest)["Pid"] = id;

				// send the request
				Associator->sendToAssociator(datarequest);
			}
		}
	}

	// request current stationlist
	if (siteListDelay > 0) {
		// what time is it
		time_t tNowRequest;
		std::time(&tNowRequest);

		// every interval
		if ((tNowRequest - m_tLastSiteRequest) >= siteListDelay) {
			// Request the sitelist from associator
			if (Associator != NULL) {
				// build the request
				std::shared_ptr<json::Object> datarequest = std::make_shared<
						json::Object>(json::Object());
				(*datarequest)["Cmd"] = "ReqSiteList";

				// send the request
				Associator->sendToAssociator(datarequest);
			}

			// this is now the last time we wrote
			m_tLastSiteRequest = tNowRequest;
		}
	}

	// work was successful
	return (true);
}

// handle output
void output::writeOutput(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::writeoutput(): Null json object passed in.");
		return;
	}

	// get the data type
	std::string dataType = "unknown";
	if (data->HasKey("Cmd")) {
		dataType = (*data)["Cmd"].ToString();
	} else if (data->HasKey("Type")) {
		dataType = (*data)["Type"].ToString();
	}

	// get the data id (may or may not have)
	std::string ID = "null";
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	}

	std::string agency = getSOutputAgencyId();
	std::string author = getSOutputAuthor();

	if (dataType == "Hypo") {
		// convert a hypo to a detection
		std::string detectionString = parse::hypoToJSONDetection(data, agency,
																	author);

		sendOutput("Detection", ID, detectionString);
	} else if (dataType == "Cancel") {
		// convert a cancel to a retract
		std::string retractString = parse::cancelToJSONRetract(data, agency,
																author);

		sendOutput("Retraction", ID, retractString);
	} else if (dataType == "SiteLookup") {
		// convert a site lookup to a station info request
		std::string stationInfoRequestString =
				parse::siteLookupToStationInfoRequest(data, agency, author);

		sendOutput("StationInfoRequest", ID, stationInfoRequestString);
	} else if (dataType == "SiteList") {
		// convert a site list to a station list
		std::string stationListString = parse::siteListToStationList(data);

		sendOutput("StationList", ID, stationListString);
	} else {
		return;
	}
}

// filter
bool output::isDataReady(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::isdataready(): Null tracking object passed in.");
		return (false);
	}

	if (!(data->HasKey("Cmd"))) {
		logger::log("error",
					"output::isdataready(): Bad tracking object passed in, "
							" missing cmd: " + json::Serialize(*data));
		return (false);
	} else if ((!(data->HasKey("PubLog"))) || (!(data->HasKey("Version")))) {
		logger::log(
				"error",
				"output::isdataready(): Bad tracking object passed in, "
						" missing PubLog or Version:" + json::Serialize(*data));
		return (false);
	}

	// get the id
	std::string id = "";
	if ((*data).HasKey("ID")) {
		id = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		id = (*data)["Pid"].ToString();
	} else {
		id = "";
	}

	// get the time the hypo was created
	std::string createTimeString = (*data)["CreateTime"].ToString();
	int createTime = util::convertISO8601ToEpochTime(createTimeString);

	// get the pub log
	json::Array pubLog = (*data)["PubLog"].ToArray();

	// get the current version
	int currentVersion = (*data)["Version"].ToInt();

	// what time is it now
	time_t tNow;
	std::time(&tNow);

	// has this hypo changed?
	bool changed = isDataChanged(data);

	// for each publication time
	for (int i = 0; i < m_PublicationTimes.size(); i++) {
		// get the published version for this pub time
		int pubVersion = pubLog[i].ToInt();

		// has this pub time been published at all?
		if (pubVersion > 0) {
			// yes, move on
			continue;
		}

		// has this pub time passed?
		if (tNow < (createTime + m_PublicationTimes[i])) {
			// no, move on
			continue;
		}

		// update pubLog for this time
		pubLog[i] = currentVersion;
		(*data)["PubLog"] = pubLog;

		// depending on whether this version has already been changed
		if (changed == true) {
			logger::log(
					"debug",
					"output::isdataready(): Publishing " + id + " version:"
							+ std::to_string(currentVersion) + " tNow:"
							+ std::to_string(static_cast<int>(tNow))
							+ " > (createTime + m_PublicationTimes[i]): "
							+ std::to_string(
									static_cast<int>((createTime
											+ m_PublicationTimes[i])))
							+ " (createTime: " + std::to_string(createTime)
							+ " m_PublicationTimes[i]: "
							+ std::to_string(
									static_cast<int>(m_PublicationTimes[i]))
							+ ")");

			// ready to publish
			return (true);
		} else {
			logger::log(
					"debug",
					"output::isdataready(): Skipping " + id + " version:"
							+ std::to_string(currentVersion)
							+ " because it is has not changed.");

			// already published, don't publish
			return (false);
		}
	}

	// not ready to publish yet
	return (false);
}

bool output::isDataChanged(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		logger::log("error",
					"output::isDataChanged(): Null tracking object passed in.");
		return (false);
	}

	if (!(data->HasKey("Cmd"))) {
		logger::log("error",
					"output::isDataChanged(): Bad tracking object passed in, "
							" missing cmd: " + json::Serialize(*data));
		return (false);
	} else if ((!(data->HasKey("PubLog"))) || (!(data->HasKey("Version")))) {
		logger::log(
				"error",
				"output::isDataChanged(): Bad tracking object passed in, "
						" missing PubLog or Version:" + json::Serialize(*data));
		return (false);
	}

	// get the pub log
	json::Array pubLog = (*data)["PubLog"].ToArray();

	// get the current version
	int currentVersion = (*data)["Version"].ToInt();

	// for each entry in the pub log
	for (int i = 0; i < pubLog.size(); i++) {
		// get the published version for this log entry
		int pubVersion = pubLog[i].ToInt();

		// has the current version been published?
		if (pubVersion == currentVersion) {
			// yes, no change
			return (false);
		}
	}

	// it's changed.
	return (true);
}

bool output::isDataPublished(std::shared_ptr<json::Object> data,
								bool ignoreVersion) {
	if (data == NULL) {
		logger::log(
				"error",
				"output::isDataPublished(): Null json data object passed in.");
		return (false);
	}

	if ((!(data->HasKey("Cmd"))) || (!(data->HasKey("PubLog")))
			|| (!(data->HasKey("Version")))) {
		logger::log(
				"error",
				"output::isDataPublished(): Bad json hypo object passed in "
						" missing Cmd, PubLog or Version "
						+ json::Serialize(*data));
		return (false);
	}

	// get the pub log
	json::Array pubLog = (*data)["PubLog"].ToArray();
	int currentVersion = (*data)["Version"].ToInt();

	// for each entry in the pub log
	for (int i = 0; i < pubLog.size(); i++) {
		// get whether this one was  published
		int pubVersion = pubLog[i].ToInt();

		// pub version less than 1 means not published
		if (pubVersion < 1) {
			continue;
		}

		// check to see if the published version is the current
		// version.
		// if we're writing a detection, and we want to know if an event is new
		//   or an update, we care about this
		// if we're writing a retraction, we don't care about this
		if ((ignoreVersion == false) && (pubVersion == currentVersion)) {
			// we don't count our current version as published
			continue;
		}

		// published
		return (true);
	}

	// not published
	return (false);
}
}  // namespace glass
