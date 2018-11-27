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
#include <vector>
#include <map>
#include <ctime>

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::ThreadBaseClass class
 *
 * This class supports creating, starting, stopping, and monitoring one or more
 * work threads, that do the same work allowing for specific thread name and
 * sleep between work time.
 *
 * It is intended that the derived class implement the desired thread work by
 * overriding the pure virtual function work()
 *
 * This class inherits from util::BaseClass
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
	 * a provided thread name, sleep between work time duration, number of
	 * threads, and statsus check interval
	 *
	 * The advanced constructor for the ThreadBaseClass class.
	 * Initializes members to provided values.
	 *
	 * \param threadName - A std::string containing the desired
	 * name of the work thread.
	 * \param sleepTimeMS - An integer value containing the amount of
	 * time to sleep between work() calls in the work thread in milliseconds,
	 * default 100ms
	 * \param numThreads - An integer containing the number of
	 * work threads.  Default 1
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 30.
	 */
	ThreadBaseClass(std::string threadName, int sleepTimeMS = 100,
					int numThreads = 1, int checkInterval = 30);

	/**
	 * \brief ThreadBaseClass destructor
	 *
	 * The destructor for the ThreadBaseClass class.
	 */
	~ThreadBaseClass();

	/**
	 * \brief work thread start function
	 *
	 * Creates one or more thread objects to run the workLoop() function, and
	 * starts them, setting m_WorkThreadsState to be
	 * glass3::util::ThreadState::Starting.
	 *
	 * \return returns true if successful, false if the thread creation failed
	 * or if a thread had already been started
	 */
	virtual bool start();

	/**
	 * \brief work thread stop function
	 *
	 * Stops, waits for, and deletes the threads that runs the workLoop()
	 * function,  setting m_WorkThreadsState to
	 * glass3::util::ThreadState::Stopped
	 *
	 * \return returns true if successful, false if the thread is not created and
	 * running
	 */
	virtual bool stop();

	/**
	 * \brief Retrieves the number of work threads
	 *
	 * Retrieves the number of work threads managed by this class
	 *
	 * \return Returns the number of work threads
	 */
	int getNumThreads();

	/**
	 * \brief Sets the number of work threads
	 *
	 * This function sets m_iNumThreads. This function sets the number of
	 * work threads that should be running, and thus will be started in
	 * start().
	 *
	 * \note This function is only effective when the ThreadBaseClass is not
	 * started.
	 *
	 * \param numThreads - An integer containing the number of work threads
	 */
	void setNumThreads(int numThreads);

	/**
	 * \brief Function to set thread health
	 *
	 * This function signifies the thread health by using setLastHealthy
	 * to set m_tLastHealthy for the current thread to now if health is true
	 *
	 * \param health = A boolean value indicating thread health, true indicates
	 * that setLastHealthy to set m_tLastHealthy to now, false indicates
	 * it should not
	 */
	void setThreadHealth(bool health = true);

	/**
	 * \brief work threads check function
	 *
	 * Checks to see if each thread that runs the workLoop() function is still
	 * operational, by checking the value of m_ThreadHealthMap[id] (==true)
	 * every m_iHealthCheckInterval seconds, setting m_ThreadHealthMap[id] to
	 * now after the check.
	 *
	 * \return returns true if the pool is still running after
	 * m_iHealthCheckInterval seconds, false otherwise
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
	 * \brief Function to get the work threads state
	 *
	 * This function gets the state of the threads by getting the value of
	 * m_WorkThreadsState.
	 *
	 * \return Returns a glass3::util::ThreadState enumeration value
	 * representing the threads state
	 */
	glass3::util::ThreadState getWorkThreadsState();

	/**
	 * \brief Function to set work threads health check interval
	 *
	 * This function sets the time interval after which an entry in
	 * m_ThreadHealthMap being older than now minus the time interval (not
	 * responded) indicates that the work thread has died in healthCheck()
	 *
	 * \param interval = An integer value indicating the work thread health
	 * check interval in seconds
	 */
	void setHealthCheckInterval(int interval);

	/**
	 * \brief Function to retrieve the work threads health check interval
	 *
	 * This function retrieves the time interval after which an entry in
	 * m_ThreadHealthMap being older than now minus the time interval (not
	 * responded) indicates that the work thread has died in healthCheck()
	 *
	 * \return An integer value containing the work thread health check interval
	 * in seconds
	 */
	int getHealthCheckInterval();

	/**
	 * \brief Function to retrieve the name of the work threads
	 *
	 * This function retrieves the name of the work threads, this name is used to
	 * identify the work threads
	 *
	 * \return A std::string containing the thread name
	 */
	const std::string& getThreadName();

	/**
	 * \brief Function to retrieve the oldest time any of the work threads last
	 * updated their health status as healthy
	 *
	 * This function retrieves the oldest time any of the the health statuses of
	 * the work threads was updated as healthy by the setThreadHealth function
	 *
	 * \return A std::time_t containing the last check time
	 */
	std::time_t getAllLastHealthy();

	/**
	 * \brief ThreadBaseClass work function
	 *
	 * This function is a pure virtual work function. It is intended that the
	 * derived class implement the desired thread work by overriding this
	 * function
	 * \return This function returns glass3::util::WorkState, indicating whether
	 * the work was successful, encountered an error, or was idle (no work to
	 * perform)
	 */
	virtual glass3::util::WorkState work() = 0;

 protected:
	/**
	 * \brief Function to set threads state
	 *
	 * This function signifies the threads state by setting m_WorkThreadsState
	 * to the provided value. This function is called by start(), stop(), and
	 * workLoop() to indicate various thread states during the thread startup
	 * and shutdown processes
	 *
	 * \param state = A glass3::util::ThreadState enumeration value indicating
	 * the new thread state
	 */
	void setWorkThreadsState(glass3::util::ThreadState state);

	/**
	 * \brief ThreadBaseClass work loop function
	 *
	 * This function is the thread work loop function. It runs in a loop while
	 * m_WorkThreadsState is glass3::util::ThreadState::Started, calls work()
	 * every m_iSleepTimeMS milliseconds, to do a unit of work, sets thread(s)
	 * health via setThreadHealth(). Setting
	 * m_WorkThreadsState to not equal glass3::util::ThreadState::Started via
	 * setWorkThreadsState, or a glass3::util::WorkState::Error return from
	 * work() will cause the loop to exit, and the function to return (ending
	 * the thread(s))
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

	/**
	 * \brief the std::vector that contains the work std::thread objects
	 */
	std::vector<std::thread> m_WorkThreads;

	/**
	 * \brief A std::map containing the epoch times as std::atomic<double>> that
	 * each work thread was last marked as healthy, identified by the
	 * thread id
	 */
	std::map<std::thread::id, std::atomic<int>> m_ThreadHealthMap;

 private:
	/**
	 * \brief the std::string containing the name of the works thread,
	 * used for logging
	 */
	std::string m_sThreadName;

	/**
	 * \brief the integer interval in seconds after which the work threads
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	std::atomic<int> m_iHealthCheckInterval;

	/**
	 * \brief An integer containing the number of work threads.
	 * \note can only be changed when the work threads are not running
	 */
	std::atomic<int> m_iNumThreads;

	/**
	 * \brief glass3::util::ThreadState enumeration used to track thread status,
	 * set by setWorkThreadsState()
	 */
	std::atomic<ThreadState> m_WorkThreadsState;

	/**
	 * \brief integer variable indicating how long to sleep in milliseconds
	 * between work() calls in workloop()
	 */
	std::atomic<int> m_iSleepTimeMS;

	bool m_bTerminate;

	// constants
	/**
	 * \brief default health check interval in seconds
	 */
	static const int k_iHeathCheckIntervalDefault = 30;

	/**
	 * \brief default sleep time in milliseconds
	 */
	static const int k_iSleepTimeDefault = 100;

	/**
	 * \brief default number of threads
	 */
	static const int k_iNumThreadsDefault = 10;
};
}  // namespace util
}  // namespace glass3
#endif  // THREADBASECLASS_H
