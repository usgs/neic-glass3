#include <queue.h>
#include <json.h>
#include <logger.h>
#include <memory>
#include <mutex>
#include <string>
#include <queue>

namespace glass3 {
namespace util {

// ---------------------------------------------------------Queue
Queue::Queue() {
	clear();
}

// ---------------------------------------------------------~Queue
Queue::~Queue() {
	clear();
}

// ---------------------------------------------------------clear
void Queue::clear() {
	getMutex().lock();

	// while there are elements in the Queue
	while (!m_DataQueue.empty()) {
		// remove them
		m_DataQueue.pop();
	}

	getMutex().unlock();

	// finally do baseclass clear
	util::BaseClass::clear();
}

// ---------------------------------------------------------addDataToQueue
bool Queue::addDataToQueue(std::shared_ptr<json::Object> data) {
	std::lock_guard < std::mutex > guard(getMutex());

	// add the new data to the Queue
	m_DataQueue.push(data);

	return (true);
}

// ---------------------------------------------------------getDataFromQueue
std::shared_ptr<json::Object> Queue::getDataFromQueue() {
	std::lock_guard < std::mutex > guard(getMutex());

	// return null if the Queue is empty
	if (m_DataQueue.empty() == true) {
		return (NULL);
	}

	// get the next element
	std::shared_ptr < json::Object > data;
	data = m_DataQueue.front();

	// remove that element now that we got it
	m_DataQueue.pop();

	return (data);
}

// ---------------------------------------------------------size
int Queue::size() {
	std::lock_guard < std::mutex > guard(getMutex());

	int queuesize = static_cast<int>(m_DataQueue.size());

	return (queuesize);
}
}  // namespace util
}  // namespace glass3

