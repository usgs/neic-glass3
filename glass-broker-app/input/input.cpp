#include <input.h>
#include <logger.h>
#include <fileutil.h>
#include <timeutil.h>

#include <vector>
#include <queue>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <string>

namespace glass {

input::input()
		: util::ThreadBaseClass("input", 100) {
	logger::log("debug", "input::input(): Construction.");

	m_DataQueue = NULL;
	m_Consumer = NULL;
	m_JSONParser = NULL;

	clear();
}

input::input(int linesleepms)
		: util::ThreadBaseClass("input", 100) {
	m_DataQueue = NULL;
	m_Consumer = NULL;
	m_JSONParser = NULL;

	clear();
}

input::input(json::Object *config, int linesleepms)
		: util::ThreadBaseClass("input", 100) {
	m_DataQueue = NULL;
	m_Consumer = NULL;
	m_JSONParser = NULL;

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

input::~input() {
	logger::log("debug", "input::~input(): Destruction.");

	// stop the input thread
	stop();

	if (m_DataQueue != NULL) {
		// clear the queue
		m_DataQueue->clearQueue();

		delete (m_DataQueue);
	}

	if (m_Consumer != NULL) {
		delete (m_Consumer);
	}

	if (m_JSONParser != NULL) {
		delete (m_JSONParser);
	}
}

// configuration
bool input::setup(json::Object *config) {
	if (config == NULL) {
		logger::log("error", "input::setup(): NULL configuration passed in.");
		return (false);
	}

	logger::log("debug", "input::setup(): Setting Up.");

	// Configuration
	if (!(config->HasKey("Configuration"))) {
		logger::log("error", "input::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"].ToString();
		if (configtype != "glass-broker-app-input") {
			logger::log(
					"error",
					"input::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	// consumer config
	std::string consumerConfig = "";
	if (!(config->HasKey("HazdevBrokerConfig"))) {
		// consumer config is required
		consumerConfig = "";
		logger::log(
				"error",
				"input::setup(): Required configuration value HazdevBrokerConfig not "
				"specified.");
		return (false);
	} else {
		consumerConfig = json::Serialize(
				(*config)["HazdevBrokerConfig"].ToObject());
		logger::log(
				"info",
				"input::setup(): Using HazdevBrokerConfig: " + consumerConfig
						+ ".");
	}

	// topic config
	std::string topicConfig = "";
	if (!(config->HasKey("HazdevBrokerTopicConfig"))) {
		// consumer config is required
		topicConfig = "";
		logger::log("info",
					"input::setup(): Using default topic configuration.");
	} else {
		topicConfig = json::Serialize(
				(*config)["HazdevBrokerTopicConfig"].ToObject());
		logger::log(
				"info",
				"input::setup(): Using HazdevBrokerTopicConfig: " + topicConfig
						+ ".");
	}

	// consumer topics
	if (!(config->HasKey("Topics"))) {
		// topics are required
		logger::log("error",
					"input::setup(): Required configuration value Topics not "
					"specified.");
		return (false);
	} else {
		json::Array topicsArray = (*config)["Topics"].ToArray();
		for (int i = 0; i < topicsArray.size(); i++) {
			m_sTopicList.push_back(topicsArray[i].ToString());
		}
	}

	// default agencyid
	if (!(config->HasKey("DefaultAgencyID"))) {
		// agencyid is optional
		m_sDefaultAgencyID = "US";
		logger::log("info", "input::setup(): Defaulting to US as AgencyID.");
	} else {
		m_sDefaultAgencyID = (*config)["DefaultAgencyID"].ToString();
		logger::log(
				"info",
				"input::setup(): Using AgencyID: " + m_sDefaultAgencyID
						+ " as default.");
	}

	// default author
	if (!(config->HasKey("DefaultAuthor"))) {
		// agencyid is optional
		m_sDefaultAuthor = "glassConverter";
		logger::log("info",
					"input::setup(): Defaulting to glassConverter as Author.");
	} else {
		m_sDefaultAuthor = (*config)["DefaultAuthor"].ToString();
		logger::log(
				"info",
				"input::setup(): Using Author: " + m_sDefaultAuthor
						+ " as default.");
	}

	// queue max size
	if (!(config->HasKey("QueueMaxSize"))) {
		// agencyid is optional
		m_QueueMaxSize = -1;
		logger::log(
				"info",
				"input::setup(): Defaulting to -1 for QueueMaxSize (no maximum "
				"queue size).");
	} else {
		m_QueueMaxSize = (*config)["QueueMaxSize"].ToInt();
		logger::log(
				"info",
				"input::setup(): Using QueueMaxSize: "
						+ std::to_string(m_QueueMaxSize) + ".");
	}

	// unlock our configuration
	m_ConfigMutex.unlock();

	// set up consumer
	if (m_Consumer != NULL)
		delete (m_Consumer);

	// create new consumer
	m_Consumer = new hazdevbroker::Consumer();

	// set up logging
	m_Consumer->setLogCallback(
			std::bind(&input::logConsumer, this, std::placeholders::_1));

	// set up consumer, set up default topic config
	m_Consumer->setup(consumerConfig, topicConfig);

	// subscribe to topics
	m_Consumer->subscribe(m_sTopicList);

	if (m_JSONParser != NULL)
		delete (m_JSONParser);
	m_JSONParser = new parse::JSONParser(m_sDefaultAgencyID, m_sDefaultAuthor);

	if (m_DataQueue != NULL)
		delete (m_DataQueue);
	m_DataQueue = new util::Queue();

	logger::log("debug", "input::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	util::BaseClass::setup(config);

	// we're done
	return (true);
}

void input::clear() {
	logger::log("debug", "input::clear(): clearing configuration.");

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	m_sTopicList.clear();
	m_sDefaultAgencyID = "";
	m_sDefaultAuthor = "";
	m_QueueMaxSize = -1;

	// unlock our configuration
	m_ConfigMutex.unlock();

	if (m_DataQueue != NULL)
		m_DataQueue->clearQueue();

	// finally do baseclass clear
	util::BaseClass::clear();
}

// get next data from input
json::Object* input::getData() {
	// just get the value from the queue
	return (m_DataQueue->getDataFromQueue());
}

int input::dataCount() {
	if (m_DataQueue == NULL) {
		return (-1);
	}

	return (m_DataQueue->size());
}

bool input::work() {
	// make sure we have a consumer
	if (m_Consumer == NULL)
		return (false);

	std::string message = m_Consumer->pollString(100);

	if (message != "") {
		logger::log("trace", "input::work(): Got message: " + message);
		json::Object* newdata = NULL;
		try {
			newdata = m_JSONParser->parse(message);
		} catch (const std::exception &e) {
			logger::log(
					"debug",
					"input::work(): Exception:" + std::string(e.what())
							+ " parsing string: " + message);
		}

		if ((newdata != NULL) && (m_JSONParser->validate(newdata) == true)) {
			m_DataQueue->addDataToQueue(newdata);
		}
	}

	// work was successful
	return (true);
}

void input::logConsumer(const std::string &message) {
	logger::log("debug", "input::logConsumer(): " + message);
}
}  // namespace glass
