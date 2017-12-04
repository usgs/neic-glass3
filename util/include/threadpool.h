/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

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
class ThreadPool {
 public:
	/**
	 * \brief threadpool constructor
	 *
	 * The constructor for the threadpool class.
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

	/**
	 * \brief check to see if threadpool is still functional
	 *
	 * Checks each thread in the pool to see if it is still responsive.
	 */
	bool check();

	/**
	 * \brief threadpool stop function
	 *
	 * Stops and waits for each thread in the threadpool
	 */
	bool stop();

	/**
	 * \brief threadpool status update function
	 *
	 * Updates the status for each thread in the threadpool
	 * \param status - A boolean flag containing the status to set
	 */
	void setStatus(bool status);

	/**
	 *\brief get the current number of jobs in the queue
	 */
	int getJobQueueSize() {
		m_QueueMutex.lock();
		int queuesize = static_cast<int>(m_JobQueue.size());
		m_QueueMutex.unlock();
		return queuesize;
	}

	/**
	 *\brief getter for m_bRunJobLoop
	 */
	bool getBRunJobLoop() const {
		return (m_bRunJobLoop);
	}

	/**
	 *\brief getter for m_iNumThreads
	 */
	int getINumThreads() const {
		return (m_iNumThreads);
	}

	/**
	 *\brief getter for m_iSleepTimeMS
	 */
	int getISleepTimeMs() const {
		return (m_iSleepTimeMS);
	}

	/**
	 *\brief getter for m_sPoolName
	 */
	const std::string& getSPoolName() const {
		return (m_sPoolName);
	}

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
	 * \brief the std::mutex for m_QueueMutex
	 */
	std::mutex m_QueueMutex;

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
	time_t tLastCheck;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	int m_iCheckInterval;
};
}  // namespace util
#endif  // THREADPOOL_H
