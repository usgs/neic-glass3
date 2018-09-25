#include <threadbaseclass.h>
#include <logger.h>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace glass3 {
namespace util {

#define DEFAULT_HEALTHCHECK_INTERVAL 30
#define DEFAULT_SLEEP_TIME 100

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass()
		: util::BaseClass() {
	m_sThreadName = "unknown";

	setWorkThreadsState(glass3::util::ThreadState::Initialized);
	setHealthCheckInterval(DEFAULT_HEALTHCHECK_INTERVAL);
	setNumThreads(1);
	setThreadHealth();

	// set to default inter-loop sleep
	setSleepTime(DEFAULT_SLEEP_TIME);

	m_WorkThreads.clear();
}

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass(std::string threadName, int sleepTimeMS,
									int numThreads, int checkInterval)
		: util::BaseClass() {
	m_sThreadName = threadName;

	setWorkThreadsState(glass3::util::ThreadState::Initialized);
	setHealthCheckInterval(checkInterval);
	setNumThreads(numThreads);
	setThreadHealth();

	// set to provided inter-loop sleep
	setSleepTime(sleepTimeMS);

	m_WorkThreads.clear();
}

// ---------------------------------------------------------~ThreadBaseClass
ThreadBaseClass::~ThreadBaseClass() {
	try {
		stop();
	} catch (const std::exception& e) {
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::~ThreadBaseClass()(): Exception "
						+ std::string(e.what()));
	}
}

// ---------------------------------------------------------start
bool ThreadBaseClass::start() {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return (true);
	}
	// are we already running
	if ((getWorkThreadsState() != glass3::util::ThreadState::Initialized)
			&& (getWorkThreadsState() != glass3::util::ThreadState::Stopped)) {
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already starting "
						"or running. (" + getThreadName() + ")");
		return (false);
	}

	// make sure we have no threads
	if (m_WorkThreads.size() > 0) {
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Threads are already allocated. ("
						+ getThreadName() + ")");
		return (false);
	}

	// we're starting
	setWorkThreadsState(glass3::util::ThreadState::Starting);

	// create threads
	for (int i = 0; i < getNumThreads(); i++) {
		// create thread
		m_WorkThreads.push_back(std::thread(&ThreadBaseClass::workLoop, this));

		// add to status map if we're tracking status
		if (getHealthCheckInterval() > 0) {
			// insert a new key into the health map for this thread
			// to track status
			m_ThreadHealthMap[m_WorkThreads[i].get_id()] = std::time(nullptr);
		}
	}

	glass3::util::Logger::log(
			"trace",
			"ThreadBaseClass::start(): Started "
					+ std::to_string(m_WorkThreads.size()) + " Work Threads. ("
					+ getThreadName() + ")");
	return (true);
}

// ---------------------------------------------------------stop
bool ThreadBaseClass::stop() {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return (false);
	}
	if (m_WorkThreads.size() <= 0) {
		return (false);
	}

	// check if we're running
	if (getWorkThreadsState() != glass3::util::ThreadState::Started) {
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not running, "
						"or is already stopping. (" + getThreadName() + ")");
		return (false);
	}

	// we're stopping
	setWorkThreadsState(glass3::util::ThreadState::Stopping);

	// wait for threads to finish
	for (int i = 0; i < m_WorkThreads.size(); i++) {
		try {
			m_WorkThreads[i].join();
		} catch (const std::system_error& e) {
			glass3::util::Logger::log(
					"warning",
					"ThreadBaseClass::stop(): Exception "
							+ std::string(e.what()) + " joining work thread #"
							+ std::to_string(i) + "(" + getThreadName() + ")");
		}
	}

	m_WorkThreads.clear();

	setWorkThreadsState(glass3::util::ThreadState::Stopped);

	// done
	glass3::util::Logger::log(
			"debug",
			"ThreadBaseClass::stop(): Stopped Work Thread. (" + getThreadName()
					+ ")");
	return (true);
}

// ---------------------------------------------------------setThreadHealth
void ThreadBaseClass::setThreadHealth(bool health) {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return;
	}

	if (health == true) {
		std::time_t tNow;
		std::time(&tNow);
		setLastHealthy(tNow);
	} else {
		setLastHealthy(0);
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::setThreadHealth(): health set to false");
	}
}

// ---------------------------------------------------------healthCheck
bool ThreadBaseClass::healthCheck() {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return (true);
	}

	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (getHealthCheckInterval() < 0) {
		return (true);
	}

	// are there any threads? Not sure how this would happen,
	// but it's worth asking
	if (getNumThreads() == 0) {
		glass3::util::Logger::log(
				"error",
				"ThreadBaseClass::healthCheck(): no threads! ("
						+ getThreadName() + ")");
		return (false);
	}

	// don't check if we've not started yet
	if ((getWorkThreadsState() == glass3::util::ThreadState::Starting)
			|| (getWorkThreadsState() == glass3::util::ThreadState::Initialized)) {
		return (true);
	}

	// thread is dead if we're not running
	if (getWorkThreadsState() != glass3::util::ThreadState::Started) {
		glass3::util::Logger::log(
				"error",
				"ThreadBaseClass::healthCheck(): Thread is not running. ("
						+ getThreadName() + ")");
		return (false);
	}

	int lastCheckInterval = (std::time(nullptr) - getAllLastHealthy());
	if (lastCheckInterval > getHealthCheckInterval()) {
		glass3::util::Logger::log(
				"error",
				"ThreadBaseClass::healthCheck():"
						" lastCheckInterval for at least one thread in"
						+ getThreadName() + " exceeds health check interval ( "
						+ std::to_string(lastCheckInterval) + " > "
						+ std::to_string(getHealthCheckInterval()) + " )");

		return (false);
	}

	// everything is awesome
	return (true);
}

// ---------------------------------------------------------workLoop
void ThreadBaseClass::workLoop() {
	// we're running
	setWorkThreadsState(glass3::util::ThreadState::Started);

	// run until told to stop
	while (true) {
		// signal that we're still running
		setThreadHealth();
		glass3::util::WorkState workState;

		// do our work
		try {
			workState = work();
		} catch (const std::exception &e) {
			glass3::util::Logger::log(
					"error",
					"ThreadBaseClass::workLoop: Exception during work(): "
							+ std::string(e.what())
							+ "something's wrong, stopping thread. ("
							+ getThreadName() + ")");
			break;
		}

		// check our work return
		if (workState == glass3::util::WorkState::Error) {
			// something has gone wrong
			// break out of the loop
			glass3::util::Logger::log(
					"error",
					"ThreadBaseClass::workLoop(): Work returned error, "
							"something's wrong, stopping thread. ("
							+ getThreadName() + ")");
			break;
		} else if (workState == glass3::util::WorkState::Idle) {
			// give up some time if there was nothing to do
			std::this_thread::sleep_for(
					std::chrono::milliseconds(getSleepTime()));
		}

		// make sure we should still be running
		if (getWorkThreadsState() != glass3::util::ThreadState::Started) {
			glass3::util::Logger::log(
					"info",
					"ThreadBaseClass::workLoop(): Non-Starting thread "
							"status detected ("
							+ std::to_string(getWorkThreadsState())
							+ "), stopping thread. (" + getThreadName() + ")");
			break;
		}
	}

	glass3::util::Logger::log(
			"info",
			"ThreadBaseClass::workLoop(): Stopped thread. (" + getThreadName()
					+ ")");

	setWorkThreadsState(glass3::util::ThreadState::Stopped);

	// done with thread
	return;
}

// ---------------------------------------------------------getAllLastHealthy
std::time_t ThreadBaseClass::getAllLastHealthy() {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return (0);
	}

	// empty check
	if (m_ThreadHealthMap.size() == 0) {
		return (0);
	}

	// don't bother if we're not running
	if (getWorkThreadsState() != glass3::util::ThreadState::Started) {
		return (0);
	}

	// init oldest time to now
	double oldestTime = std::time(nullptr);

	// go through all work threads
	// I don't think we need a mutex here because the only function that
	// can modify the size of a map is start()
	std::map<std::thread::id, std::atomic<int>>::iterator StatusItr;
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

// -------------------------------------------------------setHealthCheckInterval
void ThreadBaseClass::setHealthCheckInterval(int interval) {
	m_iHealthCheckInterval = interval;
}

// -------------------------------------------------------getHealthCheckInterval
int ThreadBaseClass::getHealthCheckInterval() {
	return (m_iHealthCheckInterval);
}

// ---------------------------------------------------------setWorkThreadsState
void ThreadBaseClass::setWorkThreadsState(glass3::util::ThreadState status) {
	m_WorkThreadsState = status;
}

// ---------------------------------------------------------getWorkThreadsState
glass3::util::ThreadState ThreadBaseClass::getWorkThreadsState() {
	return (m_WorkThreadsState);
}

// ---------------------------------------------------------setNumThreads
void ThreadBaseClass::setNumThreads(int numThreads) {
	if (getWorkThreadsState() == glass3::util::ThreadState::Started) {
		glass3::util::Logger::log(
				"warning",
				"ThreadBaseClass::setNumThreads(): Cannot change number of "
				"threads while running");
		return;
	}

	m_iNumThreads = numThreads;
}

// ---------------------------------------------------------getNumThreads
int ThreadBaseClass::getNumThreads() {
	return (m_iNumThreads);
}

// ---------------------------------------------------------setSleepTime
void ThreadBaseClass::setSleepTime(int sleepTimeMS) {
	m_iSleepTimeMS = sleepTimeMS;
}

// ---------------------------------------------------------getSleepTime
int ThreadBaseClass::getSleepTime() {
	return (m_iSleepTimeMS);
}

// ---------------------------------------------------------getThreadName
const std::string& ThreadBaseClass::getThreadName() {
	return (m_sThreadName);
}

// ---------------------------------------------------------setLastHealthy
void ThreadBaseClass::setLastHealthy(time_t now) {
	// don't bother if we've not got any threads
	if (getNumThreads() <= 0) {
		return;
	}

	// don't bother if we're not running
	if (getWorkThreadsState() != glass3::util::ThreadState::Started) {
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
