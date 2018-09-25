#include <brokerOutput.h>
#include <json.h>
#include <convert.h>
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <output.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

namespace glass {

brokerOutput::brokerOutput()
		: glass3::output::output() {
	glass3::util::Logger::log("debug",
								"brokerOutput::brokerOutput(): Construction.");

	m_OutputProducer = NULL;
	m_StationRequestProducer = NULL;
	m_OutputTopic = NULL;
	m_StationRequestTopic = NULL;

	// init config to defaults and allocate
	clear();
}

brokerOutput::brokerOutput(std::shared_ptr<json::Object> &config)
		: glass3::output::output() {
	glass3::util::Logger::log(
			"debug", "brokerOutput::brokerOutput(): Advanced Construction.");

	m_OutputProducer = NULL;
	m_StationRequestProducer = NULL;
	m_OutputTopic = NULL;
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

	// producer topics
	// brokerOutput
	std::string brokerOutputTopic = "";
	if (!(config->HasKey("OutputTopic"))) {
		// topics are required
		brokerOutputTopic = "";
		glass3::util::Logger::log(
				"error",
				"brokerOutput::setup(): Required configuration value OutputTopic not "
				"specified.");
		return (false);
	} else {
		brokerOutputTopic = (*config)["OutputTopic"].ToString();

		glass3::util::Logger::log(
				"info",
				"brokerOutput::setup(): Using OutputTopic: " + brokerOutputTopic
						+ ".");
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

	// set up brokerOutput producer
	if (m_OutputProducer != NULL)
		delete (m_OutputProducer);
	if (m_OutputTopic != NULL)
		delete (m_OutputTopic);

	// create new producer
	m_OutputProducer = new hazdevbroker::Producer();

	// set up logging
	m_OutputProducer->setLogCallback(
			std::bind(&brokerOutput::logProducer, this, std::placeholders::_1));

	// set up producer
	m_OutputProducer->setup(producerConfig, topicConfig);

	// create topic
	m_OutputTopic = m_OutputProducer->createTopic(brokerOutputTopic);

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

	if (type == "Detection") {
		if ((m_OutputProducer != NULL) && (m_OutputTopic != NULL)) {
			m_OutputProducer->sendString(m_OutputTopic, message);
		}
	} else if (type == "Retraction") {
		if ((m_OutputProducer != NULL) && (m_OutputTopic != NULL)) {
			m_OutputProducer->sendString(m_OutputTopic, message);
		}
	} else if (type == "StationInfoRequest") {
		if ((m_StationRequestProducer != NULL)
				&& (m_StationRequestTopic != NULL)) {
			m_StationRequestProducer->sendString(m_StationRequestTopic,
													message);
		}
	} else if (type == "StationList") {
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
		return;
	}
}

void brokerOutput::logProducer(const std::string &message) {
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
}  // namespace glass
