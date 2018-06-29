/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <baseclass.h>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <map>
#include <ctime>
#include <string>

namespace util {
/**
 * \brief util threadpool class
 *
 * The util threadpool class is a class encapsulating thread pooling
 * logic. The baseclass class supports creating and starting a
 * pool of worker threads that can run various job functions
 *
 */
class ThreadPool : public util::BaseClass {
 public:
	ThreadPool();

	/**
	 * \brief threadpool constructor
	 *
	 * The constructor for the threadpool class.
	 *
	 * \param poolname - A std::string containing the name of the thread pool
	 * \param num_threads - An integer containing the number of
	 * threads in the pool.  Default 5
	 * \param sleeptime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 100
	 * \param checkinterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 10.
	 */
	ThreadPool(std::string poolname, int num_threads = 5, int sleeptime = 100,
				int checkinterval = 30);

	/**
	 * \brief threadpool destructor
	 *
	 * The destructor for the threadpool class.
	 */
	~ThreadPool();

	/**
	 * \brief add a job for for threadpool
	 *
	 * Adds a job to the queue of jobs to be run by the thread
	 * pool
	 * \param newjob - A std::function<void()> bound to the function
	 * containing the job to run
	 */
	void addJob(std::function<void()> newjob);

	bool start();

	/**
	 * \brief threadpool stop function
	 *
	 * Stops and waits for each thread in the threadpool
	 */
	bool stop();

	/**
	 * \brief check to see if threadpool is still functional
	 *
	 * Checks each thread in the pool to see if it is still responsive.
	 */
	bool check();

	/**
	 * \brief threadpool status update function
	 *
	 * Updates the status for each thread in the threadpool
	 * \param status - A boolean flag containing the status to set
	 */
	void setStatus(bool status);

	void setAllStatus(bool status);

	bool getStatus();

	/**
	 *\brief get the current number of jobs in the queue
	 */
	int getJobQueueSize();

	/**
	 *\brief getter for m_iNumThreads
	 */
	int getNumThreads();

	/**
	 * \brief Sets the time to sleep between work() calls
	 *
	 * Sets the amount of time to sleep between work() function calls in the
	 * workLoop() function, which is run by the thread
	 *
	 * \param sleeptimems - An integer value containing the sleep between work()
	 * calls in integer milliseconds.
	 */
	void setSleepTime(int sleeptimems);

	/**
	 *\brief getter for m_iSleepTimeMS
	 */
	int getSleepTime();

	/**
	 * \brief Function to set the name of the work thread
	 *
	 * This function sets the name of the work thread, this name is used to
	 * identify the work thread in logging
	 *
	 * \param name = A std::string containing the thread name to set
	 */
	void setPoolName(std::string name);

	/**
	 *\brief getter for m_sPoolName
	 */
	const std::string& getPoolName();

	/**
	 * \brief Function to set thread health check interval
	 *
	 * This function sets the time interval after which m_bCheckWorkThread being
	 * false (not responded) indicates that the thread has died in check()
	 *
	 * \param interval = An integer value indicating the thread health check
	 * interval in seconds
	 */
	void setCheckInterval(int interval);

	/**
	 * \brief Function to retrieve the thread health check interval
	 *
	 * This function retreives the time interval after which m_bCheckWorkThread
	 * being false (not responded) indicates that the thread has died in check()
	 *
	 * \return An integer value containing the thread health check interval in
	 * seconds
	 */
	int getCheckInterval();

	/**
	 * \brief Checks to see if the work thread should still be running
	 *
	 * This function checks to see if the work thread should still running by
	 * returning the value of m_bRunWorkThread.
	 * \return Returns true if the thread should still running, false if it
	 * has been stopped
	 */
	virtual bool isRunning();

	/**
	 * \brief Function to retrieve the last time the work thread health status
	 * was checked
	 *
	 * This function retrieves the last time the health status of this thread
	 * was checked by the check() function
	 *
	 * \return A std::string containing the thread name
	 */
	std::time_t getLastCheck();

 protected:
	/**
	 * \brief the job work loop
	 *
	 * The thread loop that pulls jobs from the queue and runs them.
	 */
	void jobLoop();

	/**
	 * \brief the job work loop
	 *
	 * The function that performs the sleep between jobs
	 */
	void jobSleep();

	/**
	 * \brief Function to sset the last time the work thread health status
	 * was checked
	 *
	 * This function sets the last time the health status of this thread
	 * was checked.
	 *
	 * \param now - A std::time_t containing the last time the health status
	 * was checked
	 */
	void setLastCheck(std::time_t now);

	/**
	 * \brief Sets whether the work thread is running
	 *
	 * This function sets m_bRunWorkThread. Setting m_bRunWorkThread to true
	 * indicates that the thread should still run (via workloop()). Setting
	 * m_bRunWorkThread to false indicates that the thread should stop (and
	 * workloop() should return)
	 */
	void setRunning(bool running);

	void setNumThreads(int num);

 private:
	/**
	 * \brief the std::vector of std::threads in the pool
	 */
	std::vector<std::thread> m_ThreadPool;

	/**
	 * \brief A std::map containing the status of each thread
	 */
	std::map<std::thread::id, bool> m_ThreadStatusMap;

	/**
	 * \brief the std::mutex for m_ThreadStatusMap
	 */
	std::mutex m_StatusMutex;

	/**
	 * \brief the std::queue of std::function<void() jobs
	 */
	std::queue<std::function<void()>> m_JobQueue;

	/**
	 * \brief the boolean flags indicating that the jobloop threads
	 * should keep running.
	 */
	bool m_bRunJobLoop;

	/**
	 * \brief An integer containing the amount of
	 * time to sleep in milliseconds between jobs.
	 */
	int m_iSleepTimeMS;

	/**
	 * \brief An integer containing the number of
	 * threads in the pool.
	 */
	int m_iNumThreads;

	/**
	 * \brief the std::string containing the name of the pool
	 */
	std::string m_sPoolName;

	/**
	 * \brief the time_t holding the last time the thread status was checked
	 */
	time_t m_tLastCheck;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	int m_iCheckInterval;
};
}  // namespace util
#endif  // THREADPOOL_H
