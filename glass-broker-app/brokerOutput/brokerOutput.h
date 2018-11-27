/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BROKEROUTPUT_H
#define BROKEROUTPUT_H

#include <json.h>
#include <output.h>
#include <Producer.h>

#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include "outputTopic.h"

namespace glass3 {

/**
 * \brief glass3 broker output class
 *
 * The glass3 broker output class is a class encapsulating the broker output
 * logic.  The output class handles setting up a hazdevbroker producer,
 * configuring output topic(s) and sending messages from glasscore, to
 * kafka via the hazdevbroker producer
 *
 * brokerOutput inherits from the glass3::output::output class.
 */
class brokerOutput : public glass3::output::output {
 public:
	/**
	 * \brief brokerOutput constructor
	 *
	 * The constructor for the brokerOutput class.
	 * Initializes members to default values.
	 */
	brokerOutput();

	/**
	 * \brief brokerOutput advanced constructor
	 *
	 * The advanced constructor for the brokerOutput class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 */
	explicit brokerOutput(const std::shared_ptr<json::Object> &config);

	/**
	 * \brief brokerOutput destructor
	 *
	 * The destructor for the brokerOutput class.
	 * Stops the work thread
	 */
	~brokerOutput();

	/**
	 * \brief brokerOutput configuration function
	 *
	 * This function configures the brokerOutput class.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief brokerOutput clear function
	 *
	 * The clear function for the brokerOutput class.
	 * Clears all configuration
	 */
	void clear() override;

	/**
	 * \brief Sets the station file name
	 * \param filename - A string containing the file name
	 */
	void setStationFileName(const std::string &filename);

	/**
	 * \brief Gets the station file name
	 * \return Returns A string containing the file name
	 */
	const std::string getStationFileName();

	/**
	 * \brief the function for producer logging
	 * This function is used by the HazDevBroker producer to log messages
	 * with neic-glass3's logging system. 
	 * \param message - A string containing the logging message
	 */
	void logProducer(const std::string &message);

 protected:
	/**
	 * \brief output sending function
	 *
	 * The function used output detection data
	 *
	 * \param type - A std::string containing the type of the data message
	 * \param id - A std::string containing the id of the data message
	 * \param message - A std::string containing the data message
	 */
	void sendOutput(const std::string &type, const std::string &id,
					const std::string &message) override;

	/**
	 * \brief Sends the provided message to each of the output topics
	 * \param message - A string containing the message
	 */
	void sendToOutputTopics(const std::string &message);

	/**
	 * \brief Send heartbeats
	 *
	 * This function is used to optionally send heartbeat messages to kafka via
	 * the hazdev-broker producer.
	 */
	void sendHeartbeat() override;

 private:
	/**
	 * \brief The hazdevbroker producer used to send messages to kafka
	 */
	hazdevbroker::Producer * m_OutputProducer;

	/**
	 * \brief A vector containing the output topics
	 */
	std::vector<glass3::outputTopic*> m_vOutputTopics;

	/**
	 * \brief The optional station request topic
	 */
	RdKafka::Topic * m_StationRequestTopic;

	/**
	 * \brief the std::string containing the station file name.
	 */
	std::string m_sStationFileName;

	/**
	 * \brief Retrieves a reference to the class member containing the mutex
	 * used to control access to class members
	 */
	std::mutex & getMutex();

	/**
	 * \brief A mutex to control access to class members
	 */
	std::mutex m_Mutex;
};
}  // namespace glass3
#endif  // BROKEROUTPUT_H
