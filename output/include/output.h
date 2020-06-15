/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef OUTPUT_H
#define OUTPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <outputinterface.h>
#include <associatorinterface.h>
#include <cache.h>
#include <queue.h>
#include <threadpool.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

namespace glass3 {

/**
 * \namespace glass3::output
 * \brief The neic-glass3 project namespace containing output class and functions
 *
 * The neic-glass3 output namespace contains the class, and functions used
 * by other components of neic-glass3 to output data.
 */
namespace output {
/**
 * \brief glass output class
 *
 * The glass3 output class is a class that performs the publication tracking and
 * translation tasks for neic-glass3.
 *
 * The Output class utilizes the internal glasscore Event, Cancel, and Expire
 * messages as defined at
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Event.md  // NOLINT
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Cancel.md  // NOLINT
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Expire.md  // NOLINT
 * for publication tracking. The class uses a configurable set of fixed
 * publication times to determine when to publish. These messages are passed to
 * Output via the sendToOutput function from the iOutput interface
 *
 * The output class generates the internal formats ReqHypo and ReqSiteList messages
 * defined at
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/ReqHypo.md  // NOLINT
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/ReqSiteList.md  // NOLINT
 * to request detailed event detection and site list information from glasscore
 * via the m_Associator pointer.
 *
 * The output class translates the internal glasscore Hypo, SiteList and SiteLookup
 * messages as  messages as defined at
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Hypo.md  // NOLINT
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteList.md  // NOLINT
 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteLookup.md  // NOLINT
 * detection formats messages, as defined at
 * https://github.com/usgs/earthquake-detection-formats/tree/master/format-docs
 *
 * The Output class is designed to be extended in order to define application
 * specific output mechanisms (i.e. file output).
 *
 * output inherits from the glass3::util::ThreadBaseClass class.
 * output implements the glass3::util::iOutput interface.
 */
class output : public glass3::util::iOutput,
		public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief output constructor
	 *
	 * The constructor for the output class.
	 * Initializes members to default values.
	 */
	output();

	/**
	 * \brief output destructor
	 *
	 * The destructor for the output class.
	 * Stops the work thread
	 */
	~output();

	/**
	 * \brief output configuration function
	 *
	 * The function configures the output class.
	 *
	 * The setup() function can be called multiple times, in order to reload or
	 * update configuration information.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief output clear function
	 *
	 * The clear function for the output class.
	 * Resets members to default values.
	 */
	void clear() override;

	/**
	 * \brief output message sending function
	 *
	 * The function (from glass3::util::iOutput) used by other libraries to send
	 * messages to output.
	 *
	 * \param message - A shared_ptr to a json::Object containing the message to
	 * send to output.
	 */
	void sendToOutput(std::shared_ptr<json::Object> message) override;

	/**
	 * \brief work thread start function
	 *
	 * Overrides ThreadBaseClass::start() to create a thread to run the
	 * checkEventsLoop() function and add the thread to ThreadBaseClass list
	 * of threads, so it can be managed
	 *
	 * \return returns true if successful, false if ThreadBaseClass::start()
	 * failed.
	 */
	bool start() override;

	/**
	 * \brief output heath check function
	 *
	 * Overrides ThreadBaseClass::healthCheck to add monitoring the thread pool.
	 * Uses ThreadBaseClass::healthCheck to monitor worker threads
	 * \return returns true if thread pool and worker threads are still running.
	 */
	bool healthCheck() override;

	/**
	 * \brief Function to set the interval for requesting the site list
	 *
	 * This function sets the interval in seconds between requesting glass
	 * core's current site list. A negative delay indicates that the site list
	 * should not be requested
	 *
	 * \param delay = An integer value containing the delay in seconds
	 */
	void setSiteListRequestInterval(int delay);

	/**
	 * \brief Function to retrieve the interval for requesting the site list
	 *
	 * This function retrieves the interval in seconds between requesting glass
	 * core's current sitelist
	 *
	 * \return Returns an integer value containing the delay in seconds
	 */
	int getSiteListRequestInterval();

	/**
	 * \brief Function to set the interval to generate informational reports
	 *
	 * This function sets the interval in seconds between logging informational
	 * reports on output throughput and performance
	 *
	 * \param interval = An integer value containing the interval in seconds
	 */
	void setReportInterval(int interval);

	/**
	 * \brief Function to retrieve the interval to generate informational reports
	 *
	 * This function retrieves the interval in seconds between logging
	 * informationalreports on output throughput and performance
	 *
	 * \return Returns an integer value containing the interval in seconds
	 */
	int getReportInterval();

	/**
	 * \brief Function to set the associator interface pointer
	 *
	 * This function sets the associator interface pointer used by output to
	 * communicate with the associator via the Associator::sendToAssociator()
	 * function
	 *
	 * \param associator = A pointer to an object that implements the
	 * glass3::util::iAssociator interface.
	 */
	void setAssociator(glass3::util::iAssociator* associator);

	/**
	 * \brief Function to get the associator interface pointer
	 *
	 * This function gets the associator interface pointer used by output to
	 * communicate with the associator via the Associator::sendToAssociator()
	 * function
	 *
	 * \return Returns a pointer to an object that implements the
	 * glass3::util::iAssociator interface.
	 */
	glass3::util::iAssociator* getAssociator();

	/**
	 * \brief Function to set the publish on expiration flag
	 *
	 * This function sets the boolean flag that indicates whether output should
	 * generate a detection message when it receives an expiration notification
	 * from the associator.
	 *
	 * \param pub = A boolean flag indicating whether to generate a detection
	 * message on expiration
	 */
	void setPubOnExpiration(bool pub);

	/**
	 * \brief Function to retrieve the publish on expiration flag
	 *
	 * This function retrieves the boolean flag that indicates whether output
	 * should generate a detection message when it receives an expiration
	 * notification from the associator.
	 *
	 * \return Returns a boolean flag indicating whether to generate a detection
	 * message on expiration
	 */
	bool getPubOnExpiration();

	/**
	 * \brief Function to set the immediate publication threshold
	 *
	 * This function sets the threshold used to determine whether output should
	 * generate a detection message when it receives an event message with a 
	 * sufficent bayes value from the associator.
	 *
	 * \param threshold - A double value containing the immediate publication threshold
	 */
	void setImmediatePubThreshold(double threshold);

	/**
	 * \brief Function to retrieve the immediate publication threshold
	 *
	 * This function retrieves the threshold used to determine whether output should
	 * generate a detection message when it receives an event message with a 
	 * sufficent bayes value from the associator.
	 *
	 * \return Returns a double value containing the immediate publication threshold
	 */
	double getImmediatePubThreshold();

	/**
	 * \brief Function to retrieve the publication times
	 *
	 * This function retrieves the publication times in seconds used to determine
	 * when to generate detection messages for events. An event will not generate
	 * a message if it has not changed, and will not generate a message if all
	 * publication times have passed (unless m_bPubOnExpiration is set to true)
	 *
	 * \return Returns a std::vector of integers containing the times in seconds
	 * since initial report that events should generate detection messages
	 */
	std::vector<int> getPubTimes();

	/**
	 * \brief Function to add a single publication time to the list
	 *
	 * This function adds a single publication time in seconds to the list used
	 * to determine when to generate detection messages for events. An event
	 * will not generate a message if it has not changed, and will not generate
	 * a message if all publication times have passed (unless m_bPubOnExpiration
	 * is set to true)
	 *
	 * \param pubTime = An integer containing a times in seconds since initial
	 * report that events should generate detection messages
	 *
	 */
	void addPubTime(int pubTime);

	/**
	 * \brief Function to clear the publication times
	 *
	 * This function clears the publication times in seconds used to determine
	 * when to generate detection messages for events.
	 */
	void clearPubTimes();

	/**
	 * \brief add tracking information to the output tracking cache
	 *
	 * Add the glasscore event message (used for tracking) as defined at
	 * https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Event.md  // NOLINT
	 * to the tracking cache as the tracking information
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information
	 * \return Returns true if successful, false otherwise
	 */
	bool addTrackingData(std::shared_ptr<json::Object> data);

	/**
	 * \brief get tracking information from the output tracking cache by id
	 *
	 * Get the tracking information from the cache of tracking information
	 * pending for output  based on a provided id
	 *
	 * \param id - A std::string containing the id of the tracking information to
	 * retrieve from the cache
	 * \return Returns a shared_ptr to the json object containing the glasscore
	 * event message used for storing tracking information if found, null
	 * otherwise
	 */
	std::shared_ptr<const json::Object> getTrackingData(std::string id);

	/**
	 * \brief get tracking information from the output tracking cache
	 *
	 * Get the first ready tracking data from the cache by starting at the
	 * beginning of the cache and evaluating each tracking data with
	 * isDataReady()
	 *
	 *\return Returns a shared_ptr to the json object containing the glasscore
	 * event message used for storing tracking information if found, NULL if no
	 * information found that is ready to publish.
	 */
	std::shared_ptr<const json::Object> getNextTrackingData();

	/**
	 * \brief check if tracking information is in output tracking cache
	 *
	 * Check to see if given tracking information is already in the output
	 * tracking cache.
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information
	 * \return Returns true if the data is in the cache, false otherwise
	 */
	bool haveTrackingData(std::shared_ptr<json::Object> data);

	/**
	 * \brief check if information is in output tracking cache by id
	 *
	 * Check to see if tracking information is already in the output
	 * tracking cache by the given id.
	 *
	 * \param ID - A std::string containing the id of the detection data to check.
	 * \return Returns true if the data is in the cache, false otherwise
	 */
	bool haveTrackingData(std::string ID);

	/**
	 * \brief remove tracking information from the output tracking cache
	 *
	 * Remove the provided tracking information from the output tracking cache.
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information to remove
	 * \return Returns true if successful, false otherwise
	 */
	bool removeTrackingData(std::shared_ptr<const json::Object> data);

	/**
	 * \brief remove tracking information from the output tracking cache by id
	 *
	 * Remove the tracking information from the output tracking cache using the
	 * given id
	 *
	 * \param ID - A std::string containing the id of the detection data to remove.
	 * \return Returns true if successful, false otherwise
	 */
	bool removeTrackingData(std::string ID);

	/**
	 * \brief clear output tracking cache
	 *
	 * Clear all tracking information from the output tracking cache.
	 */
	void clearTrackingData();

	/**
	 * \brief check to see if tracking information is ready for output
	 *
	 * Check the given tracking information to see if it is ready for output
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information to check
	 * \return Returns true if the tracking information is ready, false if not.
	 */
	bool isDataReady(std::shared_ptr<const json::Object> data);

	/**
	 * \brief check to see if detection data has changed
	 *
	 * Check the given dtracking information to see if it has been changed
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information to check
	 * \return Returns true if the tracking information is has been changed,
	 * false if not.
	 */
	bool isDataChanged(std::shared_ptr<const json::Object> data);

	/**
	 * \brief check to see if tracking information has been published
	 *
	 * Check the given tracking information to see if it has been previously
	 * published. Optionally ignore the current version when doing the check.
	 * This is used when determining whether tracking information has ever been
	 * published (i.e. when generating a retraction message)
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information to check
	 * \param ignoreVersion - A boolen flag indicating whether to ignore
	 * the tracking current information version
	 * \return Returns true if the tracking information is has been published,
	 * false if not.
	 */
	bool isDataPublished(std::shared_ptr<const json::Object> data,
							bool ignoreVersion = true);
	/**
	 * \brief check to see if tracking information is finished
	 *
	 * Check the given tracking information to see if is finished (no more
	 * publications)
	 *
	 * \param data - A shared_ptr to a json::Object containing the glasscore
	 * event message used as the tracking information to check
	 * \return Returns true if the tracking information is finished, false if
	 * not.
	 */
	bool isDataFinished(std::shared_ptr<const json::Object> data);

	/**
	 * \brief output writing function
	 *
	 * The function used to translate and generate output data
	 *
	 * \param data - A shared_ptr to a json::Object containing the data to be
	 * output.
	 */
	void writeOutput(std::shared_ptr<json::Object> data);

	/**
	 * \brief output background work function
	 *
	 * The function (from ThreadBaseClass) used to do background work. It is
	 * used to process messages from the associator, and to queue messages to
	 * be written out
	 *
	 * \return This function returns glass3::util::WorkState, indicating whether
	 * the work was successful, encountered an error, or was idle (no work to
	 * perform
	 */
	glass3::util::WorkState work() override;

 protected:
	/**
	 * \brief output tracking data background work function
	 *
	 * This function is used to manage the tracking cache, and to send request
	 * data (such as hypocenters) from the associator
	 */
	void checkEventsLoop();

	/**
	 * \brief Send output data
	 *
	 * This pure virtual function is used to send output data. It is expected
	 * that the implementing class will override this function to implement a
	 * specific output method, i.e. to disk, memory, socket, kafka, etc.
	 *
	 * \param type - A std::string containing the type of the message
	 * \param id - A std::string containing the id of the message
	 * \param message - A std::string containing the message
	 */
	virtual void sendOutput(const std::string &type, const std::string &id,
							const std::string &message) = 0;

	/**
	 * \brief Send heartbeats
	 *
	 * This function is optionally used by an overriding class to implement a
	 * specific heartbeat method, i.e. to disk, memory, socket, kafka, etc.
	 */
	virtual void sendHeartbeat();

 private:
	/**
	 * \brief A std::vector of integers containing the times in seconds
	 * since initial report that events should generate detection messages. An
	 * event will not generate a message if it has not changed, and will not
	 * generate a message if all publication times have passed (unless
	 * m_bPubOnExpiration is set to true)
	 */
	std::vector<int> m_PublicationTimes;

	/**
	 * \brief The boolean flag controlling whether to generate a detection
	 * message on expiration notification from the associator
	 */
	std::atomic<bool> m_bPubOnExpiration;

	/**
	 * \brief Information Report interval
	 *
	 * An integer containing the interval (in seconds) between logging
	 * informational reports.
	 */
	std::atomic<int> m_iReportInterval;

	/**
	 * \brief Immediate Publication Threshold
	 *
	 * An optional double containing the bayes threshold for immediately 
	 * publishing an event, -1.0 to disable
	 */
	std::atomic<double> m_dImmediatePubThreshold;

	/**
	 * \brief Pointer to Association class
	 *
	 * A glass3::util::iassociator pointer to the class that handles association
	 * for glass. It is used to request detailed event detection information and
	 * site lists from glasscore.
	 */
	glass3::util::iAssociator* m_Associator;

	/**
	 * \brief the integer configuration value indicating the delay in seconds
	 * before requesting a site list from glasscore
	 */
	std::atomic<int> m_iSiteListRequestInterval;

	/**
	 * \brief pointer to the glass3::util::cache class used to
	 * store output tracking information
	 */
	glass3::util::Cache * m_TrackingCache;

	/**
	 * \brief mutex used to control access to the tracking cache
	 */
	std::mutex m_TrackingCacheMutex;

	/**
	 * \brief pointer to the glass3::util::queue class used to manage
	 * incoming Event, Cancel, Expire, and Hypo messages from glasscore
	 */
	glass3::util::Queue* m_OutputQueue;

	/**
	 * \brief pointer to the glass3::util::queue class used to manage
	 * incoming SiteList and SiteLookup messages from glasscore
	 */
	glass3::util::Queue* m_LookupQueue;

	/**
	 * \brief the performance counter tracking the total count of messages
	 * received from glasscore since the last informational reports
	 */
	int m_iMessageCounter;

	/**
	 * \brief the performance counter tracking the number of Event messages
	 * received from glasscore since the last informational reports
	 */
	int m_iEventCounter;

	/**
	 * \brief the performance counter tracking the number of Cancel messages
	 * received from glasscore since the last informational reports
	 */
	int m_iCancelCounter;

	/**
	 * \brief the performance counter tracking the number of Expire messages
	 * received from glasscore since the last informational reports
	 */
	int m_iExpireCounter;

	/**
	 * \brief the performance counter tracking the number of Hypo messages
	 * received from glasscore since the last informational reports
	 */
	int m_iHypoCounter;

	/**
	 * \brief the performance counter tracking the number of SiteLookup messages
	 * received from glasscore since the last informational reports
	 */
	int m_iLookupCounter;

	/**
	 * \brief the performance counter tracking the number of SiteList messages
	 * received from glasscore since the last informational reports
	 */
	int m_iSiteListCounter;

	/**
	 * \brief the last time a performance report was generated
	 */
	std::time_t tLastWorkReport;

	/**
	 * \brief the last time a site list was requested
	 */
	std::time_t m_tLastSiteRequest;

	/**
	 * \brief pointer to the glass3::util::threadpool used to queue and
	 * perform output.
	 */
	glass3::util::ThreadPool *m_ThreadPool;

 private:
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
}  // namespace output
}  // namespace glass3
#endif  // OUTPUT_H
