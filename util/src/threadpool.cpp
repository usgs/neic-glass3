#include <threadpool.h>
#include <logger.h>
#include <map>
#include <string>

namespace glass3 {
namespace util {

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool() {
	// init variables
	setPoolName("NYI");
	setRunning(false);
	setNumThreads(0);
	setSleepTime(100);
	setHealthCheckInterval(1);
	setLastHealthCheck(std::time(nullptr));
}

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool(std::string poolname, int num_threads, int sleeptime,
						int checkinterval) {
	// init variables
	setPoolName(poolname);
	setRunning(false);
	setNumThreads(num_threads);
	setSleepTime(sleeptime);
	setHealthCheckInterval(checkinterval);
	setLastHealthCheck(std::time(nullptr));

	start();
}

// ---------------------------------------------------------~ThreadPool
ThreadPool::~ThreadPool() {
	// stop the threads.
	stop();
}

// ---------------------------------------------------------start
bool ThreadPool::start() {
	logger::log("trace",
				"ThreadPool::start(): Starting pool. (" + getPoolName() + ")");

	// are we already running
	if (isRunning() == true) {
		logger::log(
				"warning",
				"ThreadPool::start(): Pool is already running. ("
						+ getPoolName() + ")");
		return (false);
	}

	// make sure we have no threads
	if (m_ThreadPool.size() > 0) {
		logger::log(
				"warning",
				"ThreadPool::start(): Pool Threads are already allocated. ("
						+ getPoolName() + ")");
		return (false);
	}

	setRunning(true);

	// create threads in pool
	for (int i = 0; i < getNumThreads(); i++) {
		// create thread
		m_ThreadPool.push_back(std::thread(&ThreadPool::jobLoop, this));

		// add to status map if we're tracking status
		if (getHealthCheckInterval() > 0) {
			m_StatusMutex.lock();
			m_ThreadStatusMap[m_ThreadPool[i].get_id()] = true;
			m_StatusMutex.unlock();
		}

		logger::log(
				"debug",
				"ThreadPool::start(): Created Thread #" + std::to_string(i)
						+ " (" + getPoolName() + ")");
	}

	return (true);
}

// ---------------------------------------------------------stop
bool ThreadPool::stop() {
	logger::log("trace",
				"ThreadPool::ThreadPool(): Stop. (" + getPoolName() + ")");

	// disable status checking
	m_StatusMutex.lock();
	m_ThreadStatusMap.clear();
	m_StatusMutex.unlock();

	// signal threads to finish
	setRunning(false);
	// wait for threads to finish
	for (int i = 0; i < m_ThreadPool.size(); i++) {
		try {
			m_ThreadPool[i].join();
		} catch (const std::system_error& e) {
			logger::log(
					"warning",
					"ThreadPool::stop(): Exception " + std::string(e.what())
							+ " joining thread #" + std::to_string(i) + "("
							+ getPoolName() + ")");
		}
	}

	// we're no longer running
	setAllJobsHealth(false);

	logger::log("debug",
				"ThreadPool::ThreadPool(): Stopped. (" + getPoolName() + ")");

	return (true);
}

// ---------------------------------------------------------addJob
void ThreadPool::addJob(std::function<void()> newjob) {
	// add the job to the queue
	getMutex().lock();
	m_JobQueue.push(newjob);
	getMutex().unlock();

	logger::log("debug",
				"ThreadPool::addJob(): Added Job.(" + getPoolName() + ")");
}

// ---------------------------------------------------------jobLoop
void ThreadPool::jobLoop() {
	logger::log("debug",
				"ThreadPool::jobLoop(): Thread Start.(" + getPoolName() + ")");

	while (isRunning() == true) {
		// make sure we're still running
		if (isRunning() == false)
			break;

		// update thread status
		setJobHealth(true);

		// lock for queue access
		getMutex().lock();

		// are there any jobs
		if (m_JobQueue.empty() == true) {
			// unlock and skip until next time
			getMutex().unlock();

			// give up some time at the end of the loop
			jobSleep();

			// on to the next loop
			continue;
		}

		// get the next job
		std::function<void()> newjob = m_JobQueue.front();
		m_JobQueue.pop();

		// done with queue
		getMutex().unlock();

		logger::log("debug",
					"ThreadPool::jobLoop(): Found Job.(" + getPoolName() + ")");

		// run the job
		try {
			newjob();
		} catch (const std::exception &e) {
			logger::log(
					"error",
					"ThreadPool::jobLoop: Exception during job(): "
							+ std::string(e.what()) + " (" + getPoolName()
							+ ")");
			break;
		}

		logger::log(
				"debug",
				"ThreadPool::jobLoop(): Finished Job.(" + getPoolName() + ")");

		// give up some time at the end of the loop
		jobSleep();
	}

	logger::log("debug",
				"ThreadPool::jobLoop(): Thread Exit.(" + getPoolName() + ")");
}

// ---------------------------------------------------------jobSleep
void ThreadPool::jobSleep() {
	if (isRunning() == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}
}

// ---------------------------------------------------------healthCheck
bool ThreadPool::healthCheck() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (getHealthCheckInterval() < 0)
		return (true);

	// thread is dead if we're not running
	if (isRunning() == false) {
		logger::log(
				"error",
				"ThreadPool::healthCheck(): m_bRunJobLoop is false. ("
						+ getPoolName() + ")");
		return (false);
	}

	// are there any threads? Not sure how this would happen,
	// but it's worth asking
	if (getNumThreads() == 0) {
		logger::log(
				"error",
				"ThreadPool::healthCheck(): all threads have exited! ("
						+ getPoolName() + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - getLastHealthCheck()) >= getHealthCheckInterval()) {
		// get the thread status
		if (getAllJobsHealth() != true) {
			logger::log(
					"error",
					"ThreadPool::healthCheck(): At least one thread"
							" did not respond (" + getPoolName()
							+ ") in the last"
							+ std::to_string(getHealthCheckInterval())
							+ "seconds.");

			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true again.
		setAllJobsHealth(false);

		// remember the last time we checked
		setLastHealthCheck(tNow);
	}

	// everything is awesome
	return (true);
}

// ---------------------------------------------------------setJobHealth
void ThreadPool::setJobHealth(bool status) {
	// update thread status
	std::lock_guard<std::mutex> guard(m_StatusMutex);
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
}

// ---------------------------------------------------------setAllJobsHealth
void ThreadPool::setAllJobsHealth(bool status) {
	// update thread status
	std::lock_guard<std::mutex> guard(m_StatusMutex);
	std::map<std::thread::id, bool>::iterator StatusItr;
	for (StatusItr = m_ThreadStatusMap.begin();
			StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
		StatusItr->second = status;
	}
}

// ---------------------------------------------------------getAllJobsHealth
bool ThreadPool::getAllJobsHealth() {
	// check thread status
	std::lock_guard<std::mutex> guard(m_StatusMutex);

	// empty check
	if (m_ThreadStatusMap.size() == 0) {
		return (false);
	}

	std::map<std::thread::id, bool>::iterator StatusItr;
	for (StatusItr = m_ThreadStatusMap.begin();
			StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
		// get the thread status
		bool status = static_cast<bool>(StatusItr->second);

		// at least one thread did not respond
		if (status != true) {
			return (false);
		}
	}
	return (true);
}

// ---------------------------------------------------------getJobQueueSize
int ThreadPool::getJobQueueSize() {
	std::lock_guard<std::mutex> guard(getMutex());
	int queuesize = static_cast<int>(m_JobQueue.size());
	return (queuesize);
}

// -------------------------------------------------------setHealthCheckInterval
void ThreadPool::setHealthCheckInterval(int interval) {
	m_iHealthCheckInterval = interval;
}

// -------------------------------------------------------getHealthCheckInterval
int ThreadPool::getHealthCheckInterval() {
	return (m_iHealthCheckInterval);
}

// ---------------------------------------------------------setRunning
void ThreadPool::setRunning(bool running) {
	m_bRunJobLoop = running;
}

// ---------------------------------------------------------setNumThreads
bool ThreadPool::isRunning() {
	return (m_bRunJobLoop);
}

// ---------------------------------------------------------setNumThreads
void ThreadPool::setNumThreads(int num) {
	m_iNumThreads = num;
}

// ---------------------------------------------------------getNumThreads
int ThreadPool::getNumThreads() {
	return (m_iNumThreads);
}

// ---------------------------------------------------------setSleepTime
void ThreadPool::setSleepTime(int sleeptimems) {
	m_iSleepTimeMS = sleeptimems;
}

// ---------------------------------------------------------getSleepTime
int ThreadPool::getSleepTime() {
	return (m_iSleepTimeMS);
}

// ---------------------------------------------------------setPoolName
void ThreadPool::setPoolName(std::string name) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sPoolName = name;
}

// ---------------------------------------------------------getPoolName
const std::string& ThreadPool::getPoolName() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sPoolName);
}

// ---------------------------------------------------------setLastHealthCheck
void ThreadPool::setLastHealthCheck(time_t now) {
	m_tLastHealthCheck = now;
}

// ---------------------------------------------------------getLastHealthCheck
time_t ThreadPool::getLastHealthCheck() {
	return ((time_t)m_tLastHealthCheck);
}

}  // namespace util
}  // namespace glass3
