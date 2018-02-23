/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef QUEUE_H
#define QUEUE_H

#include <json.h>

#include <memory>
#include <mutex>
#include <string>
#include <queue>

namespace util {
/**
 * \brief util queue class
 *
 * The util cache queue is a class implementing a queue of
 * pointers to json::Objects.  The queue is thread safe.
 */
class Queue {
 public:
	/**
	 * \brief queue constructor
	 *
	 * The constructor for the queue class.
	 */
	Queue();

	/**
	 * \brief queue destructor
	 *
	 * The destructor for the queue class.
	 */
	~Queue();

	/**
	 *\brief add data to queue
	 *
	 * Add the provided data the queue
	 * \param data - A pointer to a json::Object to add to the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \return returns true if successful, false otherwise.
	 */
	bool addDataToQueue(std::shared_ptr<json::Object> data, bool lock = true);

	/**
	 *\brief get data from queue
	 *
	 * Get the next data from the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \param copy - A boolean value indicating whether to return a pointer to
	 * a copy of the
	 * data. Defaults to false
	 * \return returns a pointer to the json::Object containing the data, NULL
	 * there was no
	 * data in the queue
	 */
	std::shared_ptr<json::Object> getDataFromQueue(bool lock = true);

	/**
	 *\brief clear data from queue
	 *
	 * Clear all data from the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 */
	void clearQueue(bool lock = true);

	/**
	 *\brief get the size of the queue
	 *
	 * Get the current size of the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \return returns an integer value containing the current size of the
	 * queue
	 */
	int size(bool lock = true);

 private:
	/**
	 * \brief the std::queue used to store the queue
	 */
	std::queue<std::shared_ptr<json::Object>> m_DataQueue;

	/**
	 * \brief the mutex for the queue
	 */
	std::mutex m_QueueMutex;
};
}  // namespace util
#endif  // QUEUE_H

