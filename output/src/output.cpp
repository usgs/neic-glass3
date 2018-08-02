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
#include <vector>

namespace glass3 {
namespace output {

// ---------------------------------------------------------output
output::output()
		: glass3::util::ThreadBaseClass("output", 100) {
	glass3::util::log("debug", "output::output(): Construction.");

	setEventThreadState(glass3::util::ThreadState::Initialized);
	std::time(&tLastWorkReport);
	std::time(&m_tLastSiteRequest);

	// thread will be declared dead
	// if it doesn't report within this interval
	setHealthCheckInterval(120);

	// interval to report performance statistics
	setReportInterval(60);

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
	m_OutputQueue = NULL;
	m_LookupQueue = NULL;

	// setup thread pool for output
	m_ThreadPool = new glass3::util::ThreadPool("outputpool");

	setEventThreadHealth();
	m_EventThread = NULL;

	setAssociator(NULL);

	// init config to defaults and allocate
	clear();
}

// ---------------------------------------------------------~output
output::~output() {
	glass3::util::log("debug", "output::~output(): Destruction.");

	// stop the input thread
	stop();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_TrackingCache != NULL) {
		m_TrackingCache->clear();
		delete (m_TrackingCache);
	}

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_OutputQueue != NULL) {
		m_OutputQueue->clear();
		delete (m_OutputQueue);
	}

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_LookupQueue != NULL) {
		m_LookupQueue->clear();
		delete (m_LookupQueue);
	}
}

// configuration
bool output::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::log("error",
							"output::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::log("debug", "output::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		glass3::util::log("error",
							"output::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"];
		if (configtype != "GlassOutput") {
			glass3::util::log(
					"error", "output::setup(): Wrong configuration provided, "
							"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// publish on expiration
	if (!(config->HasKey("PublishOnExpiration"))) {
		// publish on expiration is optional, default to false
		setPubOnExpiration(false);
		glass3::util::log(
				"info",
				"output::setup(): PublishOnExpiration not specified, using default "
				"of false.");
	} else {
		setPubOnExpiration((*config)["PublishOnExpiration"].ToBool());

		glass3::util::log(
				"info",
				"output::setup(): Using PublishOnExpiration: "
						+ std::to_string(getPubOnExpiration()) + " .");
	}

	// publicationTimes
	if (!(config->HasKey("PublicationTimes"))) {
		// pubdelay is optional, default to 0
		clearPubTimes();
		addPubTime(0);
		glass3::util::log(
				"info",
				"output::setup(): PublicationTimes not specified, using default "
				"of 0.");
	} else {
		json::Array dataarray = (*config)["PublicationTimes"];
		clearPubTimes();
		for (int i = 0; i < dataarray.size(); i++) {
			int pubTime = dataarray[i].ToInt();
			addPubTime(pubTime);

			glass3::util::log(
					"info",
					"output::setup(): Using Publication Time #"
							+ std::to_string(i) + ": " + std::to_string(pubTime)
							+ " .");
		}
	}

	// agencyid
	if (!(config->HasKey("OutputAgencyID"))) {
		// agencyid is optional
		setOutputAgency("US");
		glass3::util::log(
				"info", "output::setup(): Defaulting to US as OutputAgencyID.");
	} else {
		setOutputAgency((*config)["OutputAgencyID"].ToString());
		glass3::util::log(
				"info",
				"output::setup(): Using AgencyID: " + getOutputAgencyId()
						+ " for output.");
	}

	// author
	if (!(config->HasKey("OutputAuthor"))) {
		// agencyid is optional
		setOutputAuthor("glass3");
		glass3::util::log(
				"info",
				"output::setup(): Defaulting to glass as OutputAuthor.");
	} else {
		setOutputAuthor((*config)["OutputAuthor"].ToString());
		glass3::util::log(
				"info",
				"output::setup(): Using Author: " + getOutputAuthor()
						+ " for output.");
	}

	// SiteListDelay
	if (!(config->HasKey("SiteListDelay"))) {
		glass3::util::log("info",
							"output::setup(): SiteListDelay not specified.");
	} else {
		setSiteListDelay((*config)["SiteListDelay"].ToInt());

		glass3::util::log(
				"info",
				"output::setup(): Using SiteListDelay: "
						+ std::to_string(getSiteListDelay()) + ".");
	}

	// StationFile
	if (!(config->HasKey("StationFile"))) {
		glass3::util::log("info",
							"output::setup(): StationFile not specified.");
	} else {
		setStationFile((*config)["StationFile"].ToString());

		glass3::util::log(
				"info",
				"output::setup(): Using StationFile: " + getStationFile()
						+ ".");
	}

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_TrackingCache != NULL) {
		delete (m_TrackingCache);
	}
	m_TrackingCache = new glass3::util::Cache();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_OutputQueue != NULL) {
		delete (m_OutputQueue);
	}
	m_OutputQueue = new glass3::util::Queue();

	// cppcheck-suppress nullPointerRedundantCheck
	if (m_LookupQueue != NULL) {
		delete (m_LookupQueue);
	}
	m_LookupQueue = new glass3::util::Queue();

	glass3::util::log("debug", "output::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::util::BaseClass::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void output::clear() {
	glass3::util::log("debug", "output::clear(): clearing configuration.");

	setPubOnExpiration(false);

	clearPubTimes();
	setSiteListDelay(-1);
	setStationFile("");

	setOutputAgency("US");
	setOutputAuthor("glass3");

	// finally do baseclass clear
	glass3::util::BaseClass::clear();
}

// ---------------------------------------------------------sendToOutput
void output::sendToOutput(std::shared_ptr<json::Object> message) {
	if (message == NULL) {
		return;
	}

	// get the message type
	std::string messagetype;
	if (message->HasKey("Cmd")) {
		messagetype = (*message)["Cmd"].ToString();
	} else if (message->HasKey("Type")) {
		messagetype = (*message)["Type"].ToString();
	} else {
		glass3::util::log(
				"critical",
				"output::sendToOutput(): BAD message passed in, no Cmd/Type found.");
		return;
	}

	// send site messages to their own queue, because they can be
	// very chatty and we don't want anything to slowdown output messages
	if ((messagetype == "SiteList") || (messagetype == "SiteLookup")) {
		if (m_LookupQueue != NULL) {
			m_LookupQueue->addDataToQueue(message);
		}
	} else {
		if (m_OutputQueue != NULL) {
			m_OutputQueue->addDataToQueue(message);
		}
	}
}

// ---------------------------------------------------------start
bool output::start() {
	// are we already running
	if ((getEventThreadState() == glass3::util::ThreadState::Starting)
			|| (getEventThreadState() == glass3::util::ThreadState::Started)) {
		glass3::util::log("warning",
							"output::start(): Event Thread is already starting "
									"or running. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_EventThread != NULL) {
		glass3::util::log(
				"warning",
				"output::start(): Event Thread is already allocated.");
		return (false);
	}

	// we're starting
	setEventThreadState(glass3::util::ThreadState::Starting);

	// start the thread
	m_EventThread = new std::thread(&output::checkEventsLoop, this);

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::start());
}

// ---------------------------------------------------------stop
bool output::stop() {
	// check if we're running
	if ((getEventThreadState() == glass3::util::ThreadState::Stopping)
			|| (getEventThreadState() == glass3::util::ThreadState::Stopped)
			|| (getEventThreadState() == glass3::util::ThreadState::Initialized)) {
		glass3::util::log(
				"warning", "output::stop(): Event Thread is not running, "
						"or is already stopping. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_EventThread == NULL) {
		glass3::util::log("warning",
							"output::stop(): Event Thread is not allocated. ");
		return (false);
	}

	// we're stopping
	setEventThreadState(glass3::util::ThreadState::Stopping);

	// wait for the thread to finish
	m_EventThread->join();

	// delete it
	delete (m_EventThread);
	m_EventThread = NULL;

	// we're now stopped
	setEventThreadState(glass3::util::ThreadState::Stopped);

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::stop());
}

// ---------------------------------------------------------setEventThreadHealth
void output::setEventThreadHealth(bool health) {
	if (health == true) {
		std::time_t tNow;
		std::time(&tNow);
		setEventLastHealthy(tNow);
	} else {
		setEventLastHealthy(0);
		glass3::util::log(
				"warning",
				"output::setEventThreadHealth(): health set to false");
	}
}

// ---------------------------------------------------------healthCheck
bool output::healthCheck() {
	// don't check threadpool if it is not created yet
	if (m_ThreadPool != NULL) {
		// check threadpool
		if (m_ThreadPool->healthCheck() == false) {
			return (false);
		}
	}

	if ((getWorkThreadsState() != glass3::util::ThreadState::Starting)
			&& (getWorkThreadsState() != glass3::util::ThreadState::Initialized)
			&& (getHealthCheckInterval() > 0)) {
		// thread is dead if we're not running
		if (getEventThreadState() != glass3::util::ThreadState::Started) {
			glass3::util::log(
					"error",
					"output::healthCheck(): Thread is not running. ("
							+ getThreadName() + ")");
			return (false);
		}

		// see if it's time to check
		time_t tNow;
		std::time(&tNow);
		int lastCheckInterval = (tNow - getEventLastHealthy());
		if (lastCheckInterval > getHealthCheckInterval()) {
			// if the we've exceeded the health check interval, the thread
			// is dead
			glass3::util::log(
					"error",
					"output::healthCheck(): lastCheckInterval for thread "
							+ getThreadName()
							+ " exceeds health check interval ( "
							+ std::to_string(lastCheckInterval) + " > "
							+ std::to_string(getHealthCheckInterval()) + " )");
			return (false);
		}
	}

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::healthCheck());
}

// ---------------------------------------------------------addTrackingData
bool output::addTrackingData(std::shared_ptr<json::Object> data) {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	if (data == NULL) {
		glass3::util::log(
				"error",
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
		glass3::util::log(
				"error", "output::addtrackingdata(): No ID found data json.");
		return (false);
	}

	// don't do anything if we didn't get an ID
	if (id == "") {
		glass3::util::log(
				"error", "output::addtrackingdata(): Bad ID from data json.");
		return (false);
	}

	// check to see if this event is already being tracked
	std::shared_ptr<const json::Object> existingdata = m_TrackingCache
			->getFromCache(id);
	if (existingdata != NULL) {
		// it is, copy the pub log for an existing event
		if (!(*existingdata).HasKey("PubLog")) {
			glass3::util::log(
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
		for (auto pubTime : getPubTimes()) {
			// generate a pub log entry
			pubLog.push_back(0);
		}
		(*data)["PubLog"] = pubLog;
	}

	glass3::util::log(
			"debug",
			"output::addTrackingData(): New tracking data: "
					+ json::Serialize(*data));

	// add, cache handles updates
	return (m_TrackingCache->addToCache(data, id));
}

// ---------------------------------------------------------removeTrackingData
bool output::removeTrackingData(std::shared_ptr<const json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::removetrackingdata(): Bad json object passed in.");
		return (false);
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		glass3::util::log(
				"error",
				"output::removetrackingdata(): Bad json hypo object passed in.");

		return (false);
	}

	return (removeTrackingData(ID));
}

// ---------------------------------------------------------removeTrackingData
bool output::removeTrackingData(std::string ID) {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	if (ID == "") {
		glass3::util::log(
				"error", "output::removetrackingdata(): Empty ID passed in.");
		return (false);
	}

	if (m_TrackingCache->isInCache(ID) == true) {
		return (m_TrackingCache->removeFromCache(ID));
	} else {
		return (false);
	}
}

// ---------------------------------------------------------getTrackingData
std::shared_ptr<const json::Object> output::getTrackingData(std::string id) {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	std::shared_ptr<json::Object> nullObj;
	if (id == "") {
		glass3::util::log(
				"error", "output::removetrackingdata(): Empty ID passed in.");
		return (nullObj);
	} else if (id == "null") {
		glass3::util::log(
				"warn", "output::removetrackingdata(): Invalid ID passed in.");
		return (nullObj);
	}

	// return the value
	if (m_TrackingCache->isInCache(id) == true) {
		return (m_TrackingCache->getFromCache(id));
	} else {
		return (nullObj);
	}
}

// ---------------------------------------------------------getNextTrackingData
std::shared_ptr<const json::Object> output::getNextTrackingData() {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	// get the data
	std::shared_ptr<const json::Object> data =
			m_TrackingCache->getNextFromCache(true);

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

// ---------------------------------------------------------haveTrackingData
bool output::haveTrackingData(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::havetrackingdata(): Bad json object passed in.");
		return (false);
	}

	std::string ID;
	if ((*data).HasKey("ID")) {
		ID = (*data)["ID"].ToString();
	} else if ((*data).HasKey("Pid")) {
		ID = (*data)["Pid"].ToString();
	} else {
		glass3::util::log(
				"error",
				"output::havetrackingdata(): Bad json hypo object passed in.");
		return (false);
	}

	return (haveTrackingData(ID));
}

// ---------------------------------------------------------haveTrackingData
bool output::haveTrackingData(std::string ID) {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	if (ID == "") {
		glass3::util::log("error",
							"output::haveTrackingData(): Empty ID passed in.");
		return (false);
	}

	return (m_TrackingCache->isInCache(ID));
}

// ---------------------------------------------------------clearTrackingData
void output::clearTrackingData() {
	std::lock_guard<std::mutex> guard(m_TrackingCacheMutex);
	m_TrackingCache->clear();
}

// ---------------------------------------------------------checkEventsLoop
void output::checkEventsLoop() {
	// we're running
	setEventThreadState(glass3::util::ThreadState::Started);

	// run until told to stop
	while (getEventThreadState() == glass3::util::ThreadState::Started) {
		// signal that we're still running
		setEventThreadHealth();

		// see if there's anything in the tracking cache
		std::shared_ptr<const json::Object> data = getNextTrackingData();

		// got something
		if (data != NULL) {
			// get the id
			std::string id;
			if ((*data).HasKey("ID")) {
				id = (*data)["ID"].ToString();
			} else if ((*data).HasKey("Pid")) {
				id = (*data)["Pid"].ToString();
			} else {
				glass3::util::log(
						"warning",
						"output::checkEventsLoop(): Bad data object received from "
						"getNextTrackingData(), no ID, skipping data.");

				// remove the message we found from the cache, since it is bad
				removeTrackingData(data);

				// keep working
				continue;
			}

			// get the command
			std::string command;
			if ((*data).HasKey("Cmd")) {
				command = (*data)["Cmd"].ToString();
			} else {
				glass3::util::log(
						"warning",
						"output::checkEventsLoop(): Bad data object received from "
						"getNextTrackingData(), no Cmd, skipping data.");

				// remove the value we found from the cache, since it is bad
				removeTrackingData(data);

				// keep working
				continue;
			}

			// process the data based on the tracking message
			if (command == "Event") {
				// Request the hypo from associator
				if (getAssociator() != NULL) {
					// build the request
					std::shared_ptr<json::Object> datarequest =
							std::make_shared<json::Object>(json::Object());
					(*datarequest)["Cmd"] = "ReqHypo";
					(*datarequest)["Pid"] = id;

					// send the request
					getAssociator()->sendToAssociator(datarequest);
				}
			}
		}

		// give up some time at the end of the loop
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}

	glass3::util::log("info", "output::checkEventsLoop(): Stopped thread.");

	setEventThreadState(glass3::util::ThreadState::Stopped);

	// done with thread
	return;
}

// ---------------------------------------------------------work
glass3::util::WorkState output::work() {
	// request current stationlist
	if (getSiteListDelay() > 0) {
		// what time is it
		time_t tNowRequest;
		std::time(&tNowRequest);

		// every interval
		if ((tNowRequest - m_tLastSiteRequest) >= getSiteListDelay()) {
			// Request the sitelist from associator
			if (getAssociator() != NULL) {
				// build the request
				std::shared_ptr<json::Object> datarequest = std::make_shared<
						json::Object>(json::Object());
				(*datarequest)["Cmd"] = "ReqSiteList";

				// send the request
				getAssociator()->sendToAssociator(datarequest);
			}

			// this is now the last time we wrote
			m_tLastSiteRequest = tNowRequest;
		}
	}

	// null check
	if ((m_OutputQueue == NULL) || (m_LookupQueue == NULL)) {
		// no message queues means we've got big problems
		glass3::util::log(
				"critical",
				"output::work(): No m_OutputQueue and/or m_LookupQueue.");
		return (glass3::util::WorkState::Error);
	}

	// first see what we're supposed to do with a new message
	// see if there's an output in the message queue
	std::shared_ptr<json::Object> message = m_OutputQueue->getDataFromQueue();
	// int outputQueueSize = m_OutputQueue->size();
	// int lookupQueueSize = m_LookupQueue->size();

	// if there's no output, check for a lookup
	if (message == NULL) {
		message = m_LookupQueue->getDataFromQueue();
	}

	if (message == NULL) {
		return (glass3::util::WorkState::Idle);
	}

	// if we got something
	/*glass3::util::log(
	 "debug",
	 "associator::dispatch(): got message:"
	 + json::Serialize(*message)
	 + " from associator. (outputQueueSize:"
	 + std::to_string(outputQueueSize) + ", lookupQueueSize:"
	 + std::to_string(lookupQueueSize) + ")");*/

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
		glass3::util::log(
				"critical",
				"output::work(): BAD message passed in, no Cmd/Type found.");
		return (glass3::util::WorkState::Error);
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
		std::shared_ptr<const json::Object> trackingData = getTrackingData(
				messageid);

		if (trackingData == NULL) {
			glass3::util::log("critical",
								"output::work(): NULL tracking data for Hypo.");
			return (glass3::util::WorkState::Error);
		}

		glass3::util::log(
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
		m_ThreadPool->addJob(std::bind(&output::writeOutput, this, message));
		m_iHypoCounter++;
	} else if (messagetype == "SiteList") {
		// glass has a stationlist to write to disk
		glass3::util::log(
				"debug",
				"output::work(): Outputting a " + messagetype + " message.");

		// write out the stationlist
		// using the threadpool
		m_ThreadPool->addJob(std::bind(&output::writeOutput, this, message));
		m_iSiteListCounter++;
	} else if (messagetype == "Event") {
		// add the event to the tracking cache
		addTrackingData(message);

		m_iEventCounter++;
	} else if (messagetype == "Cancel") {
		std::shared_ptr<const json::Object> trackingData = getTrackingData(
				messageid);

		// see if we've tracked this event
		if (trackingData != NULL) {
			// we have
			glass3::util::log(
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

				glass3::util::log(
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
		std::shared_ptr<const json::Object> trackingData = getTrackingData(
				messageid);
		// see if we've tracked this event
		if (trackingData != NULL) {
			// we have
			// glass has expired an event we have tracked
			glass3::util::log(
					"debug",
					"output::work(): Expiring event " + messageid
							+ " and removing it from tracking.");

			// check to see if there was a hypo with this expire message
			if ((message->HasKey("Hypo")) == true) {
				// check to see if this event was not finished publishing
				// or if we're configured to always send expiration
				// hypos
				if ((isDataFinished(trackingData) == false)
						|| (getPubOnExpiration() == true)) {
					// get the hypo from the event
					json::Object jsonHypo = (*message)["Hypo"];

					// make it shared
					std::shared_ptr<json::Object> hypo = std::make_shared<
							json::Object>(jsonHypo);

					// check to see if we've published this event before
					// for this check, we want to know if the current version
					// has been marked as pub
					if (isDataPublished(trackingData, false) == true) {
						(*hypo)["IsUpdate"] = true;
					} else {
						(*hypo)["IsUpdate"] = false;
					}

					glass3::util::log(
							"debug",
							"output::work(): Writing final hypo for expiring event "
									+ messageid);

					// write out the hypo to a disk file,
					// using the threadpool
					m_ThreadPool->addJob(
							std::bind(&output::writeOutput, this, hypo));
				}
			}
			// first try to remove any pending events
			// from the tracking cache
			removeTrackingData(message);
		}
		m_iExpireCounter++;
	} else if (messagetype == "SiteLookup") {
		// station info request
		glass3::util::log("debug",
							"output::work(): Writing site lookup message");
		// output immediately
		writeOutput(message);

		m_iLookupCounter++;
	} else {
		// got some other message
		glass3::util::log(
				"warning",
				"output::work(): Unknown message from glasslib: "
						+ json::Serialize(*message) + ".");
	}

	// reporting
	if ((tNow - tLastWorkReport) >= getReportInterval()) {
		if (m_iMessageCounter == 0)
			glass3::util::log(
					"warning",
					"output::work(): Received NO messages from associator "
							"thread in "
							+ std::to_string(
									static_cast<int>(tNow) - tLastWorkReport)
							+ " seconds.");
		else
			glass3::util::log(
					"info",
					"output::work(): Received "
							+ std::to_string(m_iMessageCounter)
							+ " messages from associator thread (events: "
							+ std::to_string(m_iEventCounter) + "; cancels: "
							+ std::to_string(m_iCancelCounter) + "; expires: "
							+ std::to_string(m_iExpireCounter) + "; hypos:"
							+ std::to_string(m_iHypoCounter) + "; lookups:"
							+ std::to_string(m_iLookupCounter) + "; sitelists:"
							+ std::to_string(m_iSiteListCounter) + ")" + " in "
							+ std::to_string(
									static_cast<int>(tNow - tLastWorkReport))
							+ " seconds. ("
							+ std::to_string(
									static_cast<double>(m_iMessageCounter)
											/ (static_cast<double>(tNow)
													- tLastWorkReport))
							+ " dps)");

		tLastWorkReport = tNow;
		m_iMessageCounter = 0;
		m_iEventCounter = 0;
		m_iCancelCounter = 0;
		m_iHypoCounter = 0;
		m_iExpireCounter = 0;
		m_iLookupCounter = 0;
		m_iSiteListCounter = 0;
	}

	// work was successful
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------writeOutput
void output::writeOutput(std::shared_ptr<json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error", "output::writeoutput(): Null json object passed in.");
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

	std::string agency = getOutputAgencyId();
	std::string author = getOutputAuthor();

	if (dataType == "Hypo") {
		// convert a hypo to a detection
		std::string detectionString = glass3::parse::hypoToJSONDetection(
				data, agency, author);

		sendOutput("Detection", ID, detectionString);
	} else if (dataType == "Cancel") {
		// convert a cancel to a retract
		std::string retractString = glass3::parse::cancelToJSONRetract(data,
																		agency,
																		author);

		sendOutput("Retraction", ID, retractString);
	} else if (dataType == "SiteLookup") {
		// convert a site lookup to a station info request
		std::string stationInfoRequestString =
				glass3::parse::siteLookupToStationInfoRequest(data, agency,
																author);

		sendOutput("StationInfoRequest", ID, stationInfoRequestString);
	} else if (dataType == "SiteList") {
		// convert a site list to a station list
		std::string stationListString = glass3::parse::siteListToStationList(
				data);

		sendOutput("StationList", ID, stationListString);
	} else {
		return;
	}
}

// ---------------------------------------------------------isDataReady
bool output::isDataReady(std::shared_ptr<const json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::isdataready(): Null tracking object passed in.");
		return (false);
	}

	if (!(data->HasKey("Cmd"))) {
		glass3::util::log(
				"error",
				"output::isdataready(): Bad tracking object passed in, "
						" missing cmd: " + json::Serialize(*data));
		return (false);
	} else if ((!(data->HasKey("PubLog"))) || (!(data->HasKey("Version")))) {
		glass3::util::log(
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
	int createTime = glass3::util::convertISO8601ToEpochTime(createTimeString);

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
	for (int i = 0; i < getPubTimes().size(); i++) {
		// get the published version for this pub time
		int pubVersion = pubLog[i].ToInt();

		// has this pub time been published at all?
		if (pubVersion > 0) {
			// yes, move on
			continue;
		}

		// has this pub time passed?
		if (tNow < (createTime + getPubTimes()[i])) {
			// no, move on
			continue;
		}

		// update pubLog for this time
		std::shared_ptr<json::Object> newData = std::make_shared<json::Object>(
				*data);
		pubLog[i] = currentVersion;
		(*newData)["PubLog"] = pubLog;

		// update data in cache
		m_TrackingCache->addToCache(newData, id);

		glass3::util::log(
				"debug",
				"output::isDataReady(): Updated data: "
						+ json::Serialize(*m_TrackingCache->getFromCache(id)));

		// depending on whether this version has already been changed
		if (changed == true) {
			glass3::util::log(
					"debug",
					"output::isdataready(): Publishing " + id + " version:"
							+ std::to_string(currentVersion) + " tNow:"
							+ std::to_string(static_cast<int>(tNow))
							+ " > (createTime + getPubTimes()[i]): "
							+ std::to_string(
									static_cast<int>((createTime
											+ getPubTimes()[i])))
							+ " (createTime: " + std::to_string(createTime)
							+ " getPubTimes()[i]: "
							+ std::to_string(static_cast<int>(getPubTimes()[i]))
							+ ")");

			// ready to publish
			return (true);
		} else {
			glass3::util::log(
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

// ---------------------------------------------------------isDataChanged
bool output::isDataChanged(std::shared_ptr<const json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::isDataChanged(): Null tracking object passed in.");
		return (false);
	}

	if (!(data->HasKey("Cmd"))) {
		glass3::util::log(
				"error",
				"output::isDataChanged(): Bad tracking object passed in, "
						" missing cmd: " + json::Serialize(*data));
		return (false);
	} else if ((!(data->HasKey("PubLog"))) || (!(data->HasKey("Version")))) {
		glass3::util::log(
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

// ---------------------------------------------------------isDataPublished
bool output::isDataPublished(std::shared_ptr<const json::Object> data,
								bool ignoreVersion) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::isDataPublished(): Null json data object passed in.");
		return (false);
	}

	if ((!(data->HasKey("Cmd"))) || (!(data->HasKey("PubLog")))
			|| (!(data->HasKey("Version")))) {
		glass3::util::log(
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

// ---------------------------------------------------------isDataFinished
bool output::isDataFinished(std::shared_ptr<const json::Object> data) {
	if (data == NULL) {
		glass3::util::log(
				"error",
				"output::isDataFinished(): Null json data object passed in.");
		return (false);
	}

	if ((!(data->HasKey("Cmd"))) || (!(data->HasKey("PubLog")))
			|| (!(data->HasKey("Version")))) {
		glass3::util::log(
				"error",
				"output::isDataFinished(): Bad json hypo object passed in "
						" missing Cmd, PubLog or Version "
						+ json::Serialize(*data));
		return (false);
	}

	// get the pub log
	json::Array pubLog = (*data)["PubLog"].ToArray();

	// for each entry in the pub log
	for (int i = 0; i < pubLog.size(); i++) {
		// get whether this one was published
		int pubVersion = pubLog[i].ToInt();

		// pub version less than 1 means not published
		// which means not finished
		if (pubVersion < 1) {
			return (false);
		}
	}

	// all pub log entries were greater than 0,
	// so event was finished
	return (true);
}

// ---------------------------------------------------------setOutputAgency
void output::setOutputAgency(std::string agency) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sOutputAgencyID = agency;
}

// ---------------------------------------------------------getOutputAgencyId
const std::string output::getOutputAgencyId() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sOutputAgencyID);
}

// ---------------------------------------------------------setOutputAuthor
void output::setOutputAuthor(std::string author) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sOutputAuthor = author;
}

// ---------------------------------------------------------getOutputAuthor
const std::string output::getOutputAuthor() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sOutputAuthor);
}

// ---------------------------------------------------------setSiteListDelay
void output::setSiteListDelay(int delay) {
	m_iSiteListDelay = delay;
}

// ---------------------------------------------------------getSiteListDelay
int output::getSiteListDelay() {
	return (m_iSiteListDelay);
}

// ---------------------------------------------------------setStationFile
void output::setStationFile(std::string filename) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sStationFile = filename;
}

// ---------------------------------------------------------getStationFile
const std::string output::getStationFile() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sStationFile);
}

// ---------------------------------------------------------setReportInterval
void output::setReportInterval(int interval) {
	m_iReportInterval = interval;
}

// ---------------------------------------------------------getReportInterval
int output::getReportInterval() {
	return (m_iReportInterval);
}

// ---------------------------------------------------------setAssociator
void output::setAssociator(glass3::util::iAssociator* associator) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_Associator = associator;
}

// ---------------------------------------------------------getAssociator
glass3::util::iAssociator* output::getAssociator() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_Associator);
}

// ---------------------------------------------------------setPubOnExpiration
void output::setPubOnExpiration(bool pub) {
	m_bPubOnExpiration = pub;
}

// ---------------------------------------------------------getPubOnExpiration
int output::getPubOnExpiration() {
	return (m_bPubOnExpiration);
}

// ---------------------------------------------------------getPubTimes
std::vector<int> output::getPubTimes() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_PublicationTimes);
}

// ---------------------------------------------------------setPubTimes
void output::setPubTimes(std::vector<int> pubTimes) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_PublicationTimes = pubTimes;
}

// ---------------------------------------------------------addPubTime
void output::addPubTime(int pubTime) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_PublicationTimes.push_back(pubTime);
}

// ---------------------------------------------------------clearPubTimes
void output::clearPubTimes() {
	std::lock_guard<std::mutex> guard(getMutex());
	m_PublicationTimes.clear();
}

// ---------------------------------------------------------setEventLastHealthy
void output::setEventLastHealthy(std::time_t now) {
	m_tEventLastHealthy = now;
}

// ---------------------------------------------------------getEventLastHealthy
time_t output::getEventLastHealthy() {
	return ((std::time_t) m_tEventLastHealthy);
}

// ---------------------------------------------------------setEventThreadState
void output::setEventThreadState(glass3::util::ThreadState status) {
	m_bEventThreadState = status;
}

// ---------------------------------------------------------getEventThreadState
glass3::util::ThreadState output::getEventThreadState() {
	return (m_bEventThreadState);
}

}  // namespace output
}  // namespace glass3
