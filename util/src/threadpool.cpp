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
	setNumThreads(0);
	setSleepTime(100);
	setHealthCheckInterval(1);
	setThreadState(glass3::util::ThreadState::Initialized);
}

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool(std::string poolname, int num_threads, int sleeptime,
						int checkinterval) {
	// init variables
	setPoolName(poolname);
	setNumThreads(num_threads);
	setSleepTime(sleeptime);
	setHealthCheckInterval(checkinterval);
	setThreadState(glass3::util::ThreadState::Initialized);

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
	if ((getThreadState() == glass3::util::ThreadState::Starting)
			|| (getThreadState() == glass3::util::ThreadState::Started)) {
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

	// we're starting
	setThreadState(glass3::util::ThreadState::Started);

	// create threads in pool
	for (int i = 0; i < getNumThreads(); i++) {
		// create thread
		m_ThreadPool.push_back(std::thread(&ThreadPool::jobLoop, this));

		// add to status map if we're tracking status
		if (getHealthCheckInterval() > 0) {
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
	if ((getThreadState() == glass3::util::ThreadState::Stopping)
			|| (getThreadState() == glass3::util::ThreadState::Stopped)
			|| (getThreadState() == glass3::util::ThreadState::Initialized)) {
		logger::log("warning",
					"ThreadPool::stop(): Work Threads is are not running, "
							"or is already stopping. (" + getPoolName() + ")");
		return (false);
	}

	// disable status checking
	m_ThreadHealthMap.clear();

	// we're stopping
	setThreadState(glass3::util::ThreadState::Stopping);

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
	setThreadState(glass3::util::ThreadState::Stopped);

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
	while (getThreadState() == glass3::util::ThreadState::Started) {
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
	if (getThreadState() == glass3::util::ThreadState::Started) {
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
	if (getThreadState() == glass3::util::ThreadState::Initialized) {
		return (true);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	int lastCheckInterval = (tNow - getAllLastHealthy());
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
void ThreadPool::setJobHealth(bool health) {
	if (health == true) {
		std::time_t tNow;
		std::time(&tNow);
		setLastHealthy(tNow);
	} else {
		setLastHealthy(0);
		logger::log("warning",
					"ThreadPool::setThreadHealth(): health set to false");
	}
}

// ---------------------------------------------------------setAllJobsHealth
void ThreadPool::setAllJobsHealth(bool health) {
	if (health == true) {
		std::time_t tNow;
		std::time(&tNow);
		setAllLastHealthy(tNow);
	} else {
		setAllLastHealthy(0);
		logger::log("warning",
					"ThreadPool::setAllJobsHealth(): health set to false");
	}
}

// ---------------------------------------------------------getLastHealthy
std::time_t ThreadPool::getLastHealthy() {
	// get this thread status
	if (m_ThreadHealthMap.find(std::this_thread::get_id())
			!= m_ThreadHealthMap.end()) {
		return (m_ThreadHealthMap[std::this_thread::get_id()]);
	}
	return (0);
}

// ---------------------------------------------------------getAllLastHealthy
std::time_t ThreadPool::getAllLastHealthy() {
	// empty check
	if (m_ThreadHealthMap.size() == 0) {
		return (0);
	}

	// init oldest time to now
	double oldestTime = std::time(nullptr);

	// go through all threads in the pool
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

// ---------------------------------------------------------setThreadState
void ThreadPool::setThreadState(glass3::util::ThreadState status) {
	m_bThreadState = status;
}

// ---------------------------------------------------------getThreadState
glass3::util::ThreadState ThreadPool::getThreadState() {
	return (m_bThreadState);
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

// ---------------------------------------------------------setLastHealthy
void ThreadPool::setLastHealthy(std::time_t now) {
	// update this thread status
	if (m_ThreadHealthMap.find(std::this_thread::get_id())
			!= m_ThreadHealthMap.end()) {
		m_ThreadHealthMap[std::this_thread::get_id()] = now;
	}
}

// ---------------------------------------------------------setAllLastHealthy
void ThreadPool::setAllLastHealthy(std::time_t now) {
	// update each thread status
	std::map<std::thread::id, std::atomic<double>>::iterator StatusItr;
	for (StatusItr = m_ThreadHealthMap.begin();
			StatusItr != m_ThreadHealthMap.end(); ++StatusItr) {
		StatusItr->second = now;
	}
}

}  // namespace util
}  // namespace glass3
