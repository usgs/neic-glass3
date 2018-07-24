#include <threadpool.h>
#include <logger.h>
#include <map>
#include <string>

namespace glass3 {
namespace util {

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool() {
	// init variables
	setPoolName("unknown");
	setNumThreads(5);
	setSleepTime(100);
	setHealthCheckInterval(30);
	setThreadPoolState(glass3::util::ThreadState::Initialized);
}

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool(std::string poolName, int numThreads, int sleepTime,
						int checkInterval) {
	// init variables
	setPoolName(poolName);
	setNumThreads(numThreads);
	setSleepTime(sleepTime);
	setHealthCheckInterval(checkInterval);
	setThreadPoolState(glass3::util::ThreadState::Initialized);

	start();
}

// ---------------------------------------------------------~ThreadPool
ThreadPool::~ThreadPool() {
	// stop the threads.
	stop();
}

// ---------------------------------------------------------start
bool ThreadPool::start() {
	// are we already running
	if ((getThreadPoolState() != glass3::util::ThreadState::Initialized)
			 && (getThreadPoolState() != glass3::util::ThreadState::Stopped)) {
		logger::log("warning",
					"ThreadPool::start(): Work Thread is already starting "
							"or running. (" + getPoolName() + ")");
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

	// we're starting (can't set thread state to starting because
	// that would cause the job loops to immediately exit,
	// so we set to started, and we never check against starting).
	setThreadPoolState(glass3::util::ThreadState::Started);

	// create threads in pool
	for (int i = 0; i < getNumThreads(); i++) {
		// create thread
		m_ThreadPool.push_back(std::thread(&ThreadPool::jobLoop, this));

		// add to status map if we're tracking status
		if (getHealthCheckInterval() > 0) {
			// insert a new key into the health map for this thread
			// to track status
			m_ThreadHealthMap[m_ThreadPool[i].get_id()] = std::time(nullptr);
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
	// check if we're running
	if (getThreadPoolState() != glass3::util::ThreadState::Started) {
		logger::log("warning",
					"ThreadPool::stop(): Work Threads is are not running, "
							"or is already stopping. (" + getPoolName() + ")");
		return (false);
	}

	// we're stopping
	setThreadPoolState(glass3::util::ThreadState::Stopping);

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

	// we're now stopped
	setThreadPoolState(glass3::util::ThreadState::Stopped);

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
	// run until told to stop
	while (getThreadPoolState() == glass3::util::ThreadState::Started) {
		// update thread status
		setJobHealth();

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
	// only sleep if we're running, if we are not, we don't want to
	// wait to exit.
	if (getThreadPoolState() == glass3::util::ThreadState::Started) {
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}
}

// ---------------------------------------------------------healthCheck
bool ThreadPool::healthCheck() {
	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (getHealthCheckInterval() < 0) {
		return (true);
	}

	// are there any threads? Not sure how this would happen,
	// but it's worth asking
	if (getNumThreads() == 0) {
		logger::log(
				"error",
				"ThreadPool::healthCheck(): no threads in pool! ("
						+ getPoolName() + ")");
		return (false);
	}

	// don't check if we've not started yet
	if (getThreadPoolState() == glass3::util::ThreadState::Initialized) {
		return (true);
	}

	// see if it's time to check
	int lastCheckInterval = (std::time(nullptr) - getAllLastHealthy());
	if (lastCheckInterval > getHealthCheckInterval()) {
		logger::log(
				"error",
				"ThreadPool::healthCheck():"
						" lastCheckInterval for at least one thread in"
						+ getPoolName() + " exceeds health check interval ( "
						+ std::to_string(lastCheckInterval) + " > "
						+ std::to_string(getHealthCheckInterval()) + " )");

		return (false);
	}

	// everything is awesome
	return (true);
}

// ---------------------------------------------------------setJobHealth
void ThreadPool::setJobHealth() {
	setLastHealthy(std::time(nullptr));
}

// ---------------------------------------------------------getAllLastHealthy
std::time_t ThreadPool::getAllLastHealthy() {
	// empty check
	if (m_ThreadHealthMap.size() == 0) {
		return (0);
	}

	// don't bother if we're not running
	if (getThreadPoolState() != glass3::util::ThreadState::Started) {
		return (0);
	}

	// init oldest time to now
	double oldestTime = std::time(nullptr);

	// go through all threads in the pool
	// I don't think we need a mutex here because the only function that
	// can modify the size of a map is start()
	std::map<std::thread::id, std::atomic<double>>::iterator StatusItr;
	for (StatusItr = m_ThreadHealthMap.begin();
			StatusItr != m_ThreadHealthMap.end(); ++StatusItr) {
		// get the thread status
		double healthTime = static_cast<double>(StatusItr->second);

		// at least one thread did not respond
		if (healthTime < oldestTime) {
			oldestTime = healthTime;
		}
	}
	return (oldestTime);
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

// ---------------------------------------------------------setThreadPoolState
void ThreadPool::setThreadPoolState(glass3::util::ThreadState status) {
	m_ThreadPoolState = status;
}

// ---------------------------------------------------------getThreadPoolState
glass3::util::ThreadState ThreadPool::getThreadPoolState() {
	return (m_ThreadPoolState);
}

// ---------------------------------------------------------setNumThreads
void ThreadPool::setNumThreads(int numThreads) {
	if (getThreadPoolState() == glass3::util::ThreadState::Started) {
		logger::log("warning",
					"ThreadPool::setNumThreads(): Cannot change number of "
					"threads while thread pool is running");
		return;
	}

	m_iNumThreads = numThreads;
}

// ---------------------------------------------------------getNumThreads
int ThreadPool::getNumThreads() {
	return (m_iNumThreads);
}

// ---------------------------------------------------------setSleepTime
void ThreadPool::setSleepTime(int sleepTimeMS) {
	m_iSleepTimeMS = sleepTimeMS;
}

// ---------------------------------------------------------getSleepTime
int ThreadPool::getSleepTime() {
	return (m_iSleepTimeMS);
}

// ---------------------------------------------------------setPoolName
void ThreadPool::setPoolName(std::string poolName) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sPoolName = poolName;
}

// ---------------------------------------------------------getPoolName
const std::string& ThreadPool::getPoolName() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sPoolName);
}

// ---------------------------------------------------------setLastHealthy
void ThreadPool::setLastHealthy(std::time_t now) {
	// don't bother if we're not running
	if (getThreadPoolState() != glass3::util::ThreadState::Started) {
		return;
	}

	// update this thread status
	if (m_ThreadHealthMap.find(std::this_thread::get_id())
			!= m_ThreadHealthMap.end()) {
		m_ThreadHealthMap[std::this_thread::get_id()] = now;
	}
}

}  // namespace util
}  // namespace glass3
