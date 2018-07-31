/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADBASECLASS_H
#define THREADBASECLASS_H

#include <baseclass.h>
#include <threadstate.h>
#include <workstate.h>
#include <thread>
#include <mutex>
#include <string>
#include <ctime>

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::ThreadBaseClass class
 *
 * This class supports creating, starting, stopping, and monitoring a single
 * work thread, allowing for specific thread name and sleep between work time.
 *
 * It is intended that the derived class implement the desired thread work by
 * overriding the pure virtual function work()
 *
 * This class inherits from util::BaseClass
 *
 * \note This class has no tie-in or relation with glass3::util::ThreadPool
 */
class ThreadBaseClass : public util::BaseClass {
 public:
	/**
	 * \brief ThreadBaseClass constructor
	 *
	 * The constructor for the ThreadBaseClass class.
	 * Initializes members to default values.
	 */
	ThreadBaseClass();

	/**
	 * \brief  An advanced constructor that sets up the ThreadBaseClass with
	 * a provided thread name and sleep between work time
	 *
	 * The advanced constructor for the ThreadBaseClass class.
	 * Initializes members to provided values.
	 *
	 * \param threadName - A std::string containing the desired
	 * name of the work thread.
	 * \param sleepTimeMS - An integer value containing the amount of
	 * time to sleep between work() calls in the work thread
	 */
	ThreadBaseClass(std::string threadName, int sleepTimeMS);

	/**
	 * \brief ThreadBaseClass destructor
	 *
	 * The destructor for the ThreadBaseClass class.
	 */
	~ThreadBaseClass();

	/**
	 * \brief work thread start function
	 *
	 * Creates a thread object to run the workLoop() function, and starts it,
	 * setting m_bStarted to be true.
	 *
	 * \return returns true if successful, false if the thread creation failed
	 * or if a thread had already been started
	 */
	virtual bool start();

	/**
	 * \brief work thread stop function
	 *
	 * Stops, waits for, and deletes the thread that runs the workLoop() function,
	 * setting m_bStarted, m_bRunWorkThread, and m_bThreadHealth to false
	 *
	 * \return returns true if successful, false if the thread is not created and
	 * running
	 */
	virtual bool stop();

	/**
	 * \brief Function to set thread health
	 *
	 * This function signifies the thread health by using setLastHealthy
	 * to set m_tLastHealthy to now if health is true
	 *
	 * \param health = A boolean value indicating thread health, true indicates
	 * that setLastHealthy to set m_tLastHealthy to now, false indicates
	 * it should not
	 */
	void setThreadHealth(bool health = true);

	/**
	 * \brief work thread check function
	 *
	 * Checks to see if the thread that runs the workLoop() function is still
	 * operational, by checking the value of m_bCheckWorkThread (==true) every
	 * m_iCheckInterval seconds, setting m_bCheckWorkThread to false after the
	 * check. It is expected that the derived class's overridden work() function
	 * periodically updates m_bRunWorkThread by calling setThreadHealth()
	 *
	 * \return returns true if thread is still running after m_iCheckInterval
	 * seconds, false otherwise
	 */
	virtual bool healthCheck();

	/**
	 * \brief Sets the time to sleep between work() calls
	 *
	 * Sets the amount of time to sleep between work() function calls in the
	 * workLoop() function, which is run by the thread
	 *
	 * \param sleepTimeMS - An integer value containing the sleep between work()
	 * calls in integer milliseconds.
	 */
	void setSleepTime(int sleepTimeMS);

	/**
	 * \brief Retrieves the time to sleep between work() calls
	 *
	 * Retrieves the amount of time to sleep between work() function calls in
	 * the workLoop() function, which is run by the thread
	 *
	 * \return Returns the amount of time to sleep between work() calls in
	 * integer milliseconds.
	 */
	int getSleepTime();

	/**
	 * \brief Function to get thread state
	 *
	 * This function gets the thread state by getting the value of
	 * m_ThreadState.
	 *
	 * \return Returns a glass3::util::ThreadState enumeration value representing
	 * the thread state
	 */
	virtual glass3::util::ThreadState getThreadState();

	/**
	 * \brief Function to set thread health check interval
	 *
	 * This function sets the time interval after which m_bThreadHealth being
	 * false (not responded) indicates that the thread has died in check()
	 *
	 * \param interval = An integer value indicating the thread health check
	 * interval in seconds
	 */
	void setHealthCheckInterval(int interval);

	/**
	 * \brief Function to retrieve the thread health check interval
	 *
	 * This function retreives the time interval after which m_bThreadHealth
	 * being false (not responded) indicates that the thread has died in check()
	 *
	 * \return An integer value containing the thread health check interval in
	 * seconds
	 */
	int getHealthCheckInterval();

	/**
	 * \brief Function to set the name of the work thread
	 *
	 * This function sets the name of the work thread, this name is used to
	 * identify the work thread in logging
	 *
	 * \param threadName = A std::string containing the thread name to set
	 */
	void setThreadName(std::string threadName);

	/**
	 * \brief Function to retrieve the name of the work thread
	 *
	 * This function retrieves the name of the work thread, this name is used to
	 * identify the work thread
	 *
	 * \return A std::string containing the thread name
	 */
	const std::string& getThreadName();

	/**
	 * \brief Function to retrieve the last time the work thread health status
	 * was checked
	 *
	 * This function retrieves the last time the health status of this thread
	 * was set by the setLastHealthy() function
	 *
	 * \return A std::time_t containing the last check time
	 */
	std::time_t getLastHealthy();

	/**
	 * \brief ThreadBaseClass work function
	 *
	 * This function is a pure virtual work function. It is intended that the
	 * derived class implement the desired thread work by overriding this
	 * function
	 * \return This function returns glass3::util::WorkState, indicating whether
	 * the work was successful, encountered an error, or was idle (no work to
	 * perform
	 */
	virtual glass3::util::WorkState work() = 0;

 protected:
	/**
	 * \brief Function to set thread state
	 *
	 * This function signifies the thread state by setting m_ThreadState
	 * to the provided value.
	 *
	 * \param state = A glass3::util::ThreadState enumeration value indicating
	 * the new thread state
	 */
	void setThreadState(glass3::util::ThreadState state);

	/**
	 * \brief ThreadBaseClass work loop function
	 *
	 * This function is the thread work loop function. It runs in a loop while
	 * m_bRunWorkThread is true, calls work() every m_iSleepTimeMS milliseconds,
	 * to do a unit of work, sets thread health via setThreadHealth(). Setting
	 * m_bRunWorkThread to false via setRunning, or a false return from work()
	 * will cause the loop to exit, and the function to return (ending the thread)
	 */
	void workLoop();

	/**
	 * \brief Function to set the last time the work thread was healthy
	 *
	 * This function sets the last time the work thread was healthy
	 *
	 * \param now - A std::time_t containing the last time the thread was
	 * healthy
	 */
	void setLastHealthy(std::time_t now);

 private:
	/**
	 * \brief the std::string containing the name of the work thread,
	 * used for logging
	 */
	std::string m_sThreadName;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	std::atomic<int> m_iHealthCheckInterval;

	/**
	 * \brief the std::thread pointer to the work thread, allocated by start()
	 * and destroyed by stop()
	 */
	std::thread *m_WorkThread;

	/**
	 * \brief glass3::util::ThreadState enumeration used to track thread status,
	 * set by setThreadState()
	 */
	std::atomic<glass3::util::ThreadState> m_ThreadState;

	/**
	 * \brief the time_t holding the last time the thread status was checked,
	 * set by setLastHealthy() in check
	 */
	std::atomic<double> m_tLastHealthy;

	/**
	 * \brief integer variable indicating how long to sleep in milliseconds
	 * between work() calls in workloop()
	 */
	std::atomic<int> m_iSleepTimeMS;
};
}  // namespace util
}  // namespace glass3
#endif  // THREADBASECLASS_H
