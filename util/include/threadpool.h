/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <baseclass.h>
#include <threadstate.h>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <map>
#include <ctime>
#include <string>

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::ThreadPool class
 *
 * This class supports creating, starting, stopping, and monitoring a pool of
 * job threads, managing a FIFO queue for jobs to be processed, and allowing
 * for specific pool name and sleep between jobs.
 *
 * This class inherits from util::baseclass
 *
 * \note This class has no tie-in or relation with glass3::util::ThreadBaseClass
 */
class ThreadPool : public util::BaseClass {
 public:
	/**
	 * \brief ThreadPool constructor
	 *
	 * The constructor for the ThreadPool class.
	 * Initializes members to default values.
	 */
	ThreadPool();

	/**
	 * \brief An advanced constructor that sets up the ThreadPool with
	 * a provided pool name, number of threads, sleep between jobs time and
	 * status check interval
	 *
	 * The advanced constructor for the ThreadPool class.
	 * Initializes members to provided values.
	 *
	 * \param poolName - A std::string containing the name of the thread pool
	 * \param numThreads - An integer containing the number of
	 * threads in the pool.  Default 5
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 100
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 30.
	 */
	explicit ThreadPool(std::string poolName, int numThreads = 5,
						int sleepTime = 100, int checkInterval = 30);

	/**
	 * \brief ThreadPool destructor
	 *
	 * The destructor for the ThreadPool class.
	 */
	~ThreadPool();

	/**
	 * \brief pool start function
	 *
	 * Creates a pool of thread objects to run the jobLoop() function, and
	 * starts each thread, setting m_bRunJobLoop to be true.
	 *
	 * \return returns true if successful, false if the thread creation failed
	 * or if a thread pool had already been started
	 */
	bool start();

	/**
	 * \brief pool stop function
	 *
	 * Stops, waits for, and deletes each thread that runs the jobLoop() function,
	 * setting m_bRunJobLoop to false, and clearing m_ThreadHealthMap
	 *
	 * \return returns true if successful, false if the thread is not created and
	 * running
	 */
	bool stop();

	/**
	 * \brief pool health check function
	 *
	 * Checks to see if each thread that runs the jobLoop() function is still
	 * operational, by checking the value of m_ThreadHealthMap[id] (==true) every
	 * m_iHealthCheckInterval seconds, setting m_ThreadHealthMap[id] to false after the
	 * check. It is expected that the jobs processed by the thread pool will
	 * finish within m_iHealthCheckInterval seconds
	 *
	 * \return returns true if the pool is still running after m_iHealthCheckInterval
	 * seconds, false otherwise
	 */
	bool healthCheck();

	/**
	 * \brief add a new job for for the thread pool to process
	 *
	 * Adds a job, as signified by a general-purpose polymorphic function wrapper
	 * bound to the function to run, to the queue of jobs to be run by the thread
	 * pool.
	 *
	 * \param newJob - A std::function<void()> bound to the function containing
	 * the job to run
	 */
	void addJob(std::function<void()> newJob);

	/**
	 *\brief Retrieve the current number of jobs in the queue
	 *
	 * Retrieves the number of pending (queued but not started) jobs stored in
	 * m_JobQueue
	 *
	 *\return an integer containing the current number of jobs in the queue
	 */
	int getJobQueueSize();

	/**
	 * \brief pool status update function
	 *
	 * Updates the last time healthy for a specific thread running jobLoop() in
	 * the thread pool by setting setLastHealthy() to now
	 */
	void setJobHealth();

	/**
	 * \brief Retrieves the number of threads in the pool
	 *
	 * Retrieves the number of threads managed by this thread pool
	 *
	 * \return Returns the number of threads in the pool
	 */
	int getNumThreads();

	/**
	 * \brief Sets the number of job threads
	 *
	 * This function sets m_iNumThreads. This function sets the number of
	 * threads that should be in the thread pool, and thus will be started in
	 * start().
	 *
	 * \note This function is only effective when the ThreadPool is not started.
	 *
	 * \param numThreads - An integer containing the number of threads in the
	 * pool.
	 */
	void setNumThreads(int numThreads);

	/**
	 * \brief Sets the time to sleep between job queue checks
	 *
	 * Sets the amount of time to sleep between job queue checks in
	 * the jobLoop() function, which is run by each job thread
	 *
	 * \param sleepTimeMS - An integer value containing the sleep between job
	 * queue checks in integer milliseconds.
	 */
	void setSleepTime(int sleepTimeMS);

	/**
	 * \brief Retrieves the time to sleep between job queue checks
	 *
	 * Retrieves the amount of time to sleep between job queue checks in
	 * the jobLoop() function, which is run by each job thread
	 *
	 * \return Returns the amount of time to sleep between job queue checks in
	 * integer milliseconds.
	 */
	int getSleepTime();

	/**
	 * \brief Function to set the name of the thread pool
	 *
	 * This function sets the name of the thread pool, this name is used to
	 * identify the pool in logging
	 *
	 * \param poolName = A std::string containing the thread pool name to set
	 */
	void setPoolName(std::string poolName);

	/**
	 * \brief Function to retrieve the name of the thread pool
	 *
	 * This function retrieves the name of the thread pool, this name is used to
	 * identify the thread pool
	 *
	 * \return A std::string containing the thread pool name
	 */
	const std::string& getPoolName();

	/**
	 * \brief Function to get thread pool state
	 *
	 * This function gets the state of the thread pool by getting the value of
	 * m_ThreadPoolState.
	 *
	 * \return Returns a glass3::util::ThreadState enumeration value representing
	 * the thread pool state
	 */
	glass3::util::ThreadState getThreadPoolState();

	/**
	 * \brief Function to set thread pool health check interval
	 *
	 * This function sets the time interval after which an entry in
	 * m_ThreadHealthMap being older than now minus the time interval (not
	 * responded) indicates that the thread in the pool has died in healthCheck()
	 *
	 * \param interval = An integer value indicating the thread pool health check
	 * interval in seconds
	 */
	void setHealthCheckInterval(int interval);

	/**
	 * \brief Function to retrieve the thread pool health check interval
	 *
	 * This function retrieves the time interval after which an entry in
	 * m_ThreadHealthMap being older than now minus the time interval (not
	 * responded) indicates that the thread in the pool has died in healthCheck()
	 *
	 * \return An integer value containing the thread pool health check interval
	 * in seconds
	 */
	int getHealthCheckInterval();

	/**
	 * \brief Function to retrieve the oldest time any of the job threads last
	 * updated their health status as healthy
	 *
	 * This function retrieves the oldest time any of the the health statuses of
	 * the job threads was updated as healthy by the setJobHealth function
	 *
	 * \return A std::time_t containing the last check time
	 */
	std::time_t getAllLastHealthy();

 protected:
	/**
	 * \brief Function to set thread pool state
	 *
	 * This function signifies the thread pool state by setting m_ThreadPoolState
	 * to the provided value.
	 *
	 * \param state = A glass3::util::ThreadState enumeration value indicating
	 * the new thread state
	 */
	void setThreadPoolState(glass3::util::ThreadState state);

	/**
	 * \brief thread pool job loop function
	 *
	 * This function is the thread job loop function. It runs in a loop while
	 * m_bRunJobLoop is true, polling m_JobQueue for jobs to process, running
	 * a jobs, and sleeping via jobSleep()  between jobs and checks of the
	 * m_JobQueue
	 */
	void jobLoop();

	/**
	 * \brief the job loop sleep function
	 *
	 * The function that performs the sleep between jobs or checks of the
	 * m_JobQueue
	 */
	void jobSleep();

	/**
	 * \brief Function to set the last time a job thread was healthy
	 *
	 * This function sets the last time a job thread was healthy pool by using
	 * std::this_thread::get_id() to set the correct epoch time within
	 * m_ThreadHealthMap to the provided time
	 *
	 * \param now - A std::time_t containing the last time the thread was
	 * healthy
	 */
	void setLastHealthy(std::time_t now);

 private:
	/**
	 * \brief the std::vector that contains the std::thread objects in the pool
	 */
	std::vector<std::thread> m_ThreadPool;

	/**
	 * \brief A std::map containing the epoch times as std::atomic<double>> that
	 * each thread in the pool was last marked as healthy, identified by the
	 * thread id
	 */
	std::map<std::thread::id, std::atomic<double>> m_ThreadHealthMap;

	/**
	 * \brief the std::queue of std::function<void() jobs for the jobLoop()
	 * functions to process
	 */
	std::queue<std::function<void()>> m_JobQueue;

	/**
	 * \brief glass3::util::ThreadState enumeration used to track thread status,
	 * set by setThreadPoolState()
	 */
	std::atomic<ThreadState> m_ThreadPoolState;

	/**
	 * \brief An integer containing the amount of time to sleep in milliseconds
	 * between jobs.
	 */
	std::atomic<int> m_iSleepTimeMS;

	/**
	 * \brief An integer containing the number of threads in the pool.
	 * \note can only be changed when the job threads are not running
	 */
	std::atomic<int> m_iNumThreads;

	/**
	 * \brief the std::string containing the name of the thread pool
	 */
	std::string m_sPoolName;

	/**
	 * \brief the integer interval in seconds after which the a job thread
	 * in the pool will be considered dead. A negative check interval disables
	 * thread status checks
	 */
	std::atomic<int> m_iHealthCheckInterval;
};
}  // namespace util
}  // namespace glass3
#endif  // THREADPOOL_H
