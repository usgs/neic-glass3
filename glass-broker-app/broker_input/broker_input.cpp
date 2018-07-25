#include <broker_input.h>
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

brokerInput::brokerInput()
		: glass3::input::input() {
	logger::log("debug", "brokerInput::brokerInput(): Construction.");

	m_Consumer = NULL;

	clear();
}

brokerInput::brokerInput(std::shared_ptr<const json::Object> &config)
		: glass3::input::input() {
	m_Consumer = NULL;

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the brokerInput thread
	start();
}

brokerInput::~brokerInput() {
	logger::log("debug", "brokerInput::~brokerInput(): Destruction.");

	// stop the brokerInput thread
	stop();

	if (m_Consumer != NULL) {
		delete (m_Consumer);
	}
}

// configuration
bool brokerInput::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		logger::log("error",
					"brokerInput::setup(): NULL configuration passed in.");
		return (false);
	}

	logger::log("debug", "brokerInput::setup(): Setting Up.");

	// Configuration
	if (!(config->HasKey("Configuration"))) {
		logger::log("error",
					"brokerInput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"].ToString();
		if (configtype != "GlassInput") {
			logger::log(
					"error",
					"brokerInput::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// consumer config
	std::string consumerConfig = "";
	if (!(config->HasKey("HazdevBrokerConfig"))) {
		// consumer config is required
		consumerConfig = "";
		logger::log(
				"error",
				"brokerInput::setup(): Required configuration value HazdevBrokerConfig not "
				"specified.");
		return (false);
	} else {
		consumerConfig = json::Serialize(
				(*config)["HazdevBrokerConfig"].ToObject());
		logger::log(
				"info",
				"brokerInput::setup(): Using HazdevBrokerConfig: "
						+ consumerConfig + ".");
	}

	// topic config
	std::string topicConfig = "";
	if (!(config->HasKey("HazdevBrokerTopicConfig"))) {
		// consumer config is required
		topicConfig = "";
		logger::log("info",
					"brokerInput::setup(): Using default topic configuration.");
	} else {
		topicConfig = json::Serialize(
				(*config)["HazdevBrokerTopicConfig"].ToObject());
		logger::log(
				"info",
				"brokerInput::setup(): Using HazdevBrokerTopicConfig: "
						+ topicConfig + ".");
	}

	// consumer topics
	std::vector<std::string> topicList;
	if (!(config->HasKey("Topics"))) {
		// topics are required
		logger::log(
				"error",
				"brokerInput::setup(): Required configuration value Topics not "
				"specified.");
		return (false);
	} else {
		json::Array topicsArray = (*config)["Topics"].ToArray();
		for (int i = 0; i < topicsArray.size(); i++) {
			topicList.push_back(topicsArray[i].ToString());
		}
	}

	// set up consumer
	if (m_Consumer != NULL) {
		delete (m_Consumer);
	}

	// create new consumer
	m_Consumer = new hazdevbroker::Consumer();

	// set up logging
	m_Consumer->setLogCallback(
			std::bind(&brokerInput::logConsumer, this, std::placeholders::_1));

	// set up consumer, set up default topic config
	m_Consumer->setup(consumerConfig, topicConfig);

	// subscribe to topics
	m_Consumer->subscribe(topicList);

	logger::log("debug", "brokerInput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::input::input::setup(config);

	// we're done
	return (true);
}

void brokerInput::clear() {
	logger::log("debug", "brokerInput::clear(): clearing configuration.");

	// finally do baseclass clear
	glass3::util::BaseClass::clear();
}

std::string brokerInput::getDataType(std::string input) {
	// we only expect json messages from the broker
	return (std::string(JSON_TYPE));
}

std::string brokerInput::fetchRawData() {
	// make sure we have a consumer
	if (m_Consumer == NULL)
		return ("");

	// get message from the consumer
	std::string message = m_Consumer->pollString(100);

	// check for empty string
	if (message != "") {
		return(message);
	}

	// 'till next time
	return ("");
}

void brokerInput::logConsumer(const std::string &message) {
	logger::log("debug", "brokerInput::logConsumer(): " + message);
}
}  // namespace glass
