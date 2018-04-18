/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADBASECLASS_H
#define THREADBASECLASS_H

#include <baseclass.h>
#include <thread>
#include <mutex>
#include <string>

namespace util {
/**
 * \brief util threadbaseclass class
 *
 * The util threadbaseclass class is a class encapsulating the threading
 * logic.  The baseclass class supports creating, starting, stopping,
 * and monitoring a work thread.
 *
 * This class inherits from util::baseclass
 * This class is intended to be extended by derived classes.
 */
class ThreadBaseClass : public util::BaseClass {
 public:
	/**
	 * \brief threadbaseclass constructor
	 *
	 * The constructor for the threadbaseclass class.
	 * Initilizes members to default values.
	 */
	ThreadBaseClass();

	/**
	 * \brief threadbaseclass advanced constructor
	 *
	 * The advanced constructor for the threadbaseclass class.
	 * Initializes members to default values.
	 * \param threadname - A std::string containing the desired
	 * name of the work thread.
	 * \param sleeptimems - An integer value containing the amount of
	 * time to sleep between work() calls in the work thread
	 */
	ThreadBaseClass(std::string threadname, int sleeptimems);

	/**
	 * \brief threadbaseclass destructor
	 *
	 * The destructor for the threadbaseclass class.
	 */
	~ThreadBaseClass();

	/**
	 * \brief thread start function
	 *
	 * Allocates and starts the work thread.
	 * \return returns true if successful.
	 */
	virtual bool start();

	/**
	 * \brief thread stop function
	 *
	 * Stops, waits for, and deallocates the work thread.
	 * \return returns true if successful.
	 */
	virtual bool stop();

	/**
	 * \brief thread check function
	 *
	 * Checks to see if the work thread is still running.
	 * \return returns true if thread is still running.
	 */
	virtual bool check();

	/**
	 * \brief set thread sleep time
	 *
	 * Sets the amount of time to sleep between
	 * work() calls in the work thread
	 */
	void setSleepTime(int sleeptimems);

	/**
	 * \brief get thread sleep time
	 *
	 * Gets the amount of time to sleep between
	 * work() calls in the work thread.
	 * \return Returns the amount of time to sleep.
	 */
	int getSleepTime();

	/**
	 * \brief threadbaseclass work function
	 *
	 * Virtual work function to be overridden by deriving classes
	 * \return Returns true if work was successful, false otherwise
	 */
	virtual bool work() = 0;

	/**
	 * \brief threadbaseclass is running
	 *
	 * Checks to see if the work thread is running
	 * \return Returns true if m_bRunWorkThread is true, false otherwise
	 */
	virtual bool isRunning();

	/**
	 * \brief threadbaseclass is set work check
	 *
	 * Signifies that the work thread is still alive by setting
	 * m_bCheckWorkThread to true
	 */
	void setWorkCheck();

	/**
	 *\brief getter for m_iCheckInterval
	 */
	int getCheckInterval() const {
		return m_iCheckInterval;
	}

	/**
	 *\brief getter for m_bCheckWorkThread
	 */
	bool getCheckWorkThread() const {
		return m_bCheckWorkThread;
	}

	/**
	 *\brief getter for m_bStarted
	 */
	bool getStarted() const {
		return m_bStarted;
	}

	/**
	 *\brief getter for m_sThreadName
	 */
	const std::string& getThreadName() const {
		return m_sThreadName;
	}

	/**
	 *\brief getter for tLastCheck
	 */
	time_t getLastCheck() const {
		return tLastCheck;
	}

 protected:
	/**
	 * \brief the std::string containing the name of the work thread
	 */
	std::string m_sThreadName;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	int m_iCheckInterval;

	/**
	 * \brief threadbaseclass work loop
	 *
	 * threadbaseclass work loop, runs while m_bRunWorkThread
	 * is true, calls work(), reports run status with setworkcheck()
	 */
	void workLoop();

 private:
	/**
	 * \brief the std::thread pointer to the work thread
	 */
	std::thread *m_WorkThread;

	/**
	 * \brief boolean flag indicating whether the work thread should run
	 */
	bool m_bRunWorkThread;

	/**
	 * \brief boolean flag indicating whether the work thread has been started
	 */
	bool m_bStarted;

	/**
	 * \brief boolean flag used to check thread status
	 */
	bool m_bCheckWorkThread;

	/**
	 * \brief the std::mutex for m_bCheckWorkThread
	 */
	std::mutex m_CheckMutex;

	/**
	 * \brief the time_t holding the last time the thread status was checked
	 */
	time_t tLastCheck;

	/**
	 * \brief integer variable indicating how long to sleep in milliseconds in
	 * the work thread
	 */
	int m_iSleepTimeMS;
};
}  // namespace util
#endif  // THREADBASECLASS_H
