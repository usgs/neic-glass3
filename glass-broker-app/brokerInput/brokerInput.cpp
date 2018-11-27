#include <brokerInput.h>
#include <json.h>  // NOLINT(build/include)
#include <logger.h>
#include <fileutil.h>
#include <vector>
#include <queue>
#include <memory>
#include <chrono>
#include <mutex>
#include <string>
#include <limits>

namespace glass3 {

// ---------------------------------------------------------brokerInput
brokerInput::brokerInput()
		: glass3::input::Input() {
	glass3::util::Logger::log("debug",
								"brokerInput::brokerInput(): Construction.");

	m_Consumer = NULL;
	m_iBrokerHeartbeatInterval = -1;

	clear();
}

// ---------------------------------------------------------brokerInput
brokerInput::brokerInput(const std::shared_ptr<const json::Object> &config)
		: glass3::input::Input() {
	m_Consumer = NULL;
	m_iBrokerHeartbeatInterval = -1;

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the brokerInput thread
	start();
}

// ---------------------------------------------------------~brokerInput
brokerInput::~brokerInput() {
	// cleanup
	if (m_Consumer != NULL) {
		delete (m_Consumer);
	}
}

// ---------------------------------------------------------setup
bool brokerInput::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::Logger::log(
				"error", "brokerInput::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::Logger::log("debug", "brokerInput::setup(): Setting Up.");

	// Configuration
	if (!(config->HasKey("Configuration"))) {
		glass3::util::Logger::log(
				"error", "brokerInput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"].ToString();
		if (configtype != "GlassInput") {
			glass3::util::Logger::log(
					"error",
					"brokerInput::setup(): Wrong configuration provided, "
					"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// consumer config
	std::string consumerConfig = "";
	if (!(config->HasKey("HazdevBrokerConfig"))) {
		// consumer config is required
		consumerConfig = "";
		glass3::util::Logger::log(
				"error",
				"brokerInput::setup(): Required configuration value "
				"HazdevBrokerConfig not specified.");
		return (false);
	} else {
		consumerConfig = json::Serialize(
				(*config)["HazdevBrokerConfig"].ToObject());
		glass3::util::Logger::log(
				"info",
				"brokerInput::setup(): Using HazdevBrokerConfig: "
						+ consumerConfig + ".");
	}

	// topic config
	std::string topicConfig = "";
	if (!(config->HasKey("HazdevBrokerTopicConfig"))) {
		// consumer config is required
		topicConfig = "";
		glass3::util::Logger::log(
				"info",
				"brokerInput::setup(): Using default topic configuration.");
	} else {
		topicConfig = json::Serialize(
				(*config)["HazdevBrokerTopicConfig"].ToObject());
		glass3::util::Logger::log(
				"info",
				"brokerInput::setup(): Using HazdevBrokerTopicConfig: "
						+ topicConfig + ".");
	}

	// consumer topics
	std::vector<std::string> topicList;
	if (!(config->HasKey("Topics"))) {
		// topics are required
		glass3::util::Logger::log(
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

	// optional directory to write heartbeat files to
	std::string heartbeatDirectory = "";
	if (config->HasKey("HeartbeatDirectory")) {
		std::string heartbeatDirectory =
			(*config)["HeartbeatDirectory"].ToString();
		m_Consumer->setHeartbeatDirectory(heartbeatDirectory);
		glass3::util::Logger::log(
				"info",
				"brokerInput::setup(): Using HeartbeatDirectory: "
						+ heartbeatDirectory + ".");
	}

	// optional interval to check for heartbeats
	if (config->HasKey("BrokerHeartbeatInterval")) {
		m_iBrokerHeartbeatInterval =
			(*config)["BrokerHeartbeatInterval"].ToInt();
		glass3::util::Logger::log(
				"info",
				"brokerInput::setup(): Using BrokerHeartbeatInterval: "
						+ std::to_string(m_iBrokerHeartbeatInterval) + ".");
	}

	// set up logging
	m_Consumer->setLogCallback(
			std::bind(&brokerInput::logConsumer, this, std::placeholders::_1));

	// set up consumer, set up default topic config
	m_Consumer->setup(consumerConfig, topicConfig);

	// subscribe to topics
	m_Consumer->subscribe(topicList);

	glass3::util::Logger::log("debug",
								"brokerInput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::input::Input::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void brokerInput::clear() {
	glass3::util::Logger::log(
			"debug", "brokerInput::clear(): clearing configuration.");

	// finally do baseclass clear
	glass3::input::Input::clear();
}

// ---------------------------------------------------------fetchRawData
std::string brokerInput::fetchRawData(std::string* pOutType) {
	// we only expect json messages from the broker
	*pOutType = std::string(JSON_TYPE);

	// make sure we have a consumer
	if (m_Consumer == NULL) {
		return ("");
	}

	// if we are checking heartbeat times
	if (m_iBrokerHeartbeatInterval >= 0) {
		// get current time in seconds
		int64_t timeNow = std::time(NULL);

		// get last heartbeat time
		int64_t lastHB = m_Consumer->getLastHeartbeatTime();

		// calculate elapsed time
		int64_t elapsedTime = timeNow - lastHB;

		// has it been too long since the last heartbeat?
		if (elapsedTime > m_iBrokerHeartbeatInterval) {
			glass3::util::Logger::log("error",
				"brokerInput::fetchRawData: No Heartbeat Message seen from"
				" topic(s) in " +
				std::to_string(m_iBrokerHeartbeatInterval) + " seconds! (" +
				std::to_string(elapsedTime) + ")");

			// reset last heartbeat time so that we don't fill the log
			m_Consumer->setLastHeartbeatTime(timeNow);
		}
	}

	// get message from the consumer
	std::string message = m_Consumer->pollString(100);

	// check for empty string
	if (message != "") {
		return (message);
	}

	// 'till next time
	return ("");
}

// ---------------------------------------------------------logConsumer
void brokerInput::logConsumer(const std::string &message) {
	glass3::util::Logger::log("debug",
								"brokerInput::logConsumer(): " + message);
}
}  // namespace glass3
