/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef ASSOCIATOR_H
#define ASSOCIATOR_H

#include <json.h>
#include <Glass.h>
#include <IGlassSend.h>
#include <Logit.h>
#include <inputinterface.h>
#include <outputinterface.h>
#include <associatorinterface.h>
#include <threadbaseclass.h>
#include <queue.h>
#include <ctime>
#include <memory>

/**
 * \namespace glass
 * \brief namespace containing the primary glass classes
 *
 * The glass namespace contains the primary classes and components
 * of glass, including the input, output, associator, lookup, parsing, and
 * station information classes.
 */
namespace glass {
/**
 * \brief glass association class
 *
 * The glass association class is a thread class encapsulating the glass core
 * association and nucleation engine.  The associator class pulls input data
 * from the input class, and sends it into glasscore.  It also routes any
 * results to the output class, and sends station information requests to the
 * stationlist class.  The class also sends any configuration into glasscore.
 *
 * associator inherits from the threadbaseclass class.
 *
 * associator implements the IGlassSend and iassociator interfaces.
 */
class Associator : public glasscore::IGlassSend,
		public glass3::util::iAssociator, public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief associator constructor
	 *
	 * The constructor for the associator class.
	 * Initializes members to default values.
	 */
	Associator();

	/**
	 * \brief associator advanced constructor
	 *
	 * The advanced constructor for the associator class.
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
	 * \brief basic associator configuration
	 *
	 * The this function is inherited from baseclass, but SHOULD NOT BE CALLED
	 * instead use the advanced setup function.  Function is overridden to
	 * always return false to avoid the appearance that the associator class is
	 * configurable via this method.
	 *
	 * \param config - A json::Object containing configuration, ignored
	 * \return Always returns false.
	 */
	bool setup(json::Object *config) override;

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
	 * glasscore.
	 *
	 * \param communication - A json::Object containing the message from
	 * glasscore.
	 */
	void Send(std::shared_ptr<json::Object> communication) override;

	/**
	 * \brief glasscore message sending function
	 *
	 * The function (from iassociator) used to send communication to glasscore.
	 *
	 * \param message - A json::Object containing the message to send to
	 * glasscore.
	 */
	void sendToAssociator(std::shared_ptr<json::Object> &message) override;

	/**
	 * \brief thread pool check function
	 *
	 * Checks to see if glass is running, calls
	 * threadbaseclass::check for worker thread monitoring.
	 * \return returns true if glass is still running.
	 */
	bool check() override;

	/**
	 * \brief Pointer to Input class
	 *
	 * A glass3::util::iinput pointer to the class handles glass input
	 */
	glass3::util::iInput* Input;

	/**
	 * \brief Pointer to Output class
	 *
	 * A glass3::util::ioutput pointer to the class that handles output input for glass
	 */
	glass3::util::iOutput* Output;

	/**
	 * \brief Information Report interval
	 *
	 * An integer containing the interval (in seconds) between
	 * logging informational reports.
	 */
	int ReportInterval;

 protected:
	/**
	 * \brief associator work function
	 *
	 * The function (from threadclassbase) used to do work.
	 *
	 * \return returns true if work was successful, false otherwise.
	 */
	bool work() override;

	/**
	 * \brief glasscore dispatch function
	 *
	 * The function the associator class uses to send communication from
	 * glass to the Output and StationList classes.
	 *
	 * \param communication - A json::Object containing the message from
	 * glasscore.
	 * \return returns true if the dispatch was successful, false otherwise.
	 */
	bool dispatch(std::shared_ptr<json::Object> communication);

	/**
	 * \brief glasscore logging function
	 *
	 * The function the associator class uses to log messages coming out
	 * of glascore.
	 *
	 * \param message - A glasscore::logMessageStruct containing the message to
	 * log from glasscore.
	 */
	void logGlass(glassutil::logMessageStruct message);

 private:
	/**
	 * \brief Pointer to CGlass class instance
	 */
	glasscore::CGlass *m_pGlass;

	/**
	 * \brief Integer holding the count of input data sent to glasscore since
	 * the last informational report.
	 */
	int m_iWorkCounter;

	/**
	 * \brief Integer holding the count of input data sent to glasscore overall
	 */
	int m_iTotalWorkCounter;

	/**
	 * \brief Integer holding the count of used to compute the running average
	 * of data per second
	 */
	int m_iRunningAverageCounter;

	/**
	 * \brief Integer holding the the running average of data per second
	 */
	double m_dRunningAverage;

	/**
	 * \brief The duration of time spent sending data to the glasscore.
	 */
	std::chrono::duration<double> tGlassDuration;

	/**
	 * \brief The time the last informational report was generated.
	 */
	time_t tLastWorkReport;

	/**
	 * \brief The queue of pending messages to send to glasscore
	 */
	glass3::util::Queue* m_MessageQueue;
};
}  // namespace glass
#endif  // ASSOCIATOR_H
