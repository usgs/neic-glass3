/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BROKERINPUT_H
#define BROKERINPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <input.h>
#include <Consumer.h>

#include <vector>
#include <queue>
#include <mutex>
#include <string>
#include <memory>

namespace glass3 {

/**
 * \brief glass3 broker input class
 *
 * The glass3 broker input class is a class encapsulating the broker input
 * logic.  The output class handles setting up a hazdevbroker consumer,
 * configuring input topic(s) and getting messages from kafka via the
 * hazdevbroker consumer to glasscore via the associator class.
 *
 * brokerInput inherits from the glass3::input::Input class.
 */
class brokerInput : public glass3::input::Input {
 public:
	/**
	 * \brief brokerInput constructor
	 *
	 * The constructor for the brokerInput class.
	 * Initializes members to default values.
	 */
	brokerInput();

	/**
	 * \brief brokerInput advanced constructor
	 *
	 * The advanced constructor for the brokerInput class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 * \param config - A json::Object pointer to the configuration to use
	 */
	explicit brokerInput(const std::shared_ptr<const json::Object> &config);

	/**
	 * \brief brokerInput destructor
	 *
	 * The destructor for the brokerInput class.
	 * Stops the work thread
	 */
	~brokerInput();

	/**
	 * \brief brokerInput configuration function
	 *
	 * This function configures the brokerInput class.
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief output clear function
	 *
	 * The clear function for the output class.
	 * Clears all configuration
	 */
	void clear() override;

 protected:
	/**
	 * \brief get input data string and type
	 *
	 * A function (overridden from glass3::input) that that retrieves the next
	 * data message and type from an input source
	 * \param pOutType - A pointer to a std::string used to pass out the type of
	 * the data
	 * \return returns a std::string containing the input data message
	 */
	std::string fetchRawData(std::string* pOutType) override;

	/**
	 * \brief the function for consumer logging
	 * \param message - A string containing the logging message
	 */
	void logConsumer(const std::string &message);

 private:
	/**
	 * \brief the hazdevbroker consumer object to get messages from kafka
	 */
	hazdevbroker::Consumer * m_Consumer;

	/**
	 * \brief the interval in seconds to expect hazdevbroker heartbeats.
	 */
	int m_iBrokerHeartbeatInterval;

	/**
	 * \brief Double holding the time spent polling since the last  since
	 * the last informational report.
	 */
	double m_dPollTime;

	/**
	 * \brief Integer holding the count of times the running average has been
	 * computed, used to compute the running average of time to poll 
	 */
	int m_iRunningAverageCounter;

	/**
	 * \brief Double holding the the running average of time to poll used
	 * in the information report
	 */
	double m_dRunningPollTimeAverage;

	/**
	 * \brief Integer holding the count of messages received since
	 * the last informational report.
	 */
	int m_iMessageCounter;

	/**
	 * \brief The time the last informational report was generated.
	 */
	time_t tLastPerformanceReport;

	/**
	 * \brief Information Report interval
	 *
	 * An integer containing the interval (in seconds) between
	 * logging informational reports.
	 */
	int m_iReportInterval;
};
}  // namespace glass3
#endif  // BROKERINPUT_H
