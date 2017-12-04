/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef INPUT_H
#define INPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <inputinterface.h>
#include <jsonparser.h>
#include <queue.h>
#include <Consumer.h>

#include <vector>
#include <queue>
#include <mutex>
#include <string>

namespace glass {
/**
 * \brief glass input class
 *
 * The glass input class is a thread class encapsulating the data input logic
 * The input class handles reading input data from disk, parsing it, validating
 * it, and queuing it for later use by the associator class
 *
 * input inherits from the threadbaseclass class.
 * input implements the ioutput interface.
 */
class input : public util::iInput, public util::ThreadBaseClass {
 public:
	/**
	 * \brief input constructor
	 *
	 * The constructor for the input class.
	 * Initializes members to default values.
	 */
	input();

	/**
	 * \brief input advanced constructor
	 *
	 * The advanced constructor for the input class.
	 * Initializes members to default values.
	 *
	 * \param linesleepms - An integer value holding the time to sleep
	 * between reading input lines from a file, in milliseconds
	 */
	explicit input(int linesleepms);

	/**
	 * \brief input advanced constructor
	 *
	 * The advanced constructor for the input class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 * \param linesleepms - An integer value holding the time to sleep
	 * between reading input lines from a file, in milliseconds
	 */
	input(json::Object *config, int linesleepms);

	/**
	 * \brief input destructor
	 *
	 * The destructor for the input class.
	 * Stops the work thread
	 */
	~input();

	/**
	 * \brief input configuration function
	 *
	 * The this function configures the input class, and allocates the parsing
	 * objects and data queue.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(json::Object *config) override;

	/**
	 * \brief output clear function
	 *
	 * The clear function for the output class.
	 * Clears all configuration, clears and reallocates the data queue and
	 * parsing objects
	 */
	void clear() override;

	/**
	 * \brief input data getting function
	 *
	 * The function (from iinput) used to get input data from the data queue.
	 *
	 * \return Returns a pointer to a json::Object containing the data.
	 */
	json::Object* getData() override;

	/**
	 * \brief input data count function
	 *
	 * The function (from iinput) used to get the count of how much data is in
	 * the data queue.
	 *
	 * \return Returns a integer containing the current data count.
	 */
	int dataCount() override;

	/**
	 * \brief getter for the m_QueueMaxSize configuration variable
	 */
	int getQueueMaxSize() {
		m_ConfigMutex.lock();
		int maxsize = m_QueueMaxSize;
		m_ConfigMutex.unlock();

		return (maxsize);
	}

	/**
	 * \brief getter for the m_sDefaultAgencyID configuration variable
	 */
	const std::string getSDefaultAgencyId() {
		m_ConfigMutex.lock();
		std::string agencyid = m_sDefaultAgencyID;
		m_ConfigMutex.unlock();

		return (agencyid);
	}

	/**
	 * \brief getter for the m_sDefaultAuthor configuration variable
	 */
	const std::string getSDefaultAuthor() {
		m_ConfigMutex.lock();
		std::string author = m_sDefaultAuthor;
		m_ConfigMutex.unlock();

		return (author);
	}

 protected:
	/**
	 * \brief input work function
	 *
	 * The function (from threadclassbase) used to do work.
	 * \return returns true if work was successful, false otherwise.
	 */
	bool work() override;

	/**
	 * \brief the function for consumer logging
	 * \param message - A string containing the logging message
	 */
	void logConsumer(const std::string &message);

 private:
	/**
	 * \brief the std::string configuration value indicating the default agency
	 * id
	 */
	std::string m_sDefaultAgencyID;

	/**
	 * \brief the std::string configuration value indicating the default author
	 */
	std::string m_sDefaultAuthor;

	/**
	 * \brief the list of topic strings used by the consumer
	 */
	std::vector<std::string> m_sTopicList;

	/**
	 * \brief the integer configuration value indicating the maximum size of the
	 * data queue
	 */
	int m_QueueMaxSize;

	/**
	 * \brief the mutex for configuration
	 */
	std::mutex m_ConfigMutex;

	/**
	 * \brief the data queue
	 */
	util::Queue* m_DataQueue;

	/**
	 * \brief the json format parsing object
	 */
	parse::JSONParser * m_JSONParser;

	/**
	 * \brief the consumer object to get messages from
	 */
	hazdevbroker::Consumer * m_Consumer;
};
}  // namespace glass
#endif  // INPUT_H
