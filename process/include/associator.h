/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef ASSOCIATOR_H
#define ASSOCIATOR_H

#include <json.h>
#include <IGlassSend.h>
#include <inputinterface.h>
#include <outputinterface.h>
#include <associatorinterface.h>
#include <threadbaseclass.h>
#include <queue.h>
#include <ctime>
#include <memory>

namespace glass3 {
namespace process {
/**
 * \brief glass association l
 *
 * The glass association class is a thread class hosting the glass core
 * association and nucleation engine.  The associator class pulls input data
 * from the input class, and sends it into glasscore.  It also routes any
 * results to the output class. The class also sends any configuration into glasscore.
 *
 * associator inherits from the glass3::util::ThreadBaseClass class.
 *
 * associator implements the glasscore::IGlassSend and glass3::util::iAssociator
 * interfaces.
 */
class Associator : public glasscore::IGlassSend,
		public glass3::util::iAssociator, public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief associator constructor
	 *
	 * Parameterized constructor for the associator class, which:
	 * Initializes members to default values.
	 * Sets the interface pointers to other classes
	 *
	 * \param inputint - A glass3::util::iinput pointer to the input class.
	 * \param outputint - A glass3::util::ioutput pointer to the output class.
	 */
	Associator(glass3::util::iInput* inputint,
				glass3::util::iOutput* outputint);

	/**
	 * \brief associator destructor
	 *
	 * The destructor for the associator class.
	 */
	~Associator();

	/**
	 * \brief Associator configuration function
	 *
	 * The this function configures the Associator class i.e. configures
	 * glasscore. The setup() function can be called multiple times, in order to
	 * reload or update configuration information.
	 *
	 * \param config - A shared_ptr to a json::Object containing to the
	 * configuration to pass to glasscore.
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief associator clear function
	 *
	 * The clear function for the associator class.
	 * Clears all configuration, clears and reallocates the message queue and
	 * glasscore instance
	 */
	void clear() override;

	/**
	 * \brief glasscore message receiver function
	 *
	 * The function (from IGlassSend) used to receive communication from
	 * the glasscore library.
	 *
	 * \param communication - A shared_ptr a to json::Object containing the
	 * message from glasscore.
	 */
	void recieveGlassMessage(std::shared_ptr<json::Object> communication)
			override;

	/**
	 * \brief glasscore message sending function
	 *
	 * The function (from iassociator) used to send communication to glasscore.
	 *
	 * \param message - A shared_ptr to a json::Object containing the message
	 * to send to the glasscore library.
	 */
	void sendToAssociator(std::shared_ptr<json::Object> &message) override;

	/**
	 * \brief associator heath check function
	 *
	 * Overrides ThreadBaseClass::healthCheck to add monitoring glasscore.
	 * Uses ThreadBaseClass::healthCheck to monitor worker thread
	 * \return returns true if glasscore and worker thread are still running.
	 */
	bool healthCheck() override;

 protected:
	/**
	 * \brief associator work function
	 *
	 * The function (from threadclassbase) used to do work. For Associator,
	 * this includes sending configuration, messages, and input data to the
	 * glasscore library
	 *
	 * \return returns true if work was successful, false otherwise.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief Integer holding the count of input data sent to glasscore since
	 * the last informational report.
	 */
	int m_iInputCounter;

	/**
	 * \brief Integer holding the count of input data sent to glasscore overall
	 */
	int m_iTotalInputCounter;

	/**
	 * \brief Integer holding the count of times the running average has been
	 * computed, used to compute the running  average of data per second
	 */
	int m_iRunningAverageCounter;

	/**
	 * \brief Integer holding the the running average of data per second used
	 * in the information report
	 */
	double m_dRunningDPSAverage;

	/**
	 * \brief The duration of time spent sending data to the glasscore.
	 */
	std::chrono::duration<double> tGlasscoreDuration;

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

	/**
	 * \brief The queue of pending messages to send to glasscore
	 */
	glass3::util::Queue* m_MessageQueue;

	/**
	 * \brief Pointer to Input class
	 *
	 * A glass3::util::iinput pointer to the class handles neic-glass3 input.
	 * Used to pull messages from the input queue
	 */
	glass3::util::iInput* m_Input;

	/**
	 * \brief Pointer to Output class
	 *
	 * A glass3::util::ioutput pointer to the class that handles neic-glass3
	 * output. Used to pass messages to output
	 */
	glass3::util::iOutput* m_Output;
};
}  // namespace process
}  // namespace glass3
#endif  // ASSOCIATOR_H
