/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef OUTPUTTOPIC_H
#define OUTPUTTOPIC_H

#include <json.h>
#include <Producer.h>

#include <string>

namespace glass3 {

/**
 * \brief glass3 output topic class
 *
 * The glass3 output topic class is a class that encapsulates a broker output
 * topic plus the bounds for which a message should be sent via that topic.
 */
class outputTopic {
 public:
	/**
	 * \brief outputTopic constructor
	 *
	 * The outputTopic for the brokerOutput class.
	 * Initializes members to default values, sets the hazdevbroker::Producer
	 * pointer.
	 * \param producer - A pointer to a hazdevbroker::Producer to use for the
	 * topic.
	 */
	explicit outputTopic(hazdevbroker::Producer * producer);

	/**
	 * \brief outputTopic destructor
	 *
	 * The destructor for the outputTopic class.
	 */
	~outputTopic();

	/**
	 * \brief outputTopic configuration function
	 *
	 * The this function configures the outputTopic class
	 *
	 * \param config - A pointer to a json::Value containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(const json::Value &config);

	/**
	 * \brief outputTopic clear function
	 *
	 * The clear function for the outputTopic class.
	 * Clears all configuration
	 */
	void clear();

	/**
	 * \brief outputTopic bounds check function
	 *
	 * This function checks to see if the provided coordinates are within
	 * the bounds for this outputTopic.
	 * \param lat - a double value containing the latitude to check in degrees
	 * \param lon - a double value containing the longitude to check in degrees
	 * \return Returns true if the provided coordinates are within the bounds,
	 * false otherwise
	 */
	bool isInBounds(double lat, double lon);

	/**
	 * \brief outputTopic send function
	 * The function sends the provided message using the producer pointer and
	 * the configured topic.
	 * \param message - A string containing the message
	 */
	void send(const std::string &message);

 protected:
	/**
	 * \brief the top of the bounds rectangle in degrees of latitude.
	 */
	double m_dTopLatitude;

	/**
	 * \brief the bottom of the bounds rectangle in degrees of latitude.
	 */
	double m_dBottomLatitude;

	/**
	 * \brief the left side of the bounds rectangle in degrees of longitude.
	 */
	double m_dLeftLongitude;

	/**
	 * \brief the right side of the bounds rectangle in degrees of longitude.
	 */
	double m_dRightLongitude;

	/**
	 * \brief the std::string containing the topic name
	 */
	std::string m_sTopicName;

 private:
	/**
	 * \brief the output topic pointer
	 */
	RdKafka::Topic * m_OutputTopic;

	/**
	 * \brief the output producer
	 */
	hazdevbroker::Producer * m_OutputProducer;
};
}  // namespace glass3
#endif  // OUTPUTTOPIC_H
