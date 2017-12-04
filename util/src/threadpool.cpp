#include <threadpool.h>
#include <logger.h>
#include <map>
#include <string>

namespace util {

ThreadPool::ThreadPool(std::string poolname, int num_threads, int sleeptime,
						int checkinterval) {
	logger::log("debug",
				"ThreadPool::ThreadPool(): Construction. (" + poolname + ")");

	// init varibles
	m_sPoolName = poolname;
	m_bRunJobLoop = true;
	m_iNumThreads = num_threads;
	m_iSleepTimeMS = sleeptime;
	m_iCheckInterval = checkinterval;
	std::time(&tLastCheck);

	// create threads in pool
	for (int i = 0; i < m_iNumThreads; i++) {
		// create thread
		m_ThreadPool.push_back(std::thread(&ThreadPool::jobLoop, this));

		// add to status map if we're tracking status
		if (m_iCheckInterval > 0) {
			m_StatusMutex.lock();
			m_ThreadStatusMap[m_ThreadPool[i].get_id()] = true;
			m_StatusMutex.unlock();
		}

		logger::log(
				"debug",
				"ThreadPool::ThreadPool(): Created Thread #" + std::to_string(i)
						+ " (" + m_sPoolName + ")");
	}
}

ThreadPool::~ThreadPool() {
	logger::log("debug",
				"ThreadPool::~ThreadPool(): Destruction.(" + m_sPoolName + ")");

	// stop the threads.
	stop();
}

bool ThreadPool::stop() {
	logger::log(
			"debug",
			"ThreadPool::ThreadPool(): Construction. (" + m_sPoolName + ")");

	// disable status checking
	m_StatusMutex.lock();
	m_ThreadStatusMap.clear();
	m_StatusMutex.unlock();

	// signal threads to finish
	m_bRunJobLoop = false;

	// wait for threads to finish
	for (int i = 0; i < m_ThreadPool.size(); i++) {
		m_ThreadPool[i].join();
	}

	return (true);
}

void ThreadPool::setStatus(bool status) {
	// update thread status
	m_StatusMutex.lock();
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
	m_StatusMutex.unlock();
}

void ThreadPool::addJob(std::function<void()> newjob) {
	// add the job to the queue
	m_QueueMutex.lock();
	m_JobQueue.push(newjob);
	m_QueueMutex.unlock();

	logger::log("debug",
				"ThreadPool::addJob(): Added Job.(" + m_sPoolName + ")");
}

void ThreadPool::jobLoop() {
	logger::log("debug",
				"ThreadPool::jobLoop(): Thread Start.(" + m_sPoolName + ")");

	while (m_bRunJobLoop == true) {
		// make sure we're still running
		if (m_bRunJobLoop == false)
			break;

		// update thread status
		setStatus(true);

		// lock for queue access
		m_QueueMutex.lock();

		// are there any jobs
		if (m_JobQueue.empty() == true) {
			// unlock and skip until next time
			m_QueueMutex.unlock();

			// give up some time at the end of the loop
			jobSleep();

			// on to the next loop
			continue;
		}

		// get the next job
		std::function<void()> newjob = m_JobQueue.front();
		m_JobQueue.pop();

		// done with queue
		m_QueueMutex.unlock();

		logger::log("debug",
					"ThreadPool::jobLoop(): Found Job.(" + m_sPoolName + ")");

		// run the job
		try {
			newjob();
		} catch (const std::exception &e) {
			logger::log(
					"error",
					"ThreadPool::jobLoop: Exception during job(): "
							+ std::string(e.what()) + " (" + m_sPoolName + ")");
			break;
		}

		logger::log(
				"debug",
				"ThreadPool::jobLoop(): Finished Job.(" + m_sPoolName + ")");

		// give up some time at the end of the loop
		jobSleep();
	}

	// one less thread, don't bother to remove from status checking
	// we want to know this thread exited
	m_iNumThreads--;

	logger::log("debug",
				"ThreadPool::jobLoop(): Thread Exit.(" + m_sPoolName + ")");
}

void ThreadPool::jobSleep() {
	if (m_bRunJobLoop == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}
}

bool ThreadPool::check() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (m_iCheckInterval < 0)
		return (true);

	// thread is dead if we're not running
	if (m_bRunJobLoop == false) {
		logger::log(
				"error",
				"ThreadPool::check(): m_bRunJobLoop is false. (" + m_sPoolName
						+ ")");
		return (false);
	}

	// are there any threads? Not sure how this would happen,
	// but it's worth asking
	if (m_iNumThreads == 0) {
		logger::log(
				"error",
				"ThreadPool::check(): all threads have exited! (" + m_sPoolName
						+ ")");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - tLastCheck) >= m_iCheckInterval) {
		// get the thread status
		m_StatusMutex.lock();
		std::map<std::thread::id, bool>::iterator StatusItr;
		for (StatusItr = m_ThreadStatusMap.begin();
				StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
			// get the thread status
			std::thread::id threadid = StatusItr->first;
			bool status = static_cast<bool>(StatusItr->second);

			// at least one thread did not respond
			if (status != true) {
				m_StatusMutex.unlock();

				logger::log(
						"error",
						"ThreadPool::check(): At least one thread"
								" did not respond (" + m_sPoolName
								+ ") in the last"
								+ std::to_string(m_iCheckInterval)
								+ "seconds.");

				return (false);
			}

			// mark check as false until next time
			// if the thread is alive, it'll mark it
			// as true again.
			StatusItr->second = false;
		}
		m_StatusMutex.unlock();

		// remember the last time we checked
		tLastCheck = tNow;
	}

	// everything is awesome
	return (true);
}
}  // namespace util
