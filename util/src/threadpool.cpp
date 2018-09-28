#include <threadpool.h>
#include <logger.h>
#include <string>

namespace glass3 {
namespace util {

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool()
		: util::ThreadBaseClass() {
}

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool(std::string poolName, int numThreads, int sleepTime,
						int checkInterval)
		: util::ThreadBaseClass(poolName, sleepTime, numThreads, checkInterval) {
	start();
}

// ---------------------------------------------------------~ThreadPool
ThreadPool::~ThreadPool() {
	// stop the threads.
	stop();
}

// ---------------------------------------------------------addJob
void ThreadPool::addJob(std::function<void()> newjob) {
	getMutex().lock();
	m_JobQueue.push(newjob);
	getMutex().unlock();

	glass3::util::Logger::log(
			"debug",
			"ThreadPool::addJob(): Added Job.(" + getThreadName() + ")");
}

// ---------------------------------------------------------work
glass3::util::WorkState ThreadPool::work() {
	// lock for queue access
	getMutex().lock();

	// are there any jobs
	if (m_JobQueue.empty() == true) {
		// unlock and skip until next time
		getMutex().unlock();

		// nothing to do
		return (glass3::util::WorkState::Idle);
	}

	// get the next job
	std::function<void()> newjob = m_JobQueue.front();
	m_JobQueue.pop();

	// done with queue
	getMutex().unlock();

	glass3::util::Logger::log(
			"debug",
			"ThreadPool::jobLoop(): Found Job.(" + getThreadName() + ")");

	// run the job
	try {
		newjob();
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"error",
				"ThreadPool::jobLoop: Exception during job(): "
						+ std::string(e.what()) + " (" + getThreadName() + ")");
		return (glass3::util::WorkState::Error);
	}

	glass3::util::Logger::log(
			"debug",
			"ThreadPool::jobLoop(): Finished Job.(" + getThreadName() + ")");

	// work was successful
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------getJobQueueSize
int ThreadPool::getJobQueueSize() {
	std::lock_guard<std::mutex> guard(getMutex());
	int queuesize = static_cast<int>(m_JobQueue.size());
	return (queuesize);
}

}  // namespace util
}  // namespace glass3
