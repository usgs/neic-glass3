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

namespace glass {

class outputTopic {
 public:
	outputTopic(hazdevbroker::Producer * producer);
	~outputTopic();

	bool setup(json::Value &config);
	void clear();

	bool isInBounds(double lat, double lon);

	void send(const std::string &message);

	double m_dTopLatitude;
	double m_dBottomLatitude;
	double m_dLeftLongitude;
	double m_dRightLongitude;

	std::string m_sTopicName;
	RdKafka::Topic * m_OutputTopic;
	hazdevbroker::Producer * m_OutputProducer;
};

/**
 * \brief glass output class
 *
 * The glass output class is a thread class encapsulating the detection output
 * logic.  The output class handles output messages from from glasscore,
 * and writes the messages out to disk.
 *
 * output inherits from the threadbaseclass class.
 * output implements the ioutput interface.
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
	explicit brokerOutput(std::shared_ptr<json::Object> &config);

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
	 * The this function configures the brokerOutput class, and the tracking cache it
	 * contains.
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
	 * Clears all configuration, clears and reallocates the message queue and
	 * cache
	 */
	void clear() override;

	void setStationFileName(const std::string &filename);

	const std::string getStationFileName();

	/**
	 * \brief the function for producer logging
	 * \param message - A string containing the logging message
	 */
	void logProducer(const std::string &message);

 protected:
	/**
	 * \brief output file writing function
	 *
	 * The function used output detection data
	 *
	 * \param data - A pointer to a json::Object containing the data to be
	 * output.
	 */
	void sendOutput(const std::string &type, const std::string &id,
					const std::string &message) override;

	void sendToOutputTopics(const std::string &message);

 private:
	/**
	 * \brief pointer to the glass3::util::threadpool used to queue and
	 * perform output.
	 */
	glass3::util::ThreadPool * m_ThreadPool;

	hazdevbroker::Producer * m_OutputProducer;
	std::vector<outputTopic*> m_vOutputTopics;

	hazdevbroker::Producer * m_StationRequestProducer;
	RdKafka::Topic * m_StationRequestTopic;

	/**
	 * \brief the std::string containing the station file name.
	 */
	std::string m_sStationFileName;

	/**
	 * \brief the mutex for configuration
	 */
	std::mutex m_BrokerOutputConfigMutex;
};
}  // namespace glass
#endif  // BROKEROUTPUT_H
