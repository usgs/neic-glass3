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
	setCheckInterval(1);
	setLastCheck(std::time(nullptr));
}

// ---------------------------------------------------------ThreadPool
ThreadPool::ThreadPool(std::string poolname, int num_threads, int sleeptime,
						int checkinterval) {
	// init variables
	setPoolName(poolname);
	setRunning(false);
	setNumThreads(num_threads);
	setSleepTime(sleeptime);
	setCheckInterval(checkinterval);
	setLastCheck(std::time(nullptr));

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
		if (getCheckInterval() > 0) {
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
	setAllStatus(false);

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
		setStatus(true);

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

	// one less thread, don't bother to remove from status checking
	// we want to know this thread exited
	setNumThreads(getNumThreads() - 1);

	logger::log("debug",
				"ThreadPool::jobLoop(): Thread Exit.(" + getPoolName() + ")");
}

// ---------------------------------------------------------jobSleep
void ThreadPool::jobSleep() {
	if (isRunning() == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}
}

// ---------------------------------------------------------check
bool ThreadPool::check() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (getCheckInterval() < 0)
		return (true);

	// thread is dead if we're not running
	if (isRunning() == false) {
		logger::log(
				"error",
				"ThreadPool::check(): m_bRunJobLoop is false. (" + getPoolName()
						+ ")");
		return (false);
	}

	// are there any threads? Not sure how this would happen,
	// but it's worth asking
	if (getNumThreads() == 0) {
		logger::log(
				"error",
				"ThreadPool::check(): all threads have exited! ("
						+ getPoolName() + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - getLastCheck()) >= getCheckInterval()) {
		// get the thread status
		if (getStatus() != true) {
			logger::log(
					"error",
					"ThreadPool::check(): At least one thread"
							" did not respond (" + getPoolName()
							+ ") in the last"
							+ std::to_string(getCheckInterval()) + "seconds.");

			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true again.
		setAllStatus(false);

		// remember the last time we checked
		setLastCheck(tNow);
	}

	// everything is awesome
	return (true);
}

// ---------------------------------------------------------setStatus
void ThreadPool::setStatus(bool status) {
	// update thread status
	std::lock_guard<std::mutex> guard(m_StatusMutex);
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
}

// ---------------------------------------------------------setAllStatus
void ThreadPool::setAllStatus(bool status) {
	// update thread status
	std::lock_guard<std::mutex> guard(m_StatusMutex);
	std::map<std::thread::id, bool>::iterator StatusItr;
	for (StatusItr = m_ThreadStatusMap.begin();
			StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
		StatusItr->second = status;
	}
}

// ---------------------------------------------------------getStatus
bool ThreadPool::getStatus() {
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

// ---------------------------------------------------------setCheckInterval
void ThreadPool::setCheckInterval(int interval) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_iCheckInterval = interval;
}

// ---------------------------------------------------------getCheckInterval
int ThreadPool::getCheckInterval() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_iCheckInterval);
}

// ---------------------------------------------------------setRunning
void ThreadPool::setRunning(bool running) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_bRunJobLoop = running;
}

// ---------------------------------------------------------setNumThreads
bool ThreadPool::isRunning() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_bRunJobLoop);
}

// ---------------------------------------------------------setNumThreads
void ThreadPool::setNumThreads(int num) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_iNumThreads = num;
}

// ---------------------------------------------------------getNumThreads
int ThreadPool::getNumThreads() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_iNumThreads);
}

// ---------------------------------------------------------setSleepTime
void ThreadPool::setSleepTime(int sleeptimems) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_iSleepTimeMS = sleeptimems;
}

// ---------------------------------------------------------getSleepTime
int ThreadPool::getSleepTime() {
	std::lock_guard<std::mutex> guard(getMutex());
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

// ---------------------------------------------------------setLastCheck
void ThreadPool::setLastCheck(time_t now) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_tLastCheck = now;
}

// ---------------------------------------------------------getLastCheck
time_t ThreadPool::getLastCheck() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_tLastCheck);
}

}  // namespace util
}  // namespace glass3
