/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <baseclass.h>
#include <threadbaseclass.h>
#include <threadstate.h>
#include <thread>
#include <mutex>
#include <queue>
#include <ctime>
#include <string>

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::ThreadPool class
 *
 * This class supports creating, starting, stopping, and monitoring a pool of
 * threads that perform various jobs, managing a FIFO queue for jobs to be
 * processed, and allowing for specific pool name and sleep between jobs.
 *
 * This class inherits from util::threadbaseclass
 *
 */
class ThreadPool : public util::ThreadBaseClass {
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
	 * \brief ThreadBaseClass work function
	 *
	 * This function is a pure virtual work function. It is intended that the
	 * derived class implement the desired thread work by overriding this
	 * function
	 * \return This function returns glass3::util::WorkState, indicating whether
	 * the work was successful, encountered an error, or was idle (no work to
	 * perform
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief the std::queue of std::function<void() jobs for the jobLoop()
	 * functions to process
	 */
	std::queue<std::function<void()>> m_JobQueue;
};
}  // namespace util
}  // namespace glass3
#endif  // THREADPOOL_H
