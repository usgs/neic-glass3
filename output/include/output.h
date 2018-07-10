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
namespace output {
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
	 * The this function configures the output class, and the tracking cache it
	 * contains.
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
	 * Clears all configuration, clears and reallocates the message queue and
	 * cache
	 */
	void clear() override;

	/**
	 * \brief output message sending function
	 *
	 * The function (from ioutput) used to send communication to output.
	 *
	 * \param message - A json::Object containing the message to send to output.
	 */
	void sendToOutput(std::shared_ptr<json::Object> message) override;

	bool start() override;
	bool stop() override;
	glass3::util::ThreadState getThreadState() override;

	/**
	 * \brief output heath check function
	 *
	 * Checks to see if the thread pool is still running, calls
	 * threadbaseclass::healthCheck for worker thread monitoring.
	 * \return returns true if thread pool is still running.
	 */
	bool healthCheck() override;

	void setOutputAgency(std::string agency);

	const std::string getOutputAgencyId();

	void setOutputAuthor(std::string author);

	const std::string getOutputAuthor();

	void setSiteListDelay(int delay);

	int getSiteListDelay();

	void setStationFile(std::string filename);

	const std::string getStationFile();

	void setReportInterval(int interval);

	int getReportInterval();

	void setAssociator(glass3::util::iAssociator* associator);

	glass3::util::iAssociator* getAssociator();

	void setPubOnExpiration(bool pub);

	int getPubOnExpiration();

	std::vector<int> getPubTimes();

	void setPubTimes(std::vector<int> pubTimes);

	void addPubTime(int pubTime);

	void clearPubTimes();

	/**
	 * \brief Checks to see if the event thread should still be running
	 *
	 * This function checks to see if the event thread should still running by
	 * returning the value of m_bRunWorkThread.
	 * \return Returns true if the thread should still running, false if it
	 * has been stopped
	 */
	bool isEventRunning();

	/**
	 * \brief add data to the output tracking cache
	 *
	 * Add the detection data to the cache of data pending for output
	 *
	 * \param data - A pointer to a json::Object containing the detection data.
	 * \return Returns true if successful, false otherwise
	 */
	bool addTrackingData(std::shared_ptr<json::Object> data);

	std::shared_ptr<const json::Object> getTrackingData(std::string id);

	/**
	 * \brief get data from the output tracking cache
	 *
	 * Get the first available detection data from the cache of data pending for
	 * output that is ready
	 *
	 * \return Returns a pointer to the json::Object containing the detection
	 * data ready for output, NULL if no data found that is ready.
	 */
	std::shared_ptr<const json::Object> getNextTrackingData();

	/**
	 * \brief check if data is in output tracking cache
	 *
	 * Check to see if given detection data is already in the output tracking
	 * cache.
	 *
	 * \param data - A pointer to a json::Object containing the detection data.
	 * \return Returns true if the data is in the cache, false otherwise
	 */
	bool haveTrackingData(std::shared_ptr<json::Object> data);
	bool haveTrackingData(std::string ID);

	/**
	 * \brief remove data from the output tracking cache
	 *
	 * Remove the provided detection data from the output tracking cache.
	 *
	 * \param data - A pointer to a json::Object containing the detection data.
	 * \return Returns true if successful, false otherwise
	 */
	bool removeTrackingData(std::shared_ptr<const json::Object> data);
	bool removeTrackingData(std::string ID);

	/**
	 * \brief clear output tracking cache
	 *
	 * Clear all detection data from the output tracking cache.
	 */
	void clearTrackingData();

	/**
	 * \brief check to see if detection data is ready for output
	 *
	 * Check the given detection data to see if it is ready for output
	 *
	 * \param data - A pointer to the json::Object containing the detection
	 * data to check
	 * \return Returns true if the data is ready, false if not.
	 */
	bool isDataReady(std::shared_ptr<const json::Object> data);
	bool isDataChanged(std::shared_ptr<const json::Object> data);
	bool isDataPublished(std::shared_ptr<const json::Object> data,
							bool ignoreVersion = true);
	bool isDataFinished(std::shared_ptr<const json::Object> data);

	/**
	 * \brief Function to retrieve the last time the event thread health status
	 * was checked
	 *
	 * This function retrieves the last time the health status of the event
	 * thread was checked by the check() function
	 *
	 * \return A std::time_t containing the last check time
	 */
	std::time_t getLastEventCheck();

	/**
	 * \brief Function to set thread health
	 *
	 * This function signifies the thread health by setting m_bCheckWorkThread
	 * to the provided value.
	 *
	 * \param check = A boolean value indicating thread health, true indicates
	 * that the the thread is alive, false indicates that the thread is not
	 */
	void setCheckEventThread(bool check = true);

	/**
	 * \brief Function to check thread health
	 *
	 * This function checks the thread health by getting the value of
	 * m_bCheckWorkThread.
	 *
	 * \return Returns true if the thread is alive, false if the thread has
	 * not responded yet
	 */
	bool getCheckEventThread();

	/**
	 *\brief Retrieves whether the event thread has been started
	 *
	 * This function retrieves the value of m_bEventStarted, which indicates
	 * whether the event thread has been created and started
	 * \returns true if the event thread has been started, false otherwise
	 */
	bool isEventStarted();

 protected:
	/**
	 * \brief output work function
	 *
	 * The function (from threadclassbase) used to do work.
	 *
	 * \return returns true if work was successful, false otherwise.
	 */
	bool work() override;

	void checkEventsLoop();

	/**
	 * \brief output file writing function
	 *
	 * The function used output detection data
	 *
	 * \param data - A pointer to a json::Object containing the data to be
	 * output.
	 */
	void writeOutput(std::shared_ptr<json::Object> data);

	virtual void sendOutput(const std::string &type, const std::string &id,
							const std::string &message) = 0;

	/**
	 * \brief Sets whether the event thread is running
	 *
	 * This function sets m_bRunEventThread. Setting m_bRunEventThread to true
	 * indicates that the thread should still run (via checkEventsLoop()). Setting
	 * m_bRunEventThread to false indicates that the thread should stop (and
	 * checkEventsLoop() should return)
	 */
	void setEventRunning(bool running);

	/**
	 * \brief Function to sset the last time the event thread health status
	 * was checked
	 *
	 * This function sets the last time the health status of the event thread
	 * was checked.
	 *
	 * \param now - A std::time_t containing the last time the health status
	 * was checked
	 */
	void setLastEventCheck(std::time_t now);

	/**
	 *\brief Sets whether the event thread has been started
	 *
	 * This function sets the value of m_bEventStarted, which indicates whether
	 * the event thread has been created and started
	 * \param started - a boolean flag indicating whether the event thread has
	 * been started
	 */
	void setEventStarted(bool started);

 private:
	std::vector<int> m_PublicationTimes;

	bool m_bPubOnExpiration;

	/**
	 * \brief Information Report interval
	 *
	 * An integer containing the interval (in seconds) between
	 * logging informational reports.
	 */
	int m_iReportInterval;

	/**
	 * \brief Pointer to Association class
	 *
	 * A glass3::util::iassociator pointer to the class that handles association for
	 * glass
	 */
	glass3::util::iAssociator* m_Associator;

	/**
	 * \brief the std::string configuration value defining the
	 * agency identifier used when generating output files
	 */
	std::string m_sOutputAgencyID;

	/**
	 * \brief the std::string configuration value defining the
	 * author used when generating output files
	 */
	std::string m_sOutputAuthor;

	/**
	 * \brief the integer configuration value indicating the delay in seconds
	 * before requesting glass core's current sitelist
	 */
	int m_iSiteListDelay;

	/**
	 * \brief the std::string containing the station file name.
	 */
	std::string m_sStationFile;

	/**
	 * \brief pointer to the glass3::util::cache class used to
	 * store output tracking information
	 */
	glass3::util::Cache * m_TrackingCache;
	std::mutex m_TrackingCacheMutex;

	/**
	 * \brief pointer to the glass3::util::queue class used to manage
	 * incoming output messages
	 */
	glass3::util::Queue* m_OutputQueue;

	/**
	 * \brief pointer to the glass3::util::queue class used to manage
	 * incoming lookup messages
	 */
	glass3::util::Queue* m_LookupQueue;

	/**
	 * \brief the total messages performance counter
	 */
	int m_iMessageCounter;

	/**
	 * \brief the new event messages performance counter
	 */
	int m_iEventCounter;

	/**
	 * \brief the event cancel messages performance counter
	 */
	int m_iCancelCounter;

	/**
	 * \brief the event expire messages performance counter
	 */
	int m_iExpireCounter;

	/**
	 * \brief the hypo messages performance counter
	 */
	int m_iHypoCounter;

	/**
	 * \brief the lookup messages performance counter
	 */
	int m_iLookupCounter;

	/**
	 * \brief the sitelist messages performance counter
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

	/**
	 * \brief the std::thread pointer to the event thread
	 */
	std::thread *m_EventThread;

	/**
	 * \brief boolean flag indicating whether the event thread should run
	 */
	bool m_bRunEventThread;

	/**
	 * \brief boolean flag indicating whether the event thread has been started
	 */
	bool m_bEventStarted;

	/**
	 * \brief boolean flag used to check thread status
	 */
	bool m_bCheckEventThread;

	/**
	 * \brief the std::time_t holding the last time the thread status was checked
	 */
	std::time_t tLastEventCheck;
};
}  // namespace output
}  // namespace glass3
#endif  // OUTPUT_H
