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
 * \brief glass3::util::Queue class
 *
 * The glass3::util::Queue is a class implementing a FIFO Queue of
 * shared_ptr's to json::Objects.  The Queue is thread safe.
 *
 * Queue inherits from the baseclass class.
 */
class Queue : public util::BaseClass {
 public:
	/**
	 * \brief Queue constructor
	 *
	 * The constructor for the Queue class.
	 */
	Queue();

	/**
	 * \brief Queue destructor
	 *
	 * The destructor for the Queue class.
	 */
	~Queue();

	/**
	 * \brief Queue clear function
	 *
	 * The clear function for the Queue class.
	 * Clear all data currently contained in the Queue
	 */
	void clear() override;

	/**
	 *\brief add data to Queue
	 *
	 * Add the provided data the Queue
	 * \param data - A pointer to a json::Object to add to the Queue
	 * \return returns true if successful, false otherwise.
	 */
	bool addDataToQueue(std::shared_ptr<json::Object> data);

	/**
	 *\brief get data from Queue
	 *
	 * Get the next data from the Queue
	 * \return returns a pointer to the json::Object containing the data, NULL
	 * there was no
	 * data in the Queue
	 */
	std::shared_ptr<json::Object> getDataFromQueue();

	/**
	 *\brief get the size of the Queue
	 *
	 * Get the current size of the Queue
	 * \return returns an integer value containing the current size of the
	 * Queue
	 */
	int size();

 private:
	/**
	 * \brief the std::Queue used to store the Queue
	 */
	std::queue<std::shared_ptr<json::Object>> m_DataQueue;
};
}  // namespace util
}  // namespace glass3
#endif  // QUEUE_H

