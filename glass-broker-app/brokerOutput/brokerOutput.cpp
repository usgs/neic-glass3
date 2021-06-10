#include <brokerOutput.h>
#include <json.h>  // NOLINT(build/include)
#include <convert.h>
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <output.h>
#include <Producer.h>

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <limits>

#include "outputTopic.h"

namespace glass3 {

// ---------------------------------------------------------brokerOutput
brokerOutput::brokerOutput()
		: glass3::output::output() {
	glass3::util::Logger::log("debug",
								"brokerOutput::brokerOutput(): Construction.");

	m_StationRequestProducer = NULL;
	m_StationRequestTopic = NULL;

	// init config to defaults and allocate
	clear();
}

// ---------------------------------------------------------brokerOutput
brokerOutput::brokerOutput(const std::shared_ptr<json::Object> &config)
		: glass3::output::output() {
	glass3::util::Logger::log(
			"debug", "brokerOutput::brokerOutput(): Advanced Construction.");

	m_StationRequestProducer = NULL;
	m_StationRequestTopic = NULL;

	// init config to defaults and allocate
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

// ---------------------------------------------------------~brokerOutput
brokerOutput::~brokerOutput() {
	// cleanup
	for (auto aTopic : m_vOutputTopics) {
		if (aTopic != NULL) {
			delete (aTopic);
		}
	}
	m_vOutputTopics.clear();

	if(m_StationRequestProducer != NULL) {
		delete(m_StationRequestProducer);
	}

	if(m_StationRequestTopic != NULL) {
		delete(m_StationRequestTopic);
	}
}

// ---------------------------------------------------------setup
bool brokerOutput::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::Logger::log(
				"error",
				"brokerOutput::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::Logger::log("debug", "brokerOutput::setup(): Setting Up.");

	// Configuration
	if (!(config->HasKey("Configuration"))) {
		glass3::util::Logger::log(
				"error", "brokerOutput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"];
		if (configtype != "GlassOutput") {
			glass3::util::Logger::log(
					"error",
					"brokerOutput::setup(): Wrong configuration provided, "
							"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	getMutex().lock();

	// StationFileName
	if (!(config->HasKey("StationFileName")
			&& ((*config)["StationFileName"].GetType()
					== json::ValueType::StringVal))) {
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): StationFileName not specified.");
		m_sStationFileName = "";
	} else {
		m_sStationFileName = (*config)["StationFileName"].ToString();

		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using StationFileName: "
						+ m_sStationFileName + ".");
	}

	// producer config
	std::string producerConfig = "";
	if (!(config->HasKey("HazdevBrokerConfig"))) {
		// consumer config is required
		producerConfig = "";
		glass3::util::Logger::log(
				"error", "brokerOutput::setup(): Required configuration value "
				"HazdevBrokerConfig not specified.");
		return (false);
	} else {
		producerConfig = json::Serialize(
				(*config)["HazdevBrokerConfig"].ToObject());
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using HazdevBrokerConfig: "
						+ producerConfig + ".");
	}

	// producer heartbeat interval
	int brokerHeartbeatInterval = -1;
	if (config->HasKey("BrokerHeartbeatInterval")) {
		brokerHeartbeatInterval =
			(*config)["BrokerHeartbeatInterval"].ToInt();

		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using BrokerHeartbeatInterval: "
						+ std::to_string(brokerHeartbeatInterval) + ".");
	}

	// topic config
	std::string topicConfig = "";
	if (!(config->HasKey("HazdevBrokerTopicConfig"))) {
		// consumer config is required
		topicConfig = "";
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using default topic configuration.");
	} else {
		topicConfig = json::Serialize(
				(*config)["HazdevBrokerTopicConfig"].ToObject());
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using HazdevBrokerTopicConfig: "
						+ topicConfig + ".");
	}

	// clear out any old topics
	for (auto aTopic : m_vOutputTopics) {
		if (aTopic != NULL) {
			delete (aTopic);
		}
	}
	m_vOutputTopics.clear();

	// load the topics from config
	if ((config->HasKey("OutputTopics"))
			&& ((*config)["OutputTopics"].GetType() == json::ValueType::ArrayVal)) {
		json::Array topics = (*config)["OutputTopics"].ToArray();

		// check that there are topics
		if (topics.size() == 0) {
			glass3::util::Logger::log(
					"error",
					"brokerOutput::setup(): No OutputTopics specified.");
			return (false);
		}

		// parse topics
		for (auto aTopicConfig : topics) {
			// create producer for the topic
			hazdevbroker::Producer * topicProducer = new hazdevbroker::Producer();
			topicProducer->setHeartbeatInterval(brokerHeartbeatInterval);
			topicProducer->setLogCallback(
					std::bind(&brokerOutput::logProducer, this, std::placeholders::_1));
			topicProducer->setup(producerConfig, topicConfig);

			// create output topic using the producer
			outputTopic* newTopic = new outputTopic(topicProducer);

			// setup output topic
			if (newTopic->setup(aTopicConfig) == true) {
				// add output topic to list
				m_vOutputTopics.push_back(newTopic);
			} else {
				// failure, cleanup
				newTopic->clear();
				delete (newTopic);

				glass3::util::Logger::log(
						"error",
						"brokerOutput::setup(): Failed set up output topic.");
				return (false);
			}
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"brokerOutput::setup(): Required configuration value OutputTopics not "
				"specified.");
		return (false);
	}

	// optional station lookup
	std::string stationRequestTopic = "";
	if (!(config->HasKey("StationRequestTopic"))) {
		// topics are required
		stationRequestTopic = "";
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): StationRequestTopic not specified, will not "
				"request station lookups.");
	} else {
		stationRequestTopic = (*config)["StationRequestTopic"].ToString();

		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using StationRequestTopic: "
						+ stationRequestTopic + ".");
	}

	// set up station request topic if wanted
	if (m_StationRequestTopic != NULL) {
		delete (m_StationRequestTopic);
	}
	if (stationRequestTopic != "") {
		// clear out any old request producer
		if (m_StationRequestProducer != NULL) {
			delete (m_StationRequestProducer);
		}

		// create new request producer
		m_StationRequestProducer = new hazdevbroker::Producer();

		// set up logging
		m_StationRequestProducer->setLogCallback(
				std::bind(&brokerOutput::logProducer, this, std::placeholders::_1));

		// set up request producer
		m_StationRequestProducer->setup(producerConfig, topicConfig);

		// create topic
		m_StationRequestTopic = m_StationRequestProducer->createTopic(
				stationRequestTopic);
	} else {
		m_StationRequestTopic = NULL;
	}

	// unlock our configuration
	getMutex().unlock();

	glass3::util::Logger::log("debug",
								"brokerOutput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	glass3::output::output::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void brokerOutput::clear() {
	glass3::util::Logger::log(
			"debug", "brokerOutput::clear(): clearing configuration.");

	setStationFileName("");

	// finally do baseclass clear
	glass3::output::output::clear();
}

// ---------------------------------------------------------sendOutput
void brokerOutput::sendOutput(const std::string &type, const std::string &id,
								const std::string &message) {
	if (type == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty type passed in.");
		return;
	}

	if (id == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty id passed in.");
		return;
	}

	if (message == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty message passed in.");
		return;
	}

	// handle special cases
	if (type == "StationInfoRequest") {
		// station info requests get their own special topic
		if (m_StationRequestTopic != NULL) {
			m_StationRequestProducer->sendString(m_StationRequestTopic, message);
		}
	} else if (type == "StationList") {
		// station lists are written to disk
		std::string filename = getStationFileName();

		if (filename != "") {
			// write to disk
			// back up file if it exists
			if (std::ifstream(filename)) {
				std::string backupname = filename + ".bak";

				// delete old backup
				if (std::ifstream(backupname)) {
					std::remove(backupname.c_str());
				}

				// rename existing file
				std::rename(filename.c_str(), backupname.c_str());
			}

			// create new file
			std::ofstream outfile;
			outfile.open(filename, std::ios::out);

			// write station list string to file
			try {
				outfile << message;
			} catch (const std::exception &e) {
				glass3::util::Logger::log(
						"error",
						"brokerOutput::writebrokerOutput: Problem writing "
								"station list data to disk: "
								+ std::string(e.what()) + " Problem Data: "
								+ message);
				return;
			}

			// done
			outfile.close();
		}
	} else {
		// send everything else out via the output topics
		sendToOutputTopics(message);
	}
}

// ---------------------------------------------------------sendToOutputTopics
void brokerOutput::sendToOutputTopics(const std::string &message) {
	// nullchecks
	if (message == "") {
		return;
	}

	// we need to check the message for it's type (and it's lat/lon if it's
	// a detection
	// deserialize the string into JSON
	json::Value deserializedJSON = json::Deserialize(message);

	// make sure we got valid json
	if (deserializedJSON.GetType() == json::ValueType::NULLVal) {
		return;
	}

	// convert to json object
	json::Object messageObject = deserializedJSON.ToObject();

	// check type
	std::string type = "";
	if (messageObject.HasKey("Type")
			&& ((messageObject)["Type"].GetType() == json::ValueType::StringVal)) {
		type = (messageObject)["Type"].ToString();
	} else {
		glass3::util::Logger::log(
				"error",
				"brokerOutput::sendToOutputTopics: Message missing type.");
		return;
	}

	std::string id = "";
	if (messageObject.HasKey("ID")
			&& ((messageObject)["ID"].GetType() == json::ValueType::StringVal)) {
		id = (messageObject)["ID"].ToString();
	}

	// handle based on type
	if (type == "Detection") {
		// detections have a lat/lon to filter which topic to send to
		// get the hypocenter from the message
		if ((messageObject.HasKey("Hypocenter"))
				&& ((messageObject)["Hypocenter"].GetType()
						== json::ValueType::ObjectVal)) {
			// get the hypocenter object
			json::Object hypocenter = (messageObject)["Hypocenter"].ToObject();

			// get latitude from hypocenter
			double lat = 0;
			if (hypocenter.HasKey("Latitude")
					&& (hypocenter["Latitude"].GetType()
							== json::ValueType::DoubleVal)) {
				lat = hypocenter["Latitude"].ToDouble();
			} else {
				glass3::util::Logger::log(
						"error",
						"brokerOutput::sendToOutputTopics: Detection Message "
						"Hypocenter Latitude");

				return;
			}

			// get longitude from hypocenter
			double lon = 0;
			if (hypocenter.HasKey("Longitude")
					&& (hypocenter["Longitude"].GetType()
							== json::ValueType::DoubleVal)) {
				lon = hypocenter["Longitude"].ToDouble();
			} else {
				glass3::util::Logger::log(
						"error",
						"brokerOutput::sendToOutputTopics: Detection Message "
						"Hypocenter Longitude");

				return;
			}

			// for each output topic we have
			for (auto aTopic : m_vOutputTopics) {
				// does this topic want this detection
				if (aTopic->isInBounds(lat, lon) == true) {
					// yes, send it
					aTopic->send(message);
				}
			}
			glass3::util::Logger::log(
					"debug",
					"brokerOutput::sendToOutputTopics: Detection Message "
					+ id +
					"written to topics");
		} else {
			glass3::util::Logger::log(
					"error",
					"brokerOutput::sendToOutputTopics: Detection Message missing "
					"Hypocenter");
			return;
		}
	} else if (type == "Retract") {
		// retractions don't have a lat/lon, so just send to all topics?
		// for each topic
		for (auto aTopic : m_vOutputTopics) {
			// send it
			aTopic->send(message);
		}

		glass3::util::Logger::log(
					"debug",
					"brokerOutput::sendToOutputTopics: Retraction Message "
					+ id +
					"written to topics");
	}
}

// ---------------------------------------------------------sendHeartbeat
void brokerOutput::sendHeartbeat() {
	// send heartbeats to each topic
	// for each topic
	for (auto aTopic : m_vOutputTopics) {
		// send it
		aTopic->heartbeat();
	}
}

// ---------------------------------------------------------logProducer
void brokerOutput::logProducer(const std::string &message) {
	// log whatever the producer wanted us to log
	glass3::util::Logger::log("debug",
								"brokerOutput::logProducer(): " + message);
}

// ---------------------------------------------------------setStationFileName
void brokerOutput::setStationFileName(const std::string &filename) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sStationFileName = filename;
}

// ---------------------------------------------------------getStationFileName
const std::string brokerOutput::getStationFileName() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sStationFileName);
}

// ---------------------------------------------------------getMutex
std::mutex & brokerOutput::getMutex() {
	return (m_Mutex);
}
}  // namespace glass3
