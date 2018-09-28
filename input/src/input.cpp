#include <input.h>
#include <json.h>
#include <convert.h>
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

// JSON Keys
#define CONFIG_KEY "Configuration"

namespace glass3 {
namespace input {

// ---------------------------------------------------------Input
Input::Input()
		: glass3::util::ThreadBaseClass("input") {
	glass3::util::Logger::log("debug", "Input::Input(): Construction.");

	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = new glass3::util::Queue();

	clear();
}

// ---------------------------------------------------------Input
Input::Input(std::shared_ptr<const json::Object> config)
		: glass3::util::ThreadBaseClass("input") {
	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = new glass3::util::Queue();

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the Input thread
	start();
}

// ---------------------------------------------------------~Input
Input::~Input() {
	glass3::util::Logger::log("debug", "Input::~Input(): Destruction.");

	// stop the Input thread
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
bool Input::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::Logger::log("error",
							"Input::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::Logger::log("debug", "Input::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey(CONFIG_KEY)
			&& ((*config)[CONFIG_KEY].GetType() == json::ValueType::StringVal))) {
		glass3::util::Logger::log("error",
							"Input::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)[CONFIG_KEY].ToString();
		if (configtype != "GlassInput") {
			glass3::util::Logger::log(
					"error",
					"Input::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// default agencyid
	if (!(config->HasKey("DefaultAgencyID")
			&& ((*config)["DefaultAgencyID"].GetType()
					== json::ValueType::StringVal))) {
		glass3::util::Logger::log(
				"error", "Input::setup(): Missing required DefaultAgencyID.");
		return (false);
	} else {
		setDefaultAgencyId((*config)["DefaultAgencyID"].ToString());
		glass3::util::Logger::log(
				"info",
				"Input::setup(): Using AgencyID: " + getDefaultAgencyId()
						+ " as default.");
	}

	// default author
	if (!(config->HasKey("DefaultAuthor")
			&& ((*config)["DefaultAuthor"].GetType()
					== json::ValueType::StringVal))) {
		glass3::util::Logger::log("error",
							"Input::setup(): Missing required DefaultAuthor.");
		return (false);
	} else {
		setDefaultAuthor((*config)["DefaultAuthor"].ToString());
		glass3::util::Logger::log(
				"info",
				"Input::setup(): Using Author: " + getDefaultAuthor()
						+ " as default.");
	}

	// queue max size
	if (!(config->HasKey("QueueMaxSize")
			&& ((*config)["QueueMaxSize"].GetType() == json::ValueType::IntVal))) {
		// queue max size is optional
		setInputDataMaxSize(-1);
		glass3::util::Logger::log(
				"info",
				"Input::setup(): Defaulting to -1 for QueueMaxSize (no maximum "
				"queue size).");
	} else {
		setInputDataMaxSize((*config)["QueueMaxSize"].ToInt());
		glass3::util::Logger::log(
				"info",
				"Input::setup(): Using QueueMaxSize: "
						+ std::to_string(getInputDataMaxSize()) + ".");
	}

	// need to (re)create the parsers to use the agency id / author
	if (m_GPickParser != NULL) {
		delete (m_GPickParser);
	}
	m_GPickParser = new glass3::parse::GPickParser(getDefaultAgencyId(),
													getDefaultAuthor());

	if (m_JSONParser != NULL) {
		delete (m_JSONParser);
	}
	m_JSONParser = new glass3::parse::JSONParser(getDefaultAgencyId(),
													getDefaultAuthor());

	if (m_CCParser != NULL) {
		delete (m_CCParser);
	}
	m_CCParser = new glass3::parse::CCParser(getDefaultAgencyId(),
												getDefaultAuthor());

	glass3::util::Logger::log("debug", "Input::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::util::BaseClass::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void Input::clear() {
	glass3::util::Logger::log("debug", "Input::clear(): clearing configuration.");

	setDefaultAgencyId("");
	setDefaultAuthor("");
	setInputDataMaxSize(-1);

	if (m_DataQueue != NULL)
		m_DataQueue->clear();

	// finally do baseclass clear
	glass3::util::BaseClass::clear();
}

// ---------------------------------------------------------getInputData
std::shared_ptr<json::Object> Input::getInputData() {
	if (m_DataQueue == NULL) {
		return (NULL);
	}

	// just get the value from the queue
	return (m_DataQueue->getDataFromQueue());
}

// ---------------------------------------------------------getInputDataCount
int Input::getInputDataCount() {
	if (m_DataQueue == NULL) {
		return (-1);
	}

	return (m_DataQueue->size());
}

// ---------------------------------------------------------work
glass3::util::WorkState Input::work() {
	// check to see if we have room
	if ((getInputDataMaxSize() != -1)
			&& (getInputDataCount() >= getInputDataMaxSize())) {
		// we don't, yet
		return (glass3::util::WorkState::Idle);
	}

	// get next data
	std::string type = "";
	std::string message = fetchRawData(&type);

	if (message == "") {
		return (glass3::util::WorkState::Idle);
	}

	std::shared_ptr<json::Object> newdata;
	try {
		newdata = parse(type, message);
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"debug",
				"Input::work(): Exception:" + std::string(e.what())
						+ " processing Input: " + message);
	}

	if (newdata != NULL) {
		m_DataQueue->addDataToQueue(newdata);
	}

	// work was successful
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------parse
std::shared_ptr<json::Object> Input::parse(std::string inputType,
											std::string inputMessage) {
	// choose the parser based on the type
	if ((inputType.find(GPICK_TYPE) != std::string::npos)
			&& (m_GPickParser != NULL)) {
		// global pick
		return (m_GPickParser->parse(inputMessage));
	} else if ((inputType.find(JSON_TYPE) != std::string::npos)
			&& (m_JSONParser != NULL)) {
		// all json formats share the same parser
		return (m_JSONParser->parse(inputMessage));
	} else if ((inputType == CC_TYPE) && (m_CCParser != NULL)) {
		// cc data
		return (m_CCParser->parse(inputMessage));
	} else {
		glass3::util::Logger::log("warning",
							"Input::parse(): Unknown type " + inputType);
		return (NULL);
	}
}

// ---------------------------------------------------------setInputDataMaxSize
void Input::setInputDataMaxSize(int size) {
	m_QueueMaxSize = size;
}

// ---------------------------------------------------------getInputDataMaxSize
int Input::getInputDataMaxSize() {
	return (m_QueueMaxSize);
}

}  // namespace input
}  // namespace glass3
