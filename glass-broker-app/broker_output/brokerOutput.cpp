#include <brokerOutput.h>
#include <json.h>
#include <convert.h>
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <output.h>
#include <Producer.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <limits>
#include <cmath>

namespace glass {

outputTopic::outputTopic(hazdevbroker::Producer * producer) {
	m_OutputTopic = NULL;

	clear();
	m_OutputProducer = producer;
}

outputTopic::~outputTopic() {
	clear();
}

bool outputTopic::setup(json::Value &config) {
	glass3::util::Logger::log("debug",
								"outputTopic::setup(): Setting up a topic.");

	if (config.GetType() != json::ValueType::ObjectVal) {
		return (false);
	}

	json::Object configuration = config.ToObject();

	// name
	if (configuration.HasKey("Name")
			&& (configuration["Name"].GetType() == json::ValueType::StringVal)) {
		m_sTopicName = configuration["Name"].ToString();
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ ".");
	} else {
		glass3::util::Logger::log(
				"error", "outputTopic::setup(): Missing required topic name.");
		return (false);
	}

	// check producer
	if (m_OutputProducer == NULL) {
		glass3::util::Logger::log(
				"error",
				"outputTopic::setup(): invalid producer " + m_sTopicName);
		return (false);
	}

	// create topic
	m_OutputTopic = m_OutputProducer->createTopic(m_sTopicName);
	if (m_OutputTopic == NULL) {
		glass3::util::Logger::log(
				"error",
				"outputTopic::setup(): failed to create topic " + m_sTopicName);
		return (false);
	}

	glass3::util::Logger::log("debug", "outputTopic::setup(): created topic.");

	// top lat
	if (configuration.HasKey("TopLatitude")
			&& (configuration["TopLatitude"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dTopLatitude = configuration["TopLatitude"].ToDouble();
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " TopLatitude: " + std::to_string(m_dTopLatitude)
						+ ".");
	} else {
		m_dTopLatitude = 90.0;
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " TopLatitude: " + std::to_string(m_dTopLatitude)
						+ ".");
	}

	// left lon
	if (configuration.HasKey("LeftLongitude")
			&& (configuration["LeftLongitude"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dLeftLongitude = configuration["LeftLongitude"].ToDouble();
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " LeftLongitude: " + std::to_string(m_dLeftLongitude)
						+ ".");
	} else {
		m_dLeftLongitude = -180.0;
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " LeftLongitude: " + std::to_string(m_dLeftLongitude)
						+ ".");
	}

	// bottom lat
	if (configuration.HasKey("BottomLatitude")
			&& (configuration["BottomLatitude"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dBottomLatitude = configuration["BottomLatitude"].ToDouble();
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " BottomLatitude: "
						+ std::to_string(m_dBottomLatitude) + ".");
	} else {
		m_dBottomLatitude = -90.0;
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " BottomLatitude: "
						+ std::to_string(m_dBottomLatitude) + ".");
	}

	// right lon
	if (configuration.HasKey("RightLongitude")
			&& (configuration["RightLongitude"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dRightLongitude = configuration["RightLongitude"].ToDouble();
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " RightLongitude: "
						+ std::to_string(m_dRightLongitude) + ".");
	} else {
		m_dRightLongitude = 180.0;
		glass3::util::Logger::log(
				"info",
				"outputTopic::setup(): Setting up topic: " + m_sTopicName
						+ " RightLongitude: "
						+ std::to_string(m_dRightLongitude) + ".");
	}

	glass3::util::Logger::log(
			"debug", "outputTopic::setup(): Done Setting up a topic.");

	return (true);
}

void outputTopic::clear() {
	m_dTopLatitude = 90.0;
	m_dBottomLatitude = -90.0;
	m_dLeftLongitude = -180.0;
	m_dRightLongitude = 180.0;

	m_sTopicName = "";
	if (m_OutputTopic != NULL) {
		delete (m_OutputTopic);
		m_OutputTopic = NULL;
	}

	m_OutputProducer = NULL;
}

bool outputTopic::isInBounds(double lat, double lon) {
	// check bounds
	if ((lat >= m_dBottomLatitude) && (lon >= m_dLeftLongitude)
			&& (lat <= m_dTopLatitude) && (lon <= m_dRightLongitude)) {
		return (true);
	}
	return (false);
}

void outputTopic::send(const std::string &message) {
	if (m_OutputProducer == NULL) {
		return;
	}
	if (m_OutputTopic == NULL) {
		return;
	}
	if (message == "") {
		return;
	}
	glass3::util::Logger::log(
			"debug",
			"outputTopic::send(): Sending to topic: " + m_sTopicName + ".");

	m_OutputProducer->sendString(m_OutputTopic, message);
}

brokerOutput::brokerOutput()
		: glass3::output::output() {
	glass3::util::Logger::log("debug",
								"brokerOutput::brokerOutput(): Construction.");

	m_StationRequestProducer = NULL;
	m_OutputProducer = NULL;
	m_StationRequestTopic = NULL;

	// init config to defaults and allocate
	clear();
}

brokerOutput::brokerOutput(std::shared_ptr<json::Object> &config)
		: glass3::output::output() {
	glass3::util::Logger::log(
			"debug", "brokerOutput::brokerOutput(): Advanced Construction.");

	m_StationRequestProducer = NULL;
	m_OutputProducer = NULL;
	m_StationRequestTopic = NULL;

	// init config to defaults and allocate
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

brokerOutput::~brokerOutput() {
	glass3::util::Logger::log("debug",
								"brokerOutput::~brokerOutput(): Destruction.");

	// stop the input thread
	stop();
}

// configuration
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
	m_BrokerOutputConfigMutex.lock();

	// StationFileName
	if (!(config->HasKey("StationFileName")
			&& ((*config)["StationFileName"].GetType()
					== json::ValueType::StringVal))) {
		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): StationFileName not specified.");
		setStationFileName("");
	} else {
		setStationFileName((*config)["StationFileName"].ToString());

		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using StationFileName: "
						+ getStationFileName() + ".");
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

	// clear out any old
	if (m_OutputProducer != NULL) {
		delete (m_OutputProducer);
	}

	// create new producer
	m_OutputProducer = new hazdevbroker::Producer();

	// set up logging
	m_OutputProducer->setLogCallback(
			std::bind(&brokerOutput::logProducer, this, std::placeholders::_1));

	// set up producer
	m_OutputProducer->setup(producerConfig, topicConfig);

	// producer topics
	// output
	for (auto aTopic : m_vOutputTopics) {
		if (aTopic != NULL) {
			delete (aTopic);
		}
	}
	m_vOutputTopics.clear();

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
			glass3::util::Logger::log(
					"debug", "brokerOutput::setup(): got output topic.");

			// create output topic
			outputTopic* newTopic = new outputTopic(m_OutputProducer);

			// setup output topic
			if (newTopic->setup(aTopicConfig) == true) {

				// add output tpic to list
				m_vOutputTopics.push_back(newTopic);

				glass3::util::Logger::log(
						"debug", "brokerOutput::setup(): added output topic.");
			} else {
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

	// unlock our configuration
	m_BrokerOutputConfigMutex.unlock();

	// set up station request producer if wanted
	if (stationRequestTopic != "") {
		if (m_StationRequestProducer != NULL)
			delete (m_StationRequestProducer);
		if (m_StationRequestTopic != NULL)
			delete (m_StationRequestTopic);

		// create new producer
		m_StationRequestProducer = new hazdevbroker::Producer();

		// set up logging
		m_StationRequestProducer->setLogCallback(
				std::bind(&brokerOutput::logProducer, this,
							std::placeholders::_1));

		// set up producer
		// same producer config as brokerOutput
		m_StationRequestProducer->setup(producerConfig, topicConfig);

		// create topic
		// different topic
		m_StationRequestTopic = m_StationRequestProducer->createTopic(
				stationRequestTopic);
	} else {
		m_StationRequestProducer = NULL;
		m_StationRequestTopic = NULL;
	}

	glass3::util::Logger::log("debug",
								"brokerOutput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	glass3::output::output::setup(config);

	// we're done
	return (true);
}

void brokerOutput::clear() {
	glass3::util::Logger::log(
			"debug", "brokerOutput::clear(): clearing configuration.");

	setStationFileName("");

	// finally do baseclass clear
	glass3::output::output::clear();
}

// handle output
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

	if (type == "StationInfoRequest") {
		// station info requests get their own special topic
		if ((m_StationRequestProducer != NULL)
				&& (m_StationRequestTopic != NULL)) {
			m_StationRequestProducer->sendString(m_StationRequestTopic,
													message);
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
		sendToOutputTopics(message);
	}
}

void brokerOutput::sendToOutputTopics(const std::string &message) {
	// nullchecks
	if (message == "") {
		return;
	}

	// deserialize the string into JSON
	json::Value deserializedJSON = json::Deserialize(message);

	// make sure we got valid json
	if (deserializedJSON.GetType() == json::ValueType::NULLVal) {
		return;
	}
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

	// handle based on type
	if (type == "Detection") {
		// detections have a lat/lon to filter which topic to send to
		if ((messageObject.HasKey("Hypocenter"))
				&& ((messageObject)["Hypocenter"].GetType()
						== json::ValueType::ObjectVal)) {
			// get object
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
			// for each output topic
			for (auto aTopic : m_vOutputTopics) {
				// does this topic want this detection
				if (aTopic->isInBounds(lat, lon) == true) {
					// send it
					aTopic->send(message);
				}
			}
		} else {
			glass3::util::Logger::log(
					"error",
					"brokerOutput::sendToOutputTopics: Detection Message missing "
					"Hypocenter");
			return;
		}

	} else if (type == "Retraction") {
		// retractions don't have a lat/lon, so just send to all topics
		// for each topic
		for (auto aTopic : m_vOutputTopics) {
			// send it
			aTopic->send(message);
		}
	}
}

void brokerOutput::logProducer(const std::string &message) {
	glass3::util::Logger::log("debug",
								"outputTopic::logProducer(): " + message);
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
}  // namespace glass
