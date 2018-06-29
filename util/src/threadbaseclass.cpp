#include <threadbaseclass.h>
#include <logger.h>
#include <thread>
#include <string>
#include <ctime>

namespace util {

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass()
		: util::BaseClass() {
	setThreadName("NYI");
	setRunning(false);
	setCheckWorkThread(true);
	setStarted(false);
	m_WorkThread = NULL;

	setCheckInterval(1);
	setLastCheck(std::time(nullptr));

	setSleepTime(100);
}

// ---------------------------------------------------------ThreadBaseClass
ThreadBaseClass::ThreadBaseClass(std::string threadname, int sleeptimems)
		: util::BaseClass() {
	setThreadName(threadname);
	setRunning(false);
	setCheckWorkThread(true);
	setStarted(false);
	m_WorkThread = NULL;

	setCheckInterval(1);
	setLastCheck(std::time(nullptr));

	setSleepTime(sleeptimems);
}

// ---------------------------------------------------------~ThreadBaseClass
ThreadBaseClass::~ThreadBaseClass() {
	logger::log(
			"debug",
			"ThreadBaseClass::~ThreadBaseClass(): Destruction. ("
					+ m_sThreadName + ")");

	stop();
}

// ---------------------------------------------------------start
bool ThreadBaseClass::start() {
	logger::log(
			"trace",
			"ThreadBaseClass::start(): Starting Work Thread. (" + m_sThreadName
					+ ")");

	// are we already running
	if (isRunning() == true) {
		logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already running. ("
						+ m_sThreadName + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread != NULL) {
		logger::log(
				"warning",
				"ThreadBaseClass::start(): Work Thread is already allocated. ("
						+ m_sThreadName + ")");
		return (false);
	}

	// we've been started
	setStarted(true);

	// start the thread
	m_WorkThread = new std::thread(&ThreadBaseClass::workLoop, this);

	logger::log(
			"debug",
			"ThreadBaseClass::start(): Started Work Thread. (" + m_sThreadName
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
						+ m_sThreadName + ")");
		return (false);
	}

	// nullcheck
	if (m_WorkThread == NULL) {
		logger::log(
				"warning",
				"ThreadBaseClass::stop(): Work Thread is not allocated. ("
						+ m_sThreadName + ")");
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
	setCheckWorkThread(false);

	// done
	logger::log(
			"debug",
			"ThreadBaseClass::stop(): Stopped Work Thread. (" + m_sThreadName
					+ ")");
	return (true);
}

// ---------------------------------------------------------check
bool ThreadBaseClass::check() {
	// don't check if we've not started
	if (isStarted() == false) {
		return (true);
	}

	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (getCheckInterval() < 0) {
		return (true);
	}

	// thread is dead if we're not running
	if (isRunning() == false) {
		logger::log(
				"error",
				"ThreadBaseClass::check(): m_bRunWorkThread is false. ("
						+ m_sThreadName + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - getLastCheck()) >= getCheckInterval()) {
		// if the check is false, the thread is dead
		if (getCheckWorkThread() == false) {
			logger::log(
					"error",
					"ThreadBaseClass::check(): m_bCheckWorkThread is false. ("
							+ m_sThreadName + ") after an interval of "
							+ std::to_string(getCheckInterval()) + " seconds.");
			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true.
		setCheckWorkThread(false);

		setLastCheck(tNow);
	}

	// we passed this time
	logger::log(
			"trace",
			"ThreadBaseClass::check(): Work Thread is still running. ("
					+ m_sThreadName + ") after an interval of "
					+ std::to_string(getCheckInterval()) + " seconds.");
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
								+ m_sThreadName + ")");
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
		setCheckWorkThread();

		// give up some time at the end of the loop
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}

	logger::log(
			"info",
			"ThreadBaseClass::workLoop(): Stopped thread. (" + m_sThreadName
					+ ")");

	// we're no longer running
	setRunning(false);

	// done with thread
	return;
}

// ---------------------------------------------------------setRunning
void ThreadBaseClass::setRunning(bool running) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_bRunWorkThread = running;
}

// ---------------------------------------------------------isRunning
bool ThreadBaseClass::isRunning() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_bRunWorkThread);
}

// ---------------------------------------------------------setCheckWorkThread
void ThreadBaseClass::setCheckWorkThread(bool check) {
	// signal that we're still running
	// mutex here *may* be excessive
	std::lock_guard<std::mutex> guard(m_CheckMutex);
	m_bCheckWorkThread = check;
}

// ---------------------------------------------------------getCheckWorkThread
bool ThreadBaseClass::getCheckWorkThread() {
	std::lock_guard<std::mutex> guard(m_CheckMutex);
	return (m_bCheckWorkThread);
}

// ---------------------------------------------------------setSleepTime
void ThreadBaseClass::setSleepTime(int sleeptimems) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_iSleepTimeMS = sleeptimems;
}

// ---------------------------------------------------------getSleepTime
int ThreadBaseClass::getSleepTime() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_iSleepTimeMS);
}

// ---------------------------------------------------------setCheckInterval
void ThreadBaseClass::setCheckInterval(int interval) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_iCheckInterval = interval;
}

// ---------------------------------------------------------getCheckInterval
int ThreadBaseClass::getCheckInterval() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_iCheckInterval);
}

// ---------------------------------------------------------setStarted
void ThreadBaseClass::setStarted(bool started) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_bStarted = started;
}

// ---------------------------------------------------------isStarted
bool ThreadBaseClass::isStarted() {
	std::lock_guard<std::mutex> guard(getMutex());
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

// ---------------------------------------------------------setLastCheck
void ThreadBaseClass::setLastCheck(time_t now) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_tLastCheck = now;
}

// ---------------------------------------------------------getLastCheck
time_t ThreadBaseClass::getLastCheck() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_tLastCheck);
}
}  // namespace util

