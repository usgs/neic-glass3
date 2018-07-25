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

	// set to default inter-loop sleep
	setSleepTime(100);
}

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass(std::string threadName, int sleepTimeMS)
		: util::BaseClass() {
	setThreadName(threadName);
	setThreadState(glass3::util::ThreadState::Initialized);
	m_WorkThread = NULL;

	setHealthCheckInterval(1);
	setThreadHealth();

	// set to provided inter-loop sleep
	setSleepTime(sleepTimeMS);
}

// ---------------------------------------------------------~ThreadBaseClass
ThreadBaseClass::~ThreadBaseClass() {
	glass3::util::log(
			"debug",
			"ThreadBaseClass::~ThreadBaseClass(): Destruction. ("
					+ getThreadName() + ")");

	stop();
}

// ---------------------------------------------------------start
bool ThreadBaseClass::start() {
	// are we already running
	if ((getThreadState() != glass3::util::ThreadState::Initialized)
			&& (getThreadState() != glass3::util::ThreadState::Stopped)) {
		glass3::util::log("warning",
					"ThreadBaseClass::start(): Work Thread is already starting "
							"or running. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread != NULL) {
		glass3::util::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already allocated. ("
						+ getThreadName() + ")");
		return (false);
	}

	// we're starting
	setThreadState(glass3::util::ThreadState::Starting);

	// start the thread
	m_WorkThread = new std::thread(&ThreadBaseClass::workLoop, this);

	glass3::util::log(
			"debug",
			"ThreadBaseClass::start(): Starting Work Thread. ("
					+ getThreadName() + ")");
	return (true);
}

// ---------------------------------------------------------stop
bool ThreadBaseClass::stop() {
	// check if we're running
	if (getThreadState() != glass3::util::ThreadState::Started) {
		glass3::util::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not running, "
						"or is already stopping. (" + getThreadName() + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread == NULL) {
		glass3::util::log(
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

	// done
	glass3::util::log(
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
		glass3::util::log("warning",
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
		glass3::util::log(
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
		glass3::util::log(
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
	glass3::util::log(
			"debug",
			"ThreadBaseClass::workLoop():  Work Thread Startup. ("
					+ getThreadName() + ")");

	// we're running
	setThreadState(glass3::util::ThreadState::Started);

	// run until told to stop
	while (true) {
		// signal that we're still running
		setThreadHealth();

		try {
			// do our work
			if (work() == false) {
				// something has gone wrong
				// break out of the loop
				glass3::util::log(
						"error",
						"ThreadBaseClass::workLoop(): Work returned false, "
								"something's wrong, stopping thread. ("
								+ getThreadName() + ")");
				break;
			}
		} catch (const std::exception &e) {
			glass3::util::log(
					"error",
					"ThreadBaseClass::workLoop: Exception during work(): "
							+ std::string(e.what())
							+ "something's wrong, stopping thread. ("
							+ getThreadName() + ")");
			break;
		}

		if (getThreadState() != glass3::util::ThreadState::Started) {
			glass3::util::log(
					"info",
					"ThreadBaseClass::workLoop(): Non-Starting thread "
							"status detected ("
							+ std::to_string(getThreadState())
							+ "), stopping thread. (" + getThreadName() + ")");
			break;
		}

		// give up some time at the end of the loop
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));
	}

	glass3::util::log(
			"info",
			"ThreadBaseClass::workLoop(): Stopped thread. (" + getThreadName()
					+ ")");

	setThreadState(glass3::util::ThreadState::Stopped);

	// done with thread
	return;
}

// ---------------------------------------------------------setThreadState
void ThreadBaseClass::setThreadState(glass3::util::ThreadState state) {
	m_ThreadState = state;
}

// ---------------------------------------------------------getThreadState
glass3::util::ThreadState ThreadBaseClass::getThreadState() {
	return (m_ThreadState);
}

// ---------------------------------------------------------setSleepTime
void ThreadBaseClass::setSleepTime(int sleepTimeMS) {
	m_iSleepTimeMS = sleepTimeMS;
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
void ThreadBaseClass::setThreadName(std::string threadName) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sThreadName = threadName;
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
