#include <threadbaseclass.h>
#include <logger.h>
#include <thread>
#include <string>

namespace util {
// construction / destruction
ThreadBaseClass::ThreadBaseClass()
		: util::BaseClass() {
	logger::log("debug", "ThreadBaseClass::ThreadBaseClass(): Construction.");

	m_sThreadName = "NYI";
	m_iSleepTimeMS = 100;
	m_bRunWorkThread = false;
	m_bCheckWorkThread = true;
	m_bStarted = false;
	m_WorkThread = NULL;

	m_iCheckInterval = 1;
	std::time(&tLastCheck);
}

ThreadBaseClass::ThreadBaseClass(std::string threadname, int sleeptimems)
		: util::BaseClass() {
	m_sThreadName = threadname;
	m_bRunWorkThread = false;
	m_bCheckWorkThread = true;
	m_bStarted = false;
	m_WorkThread = NULL;

	m_iCheckInterval = 1;
	std::time(&tLastCheck);

	setSleepTime(sleeptimems);
}

ThreadBaseClass::~ThreadBaseClass() {
	logger::log(
			"debug",
			"ThreadBaseClass::~ThreadBaseClass(): Destruction. ("
					+ m_sThreadName + ")");

	stop();
}

// threading
bool ThreadBaseClass::start() {
	logger::log(
			"trace",
			"ThreadBaseClass::start(): Starting Work Thread. (" + m_sThreadName
					+ ")");

	// are we already running
	if (m_bRunWorkThread == true) {
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

	m_bStarted = true;

	// start the thread
	m_WorkThread = new std::thread(&ThreadBaseClass::workLoop, this);

	logger::log(
			"debug",
			"ThreadBaseClass::start(): Started Work Thread. (" + m_sThreadName
					+ ")");
	return (true);
}

bool ThreadBaseClass::stop() {
	logger::log(
			"trace",
			"ThreadBaseClass::stop(): Stopping Work Thread. (" + m_sThreadName
					+ ")");

	// check if we're running
	if (m_bRunWorkThread == false) {
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

	m_bStarted = false;

	// tell the thread to stop
	m_bRunWorkThread = false;

	// wait for the thread to finish
	m_WorkThread->join();

	// delete it
	delete (m_WorkThread);
	m_WorkThread = NULL;

	// we're no longer running
	m_bCheckWorkThread = false;

	// done
	logger::log(
			"debug",
			"ThreadBaseClass::stop(): Stopped Work Thread. (" + m_sThreadName
					+ ")");
	return (true);
}

bool ThreadBaseClass::check() {
	logger::log(
			"trace",
			"ThreadBaseClass::check(): Checking to see if thread is working. ("
					+ m_sThreadName + ")");

	// don't check if we've not started
	if (m_bStarted == false)
		return (true);

	// if we have a negative check interval,
	// we shouldn't worry about thread health checks.
	if (m_iCheckInterval < 0)
		return (true);

	// thread is dead if we're not running
	if (m_bRunWorkThread == false) {
		logger::log(
				"error",
				"ThreadBaseClass::check(): m_bRunWorkThread is false. ("
						+ m_sThreadName + ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - tLastCheck) >= m_iCheckInterval) {
		// lock the mutex to make sure we
		// don't run into a threading issue
		// this *may* be excessive
		m_CheckMutex.lock();

		// if the check is false, the thread is dead
		if (m_bCheckWorkThread == false) {
			m_CheckMutex.unlock();
			logger::log(
					"error",
					"ThreadBaseClass::check(): m_bCheckWorkThread is false. ("
							+ m_sThreadName + ") after an interval of "
							+ std::to_string(m_iCheckInterval) + " seconds.");
			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true.
		m_bCheckWorkThread = false;
		m_CheckMutex.unlock();

		tLastCheck = tNow;
	}

	// we passed this time
	logger::log(
			"trace",
			"ThreadBaseClass::check(): Work Thread is still running. ("
					+ m_sThreadName + ") after an interval of "
					+ std::to_string(m_iCheckInterval) + " seconds.");
	return (true);
}

void ThreadBaseClass::setSleepTime(int sleeptimems) {
	m_iSleepTimeMS = sleeptimems;
}

int ThreadBaseClass::getSleepTime() {
	return (m_iSleepTimeMS);
}

void ThreadBaseClass::workLoop() {
	// we're running
	m_bRunWorkThread = true;

	// run until told to stop
	while (m_bRunWorkThread) {
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
		setWorkCheck();

		// give up some time at the end of the loop
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}

	logger::log(
			"info",
			"ThreadBaseClass::workLoop(): Stopped thread. (" + m_sThreadName
					+ ")");

	// we're no longer running
	m_bRunWorkThread = false;

	// done with thread
	return;
}

bool ThreadBaseClass::isRunning() {
	return (m_bRunWorkThread);
}

void ThreadBaseClass::setWorkCheck() {
	// signal that we're still running
	// mutex here *may* be excessive
	m_CheckMutex.lock();
	m_bCheckWorkThread = true;
	m_CheckMutex.unlock();
}
}  // namespace util

