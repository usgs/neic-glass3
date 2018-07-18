#include <threadbaseclass.h>
#include <logger.h>
#include <thread>
#include <string>
#include <ctime>

namespace glass3 {
namespace util {

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass()
		: util::BaseClass() {
	setThreadName("unknown");
	setThreadState(glass3::util::ThreadState::Initialized);
	m_WorkThread = NULL;

	setHealthCheckInterval(1);
	setThreadHealth();

	setSleepTime(100);
}

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass(std::string threadname, int sleeptimems)
		: util::BaseClass() {
	setThreadName(threadname);
	setThreadState(glass3::util::ThreadState::Initialized);
	m_WorkThread = NULL;

	setHealthCheckInterval(1);
	setThreadHealth();

	setSleepTime(sleeptimems);
}

// ---------------------------------------------------------~ThreadBaseClass
ThreadBaseClass::~ThreadBaseClass() {
	logger::log(
			"debug",
			"ThreadBaseClass::~ThreadBaseClass(): Destruction. ("
					+ getThreadName() + ")");

	stop();
}

// ---------------------------------------------------------start
bool ThreadBaseClass::start() {
	// are we already running
	if ((getThreadState() == glass3::util::ThreadState::Starting)
			|| (getThreadState() == glass3::util::ThreadState::Started)) {
		logger::log("warning",
					"ThreadBaseClass::start(): Work Thread is already starting "
							"or running. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread != NULL) {
		logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already allocated. ("
						+ getThreadName() + ")");
		return (false);
	}

	// we're starting
	setThreadState(glass3::util::ThreadState::Starting);

	// start the thread
	m_WorkThread = new std::thread(&ThreadBaseClass::workLoop, this);

	logger::log(
			"debug",
			"ThreadBaseClass::start(): Started Work Thread. (" + getThreadName()
					+ ")");
	return (true);
}

// ---------------------------------------------------------stop
bool ThreadBaseClass::stop() {
	// check if we're running
	if ((getThreadState() == glass3::util::ThreadState::Stopping)
			|| (getThreadState() == glass3::util::ThreadState::Stopped)
			|| (getThreadState() == glass3::util::ThreadState::Initialized)) {
		logger::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not running, "
						"or is already stopping. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread == NULL) {
		logger::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not allocated. ("
						+ getThreadName() + ")");
		return (false);
	}

	// we're stopping
	setThreadState(glass3::util::ThreadState::Stopping);

	// wait for the thread to finish
	m_WorkThread->join();

	// delete it
	delete (m_WorkThread);
	m_WorkThread = NULL;

	// we're now stopped
	setThreadState(glass3::util::ThreadState::Stopped);

	// done
	logger::log(
			"debug",
			"ThreadBaseClass::stop(): Stopped Work Thread. (" + getThreadName()
					+ ")");
	return (true);
}

// ---------------------------------------------------------setThreadHealth
void ThreadBaseClass::setThreadHealth(bool health) {
	if (health == true) {
		std::time_t tNow;
		std::time(&tNow);
		setLastHealthy(tNow);
	} else {
		setLastHealthy(0);
		logger::log("warning",
					"ThreadBaseClass::setThreadHealth(): health set to false");
	}
}

// ---------------------------------------------------------healthCheck
bool ThreadBaseClass::healthCheck() {
	// don't check if we've not started yet
	if ((getThreadState() == glass3::util::ThreadState::Starting)
			|| (getThreadState() == glass3::util::ThreadState::Initialized)) {
		return (true);
	}

	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (getHealthCheckInterval() < 0) {
		return (true);
	}

	// thread is dead if we're not running
	if (getThreadState() != glass3::util::ThreadState::Started) {
		logger::log(
				"error",
				"ThreadBaseClass::healthCheck(): Thread is not running. ("
						+ getThreadName() + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	int lastCheckInterval = (tNow - getLastHealthy());
	if (lastCheckInterval > getHealthCheckInterval()) {
		// if the we've exceeded the health check interval, the thread
		// is dead
		logger::log(
				"error",
				"ThreadBaseClass::healthCheck(): lastCheckInterval for thread "
						+ getThreadName() + " exceeds health check interval ( "
						+ std::to_string(lastCheckInterval) + " > "
						+ std::to_string(getHealthCheckInterval()) + " )");
		return (false);
	}

	// we passed this time
	return (true);
}

// ---------------------------------------------------------workLoop
void ThreadBaseClass::workLoop() {
	// we're running
	setThreadState(glass3::util::ThreadState::Started);

	// run until told to stop
	while (getThreadState() == glass3::util::ThreadState::Started) {
		try {
			// do our work
			if (work() == false) {
				// something has gone wrong
				// break out of the loop
				logger::log(
						"error",
						"ThreadBaseClass::workLoop(): Work returned false, "
								"something's wrong, stopping thread. ("
								+ getThreadName() + ")");
				break;
			}
		} catch (const std::exception &e) {
			logger::log(
					"error",
					"ThreadBaseClass::workLoop: Exception during work(): "
							+ std::string(e.what()));
			break;
		}
		// signal that we're still running
		setThreadHealth();

		// give up some time at the end of the loop
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}

	logger::log(
			"info",
			"ThreadBaseClass::workLoop(): Stopped thread. (" + getThreadName()
					+ ")");

	setThreadState(glass3::util::ThreadState::Stopped);

	// done with thread
	return;
}

// ---------------------------------------------------------setThreadState
void ThreadBaseClass::setThreadState(glass3::util::ThreadState status) {
	m_bThreadState = status;
}

// ---------------------------------------------------------getThreadState
glass3::util::ThreadState ThreadBaseClass::getThreadState() {
	return (m_bThreadState);
}

// ---------------------------------------------------------setSleepTime
void ThreadBaseClass::setSleepTime(int sleeptimems) {
	m_iSleepTimeMS = sleeptimems;
}

// ---------------------------------------------------------getSleepTime
int ThreadBaseClass::getSleepTime() {
	return (m_iSleepTimeMS);
}

// -------------------------------------------------------setHealthCheckInterval
void ThreadBaseClass::setHealthCheckInterval(int interval) {
	m_iHealthCheckInterval = interval;
}

// -------------------------------------------------------getHealthCheckInterval
int ThreadBaseClass::getHealthCheckInterval() {
	return (m_iHealthCheckInterval);
}

// ---------------------------------------------------------setThreadName
void ThreadBaseClass::setThreadName(std::string name) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sThreadName = name;
}

// ---------------------------------------------------------getThreadName
const std::string& ThreadBaseClass::getThreadName() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sThreadName);
}

// ---------------------------------------------------------setLastHealthy
void ThreadBaseClass::setLastHealthy(time_t now) {
	m_tLastHealthy = now;
}

// ---------------------------------------------------------getLastHealthy
time_t ThreadBaseClass::getLastHealthy() {
	return ((time_t) m_tLastHealthy);
}
}  // namespace util
}  // namespace glass3
