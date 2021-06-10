#include "outputTopic.h"
#include <json.h>  // NOLINT(build/include)
#include <logger.h>
#include <Producer.h>
#include <geo.h>

#include <string>

namespace glass3 {

// ---------------------------------------------------------outputTopic
outputTopic::outputTopic(hazdevbroker::Producer * producer) {
	m_OutputTopic = NULL;
	m_OutputProducer = NULL;

	m_iReportInterval = 300;
	std::time(&tLastPerformanceReport);
	m_dSendTime = 0;
	m_dRunningSendTimeAverage = -1;
	m_iRunningAverageCounter = 0;
	m_iMessageCounter = 0;

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

	// now look for the optional Lat/Lon box used to restrict the detection
	// messages sent via this topic.  If a key is not specified, use the
	// geographic bounds (90/-180/-90/180) as the default
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
		m_dTopLatitude = glass3::util::Geo::k_MaximumLatitude;
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
		m_dLeftLongitude = glass3::util::Geo::k_MinimumLongitude;
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
		m_dBottomLatitude = glass3::util::Geo::k_MinimumLatitude;
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
		m_dRightLongitude = glass3::util::Geo::k_MaximumLongitude;
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
	m_dTopLatitude = glass3::util::Geo::k_MaximumLatitude;
	m_dBottomLatitude = glass3::util::Geo::k_MinimumLatitude;
	m_dLeftLongitude = glass3::util::Geo::k_MinimumLongitude;
	m_dRightLongitude = glass3::util::Geo::k_MaximumLongitude;

	m_sTopicName = "";
	if (m_OutputTopic != NULL) {
		delete (m_OutputTopic);
		m_OutputTopic = NULL;
	}

	if (m_OutputProducer != NULL) {
		delete (m_OutputProducer);
		m_OutputProducer = NULL;
	}
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
	std::chrono::high_resolution_clock::time_point tSendStartTime =
			std::chrono::high_resolution_clock::now();

	m_OutputProducer->sendString(m_OutputTopic, message);

	std::chrono::high_resolution_clock::time_point tSendEndTime =
		std::chrono::high_resolution_clock::now();

	// compute the time sending
	double currentSendTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tSendEndTime - tSendStartTime)
			.count();

	m_iMessageCounter++;
	m_dSendTime += currentSendTime;

	// don't alert if our average is not going yet
	if ((m_iRunningAverageCounter > 1) &&
		(currentSendTime > 2*(m_dRunningSendTimeAverage))) {
		glass3::util::Logger::log("warning",
				"outputTopic::send(): Time sending a message to topic "
				+ m_sTopicName +
				" took two times longer than average: "
				+ std::to_string(currentSendTime)
				+ " vs "
				+ std::to_string(m_dRunningSendTimeAverage));
	}

	// do a performance report if it's time
	std::time_t tNow;
	std::time(&tNow);
	if ((tNow - tLastPerformanceReport) >= m_iReportInterval) {
		// calculate send time average since the last report
		// we need a default so use the last send time
		double sendTimeAverage = currentSendTime;

		// avoid startup wierdness (first report)
		if (m_dRunningSendTimeAverage < 0) {
			m_dRunningSendTimeAverage = currentSendTime;
		}
		if ((m_iRunningAverageCounter > 0)  && (m_iMessageCounter > 0)) {
			sendTimeAverage = static_cast<double>(m_dSendTime)
				/ static_cast<double>(m_iMessageCounter);
		}

		// calculate running average of the data per second
		m_iRunningAverageCounter++;
		if (m_iRunningAverageCounter == 1) {
			m_dRunningSendTimeAverage = sendTimeAverage;
		}
		m_dRunningSendTimeAverage = (m_dRunningSendTimeAverage
				* (m_iRunningAverageCounter - 1) + sendTimeAverage)
				/ m_iRunningAverageCounter;

		// log the report
		glass3::util::Logger::log(
				"info",
				"outputTopic::send(): Topic: "
						+ m_sTopicName
						+ "; current send time: "
						+ std::to_string(currentSendTime) +
						"; average send time: "
						+ std::to_string(sendTimeAverage)
						+ "; running send time average: "
						+ std::to_string(m_dRunningSendTimeAverage)
						+ "; number of messages sent: "
						+ std::to_string(m_iMessageCounter));

		tLastPerformanceReport = tNow;
		m_dSendTime = 0;
		m_iMessageCounter = 0;
	}
}

// ---------------------------------------------------------heartbeat
void outputTopic::heartbeat() {
	if (m_OutputProducer == NULL) {
		return;
	}
	if (m_OutputTopic == NULL) {
		return;
	}

	// send it
	m_OutputProducer->sendHeartbeat(m_OutputTopic);
}
}  // namespace glass3
