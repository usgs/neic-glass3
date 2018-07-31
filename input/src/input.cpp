#include <input.h>
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
namespace input {

// ---------------------------------------------------------input
input::input()
		: glass3::util::ThreadBaseClass("input", 1) {
	glass3::util::log("debug", "input::input(): Construction.");

	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = NULL;

	clear();
}

input::input(std::shared_ptr<const json::Object> config)
		: glass3::util::ThreadBaseClass("input", 1) {
	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = NULL;

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

// ---------------------------------------------------------~input
input::~input() {
	glass3::util::log("debug", "input::~input(): Destruction.");

	// stop the input thread
	stop();

	if (m_DataQueue != NULL) {
		// clear the queue
		m_DataQueue->clear();

		delete (m_DataQueue);
	}

	if (m_GPickParser != NULL)
		delete (m_GPickParser);

	if (m_JSONParser != NULL)
		delete (m_JSONParser);

	if (m_CCParser != NULL)
		delete (m_CCParser);
}

// ---------------------------------------------------------setup
bool input::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::log("error",
							"input::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::log("debug", "input::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		glass3::util::log("error",
							"input::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"].ToString();
		if (configtype != "GlassInput") {
			glass3::util::log(
					"error",
					"input::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// default agencyid
	if (!(config->HasKey("DefaultAgencyID"))) {
		// agencyid is optional
		setDefaultAgency("US");
		glass3::util::log("info",
							"input::setup(): Defaulting to US as AgencyID.");
	} else {
		setDefaultAgency((*config)["DefaultAgencyID"].ToString());
		glass3::util::log(
				"info",
				"input::setup(): Using AgencyID: " + getDefaultAgencyId()
						+ " as default.");
	}

	// default author
	if (!(config->HasKey("DefaultAuthor"))) {
		// author is optional
		setDefaultAuthor("glassConverter");
		glass3::util::log(
				"info",
				"input::setup(): Defaulting to glassConverter as Author.");
	} else {
		setDefaultAuthor((*config)["DefaultAuthor"].ToString());
		glass3::util::log(
				"info",
				"input::setup(): Using Author: " + getDefaultAuthor()
						+ " as default.");
	}

	// queue max size
	if (!(config->HasKey("QueueMaxSize"))) {
		// queue max size is optional
		setQueueMaxSize(-1);
		glass3::util::log(
				"info",
				"input::setup(): Defaulting to -1 for QueueMaxSize (no maximum "
				"queue size).");
	} else {
		setQueueMaxSize((*config)["QueueMaxSize"].ToInt());
		glass3::util::log(
				"info",
				"input::setup(): Using QueueMaxSize: "
						+ std::to_string(getQueueMaxSize()) + ".");
	}

	if (m_GPickParser != NULL) {
		delete (m_GPickParser);
	}
	m_GPickParser = new glass3::parse::GPickParser(m_sDefaultAgencyID,
													m_sDefaultAuthor);

	if (m_JSONParser != NULL) {
		delete (m_JSONParser);
	}
	m_JSONParser = new glass3::parse::JSONParser(m_sDefaultAgencyID,
													m_sDefaultAuthor);

	if (m_CCParser != NULL) {
		delete (m_CCParser);
	}
	m_CCParser = new glass3::parse::CCParser(m_sDefaultAgencyID,
												m_sDefaultAuthor);

	if (m_DataQueue != NULL) {
		delete (m_DataQueue);
	}
	m_DataQueue = new glass3::util::Queue();

	glass3::util::log("debug", "input::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::util::BaseClass::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void input::clear() {
	glass3::util::log("debug", "input::clear(): clearing configuration.");

	setDefaultAgency("");
	setDefaultAuthor("");
	setQueueMaxSize(-1);

	if (m_DataQueue != NULL)
		m_DataQueue->clear();

	// finally do baseclass clear
	glass3::util::BaseClass::clear();
}

// ---------------------------------------------------------getInputData
std::shared_ptr<json::Object> input::getInputData() {
	if (m_DataQueue == NULL) {
		return (NULL);
	}

	// just get the value from the queue
	return (m_DataQueue->getDataFromQueue());
}

// ---------------------------------------------------------getInputDataCount
int input::getInputDataCount() {
	if (m_DataQueue == NULL) {
		return (-1);
	}

	return (m_DataQueue->size());
}

// ---------------------------------------------------------work
glass3::util::WorkState input::work() {
	// check to see if we have room
	if ((getQueueMaxSize() != -1)
			&& (getInputDataCount() >= getQueueMaxSize())) {
		// we don't, yet
		return (glass3::util::WorkState::Idle);
	}

	// get next data
	std::string message = fetchRawData();

	if (message == "") {
		return (glass3::util::WorkState::Idle);
	}

	glass3::util::log("trace", "input::work(): Got message: " + message);
	std::string type = getDataType(message);
	std::shared_ptr<json::Object> newdata;
	try {
		newdata = parse(type, message);
	} catch (const std::exception &e) {
		glass3::util::log(
				"debug",
				"input::work(): Exception:" + std::string(e.what())
						+ " parsing string: " + message);
	}

	if ((newdata != NULL) && (validate(type, newdata) == true)) {
		m_DataQueue->addDataToQueue(newdata);
	}

	// work was successful
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------parse
std::shared_ptr<json::Object> input::parse(std::string type,
											std::string input) {
	// choose the parser based on the type
	// global pick
	if (((type == GPICK_TYPE) || (type == GPICKS_TYPE))
			&& (m_GPickParser != NULL))
		return (m_GPickParser->parse(input));
	// all json formats share the same parser
	else if ((type.find(JSON_TYPE) != std::string::npos)
			&& (m_JSONParser != NULL))
		return (m_JSONParser->parse(input));
	// cc data
	else if ((type == CC_TYPE) && (m_CCParser != NULL))
		return (m_CCParser->parse(input));
	else
		return (NULL);
}

// ---------------------------------------------------------validate
bool input::validate(std::string type, std::shared_ptr<json::Object> input) {
	// choose the validator based on the type
	// global pick
	if (((type == GPICK_TYPE) || (type == GPICKS_TYPE))
			&& (m_GPickParser != NULL))
		return (m_GPickParser->validate(input));
	// all json formats share the same validator
	else if ((type.find(JSON_TYPE) != std::string::npos)
			&& (m_JSONParser != NULL))
		return (m_JSONParser->validate(input));
	// cc data
	else if ((type == CC_TYPE) && (m_CCParser != NULL))
		return (m_CCParser->validate(input));
	else
		return (false);
}

// ---------------------------------------------------------setDefaultAgency
void input::setDefaultAgency(std::string agency) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sDefaultAgencyID = agency;
}

// ---------------------------------------------------------getDefaultAgencyId
const std::string input::getDefaultAgencyId() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sDefaultAgencyID);
}

// ---------------------------------------------------------setDefaultAuthor
void input::setDefaultAuthor(std::string author) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sDefaultAuthor = author;
}

// ---------------------------------------------------------getDefaultAuthor
const std::string input::getDefaultAuthor() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sDefaultAuthor);
}

// ---------------------------------------------------------setQueueMaxSize
void input::setQueueMaxSize(int size) {
	m_QueueMaxSize = size;
}

// ---------------------------------------------------------getQueueMaxSize
int input::getQueueMaxSize() {
	return (m_QueueMaxSize);
}

// ---------------------------------------------------------setReportInterval
void input::setReportInterval(int interval) {
	m_iReportInterval = interval;
}

// ---------------------------------------------------------getReportInterval
int input::getReportInterval() {
	return (m_iReportInterval);
}

}  // namespace input
}  // namespace glass3
