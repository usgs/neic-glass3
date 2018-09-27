#include "outputTopic.h"
#include <json.h>  // NOLINT(build/include)
#include <logger.h>
#include <Producer.h>

#include <string>

namespace glass3 {

// ---------------------------------------------------------outputTopic
outputTopic::outputTopic(hazdevbroker::Producer * producer) {
	m_OutputTopic = NULL;

	clear();
	m_OutputProducer = producer;
}

// ---------------------------------------------------------~outputTopic
outputTopic::~outputTopic() {
	clear();
}

// ---------------------------------------------------------setup
bool outputTopic::setup(const json::Value &config) {
	if (config.GetType() != json::ValueType::ObjectVal) {
		glass3::util::Logger::log("error",
									"outputTopic::setup(): Bad configuration.");
		return (false);
	}

	json::Object configuration = config.ToObject();

	// name
	if (configuration.HasKey("TopicName")
			&& (configuration["TopicName"].GetType()
					== json::ValueType::StringVal)) {
		m_sTopicName = configuration["TopicName"].ToString();
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

	return (true);
}

// ---------------------------------------------------------clear
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

// ---------------------------------------------------------isInBounds
bool outputTopic::isInBounds(double lat, double lon) {
	// check bounds
	if ((lat >= m_dBottomLatitude) && (lon >= m_dLeftLongitude)
			&& (lat <= m_dTopLatitude) && (lon <= m_dRightLongitude)) {
		return (true);
	}
	return (false);
}

// ---------------------------------------------------------send
void outputTopic::send(const std::string &message) {
	// nullchecks
	if (m_OutputProducer == NULL) {
		return;
	}
	if (m_OutputTopic == NULL) {
		return;
	}

	glass3::util::Logger::log(
			"debug",
			"outputTopic::send(): Sent message to topic: " + m_sTopicName);

	// send it
	m_OutputProducer->sendString(m_OutputTopic, message);
}
}  // namespace glass3
