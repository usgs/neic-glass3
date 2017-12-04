#include <queue.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <timeutil.h>
#include <mutex>
#include <string>
#include <queue>

namespace util {

Queue::Queue() {
	logger::log("debug", "Queue::Queue(): Construction.");

	clearQueue();
}

Queue::~Queue() {
	logger::log("debug", "Queue::~Queue(): Destruction.");

	clearQueue();
}

bool Queue::addDataToQueue(json::Object * data, bool lock) {
	if (lock) {
		m_QueueMutex.lock();
	}

	// add the new data to the Queue
	m_DataQueue.push(data);

	if (lock) {
		m_QueueMutex.unlock();
	}

	return (true);
}

json::Object * Queue::getDataFromQueue(bool lock, bool copy) {
	if (lock) {
		m_QueueMutex.lock();
	}

	// return null if the Queue is empty
	if (m_DataQueue.empty() == true) {
		if (lock)
			m_QueueMutex.unlock();

		return (NULL);
	}

	// get the next element
	json::Object *data;
	if (copy) {
		data = new json::Object(*m_DataQueue.front());
	} else {
		data = m_DataQueue.front();
	}

	// remove that element now that we got it
	m_DataQueue.pop();

	if (lock) {
		m_QueueMutex.unlock();
	}

	return (data);
}

void Queue::clearQueue(bool lock) {
	if (lock) {
		m_QueueMutex.lock();
	}

	// while there are elements in the Queue
	while (!m_DataQueue.empty()) {
		// remove them
		m_DataQueue.pop();
	}

	if (lock) {
		m_QueueMutex.unlock();
	}

	return;
}

int Queue::size(bool lock) {
	if (lock) {
		m_QueueMutex.lock();
	}

	int queuesize = static_cast<int>(m_DataQueue.size());

	if (lock) {
		m_QueueMutex.unlock();
	}

	return (queuesize);
}
}  // namespace util
