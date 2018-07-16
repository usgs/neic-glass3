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
	setRunning(false);
	setThreadHealth(true);
	setStarted(false);
	m_WorkThread = NULL;

	setHealthCheckInterval(1);
	setLastHealthCheck(std::time(nullptr));

	setSleepTime(100);
}

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass(std::string threadname, int sleeptimems)
		: util::BaseClass() {
	setThreadName(threadname);
	setRunning(false);
	setThreadHealth(true);
	setStarted(false);
	m_WorkThread = NULL;

	setHealthCheckInterval(1);
	setLastHealthCheck(std::time(nullptr));

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
	logger::log(
			"trace",
			"ThreadBaseClass::start(): Starting Work Thread. ("
					+ getThreadName() + ")");

	// are we already running
	if (isRunning() == true) {
		logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already running. ("
						+ getThreadName() + ")");
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

	// we've been started
	setStarted(true);

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
	if (isRunning() == false) {
		logger::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not running. ("
						+ getThreadName() + ")");
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

	// we're no longer started
	setStarted(false);

	// tell the thread to stop
	setRunning(false);

	// wait for the thread to finish
	m_WorkThread->join();

	// delete it
	delete (m_WorkThread);
	m_WorkThread = NULL;

	// we're no longer running
	setThreadHealth(false);

	// done
	logger::log(
			"debug",
			"ThreadBaseClass::stop(): Stopped Work Thread. (" + getThreadName()
					+ ")");
	return (true);
}

// ---------------------------------------------------------healthCheck
bool ThreadBaseClass::healthCheck() {
	// don't check if we've not started
	if (isStarted() == false) {
		return (true);
	}

	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (getHealthCheckInterval() < 0) {
		return (true);
	}

	// thread is dead if we're not running
	if (isRunning() == false) {
		logger::log(
				"error",
				"ThreadBaseClass::healthCheck(): m_bRunWorkThread is false. ("
						+ getThreadName() + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - getLastHealthCheck()) >= getHealthCheckInterval()) {
		// if the check is false, the thread is dead
		if (getThreadHealth() == false) {
			logger::log(
					"error",
					"ThreadBaseClass::healthCheck(): m_bThreadHealth is false. ("
							+ getThreadName() + ") after an interval of "
							+ std::to_string(getHealthCheckInterval())
							+ " seconds.");
			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true.
		setThreadHealth(false);

		setLastHealthCheck(tNow);
	}

	// we passed this time
	logger::log(
			"trace",
			"ThreadBaseClass::healthCheck(): Work Thread is still running. ("
					+ getThreadName() + ") after an interval of "
					+ std::to_string(getHealthCheckInterval()) + " seconds.");
	return (true);
}

// ---------------------------------------------------------workLoop
void ThreadBaseClass::workLoop() {
	// we're running
	setRunning(true);

	// run until told to stop
	while (isRunning()) {
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

	// we're no longer running
	setRunning(false);

	// done with thread
	return;
}

// ---------------------------------------------------------setRunning
void ThreadBaseClass::setRunning(bool running) {
	m_bRunWorkThread = running;
}

// ---------------------------------------------------------isRunning
bool ThreadBaseClass::isRunning() {
	return (m_bRunWorkThread);
}

// ---------------------------------------------------------setThreadHealth
void ThreadBaseClass::setThreadHealth(bool health) {
	m_bThreadHealth = health;
}

// ---------------------------------------------------------getThreadHealth
bool ThreadBaseClass::getThreadHealth() {
	return (m_bThreadHealth);
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

// ---------------------------------------------------------setStarted
void ThreadBaseClass::setStarted(bool started) {
	m_bStarted = started;
}

// ---------------------------------------------------------isStarted
bool ThreadBaseClass::isStarted() {
	return (m_bStarted);
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

// ---------------------------------------------------------setLastHealthCheck
void ThreadBaseClass::setLastHealthCheck(time_t now) {
	m_tLastHealthCheck = now;
}

// ---------------------------------------------------------getLastHealthCheck
time_t ThreadBaseClass::getLastHealthCheck() {
	return ((time_t)m_tLastHealthCheck);
}
}  // namespace util
}  // namespace glass3
