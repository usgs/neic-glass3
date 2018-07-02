/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef QUEUE_H
#define QUEUE_H

#include <baseclass.h>
#include <json.h>

#include <memory>
#include <mutex>
#include <string>
#include <queue>

namespace glass3 {
namespace util {
/**
 * \brief util queue class
 *
 * The util queue is a class implementing a FIFO queue of
 * shared_ptr's to json::Objects.  The queue is thread safe.
 *
 * queue inherits from the baseclass class.
 */
class Queue : public util::BaseClass {
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
	 * \brief queue clear function
	 *
	 * The clear function for the queue class.
	 * Clear all data currently contained in the queue
	 */
	void clear() override;

	/**
	 *\brief add data to queue
	 *
	 * Add the provided data the queue
	 * \param data - A pointer to a json::Object to add to the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \return returns true if successful, false otherwise.
	 */
	bool addDataToQueue(std::shared_ptr<json::Object> data);

	/**
	 *\brief get data from queue
	 *
	 * Get the next data from the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \return returns a pointer to the json::Object containing the data, NULL
	 * there was no
	 * data in the queue
	 */
	std::shared_ptr<json::Object> getDataFromQueue();

	/**
	 *\brief get the size of the queue
	 *
	 * Get the current size of the queue
	 * \param lock - A boolean value indicating whether to lock the mutex.
	 * Defaults to true
	 * \return returns an integer value containing the current size of the
	 * queue
	 */
	int size();

 private:
	/**
	 * \brief the std::queue used to store the queue
	 */
	std::queue<std::shared_ptr<json::Object>> m_DataQueue;
};
}  // namespace util
}  // namespace glass3
#endif  // QUEUE_H

