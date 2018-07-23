/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BROKERINPUT_H
#define BROKERINPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <input.h>
#include <Consumer.h>

#include <vector>
#include <queue>
#include <mutex>
#include <string>
#include <memory>

namespace glass {
/**
 * \brief glass brokerInput class
 *
 * The glass brokerInput class is a thread class encapsulating the data brokerInput logic
 * The brokerInput class handles reading brokerInput data from disk, parsing it, validating
 * it, and queuing it for later use by the associator class
 *
 * brokerInput inherits from the glass3::brokerInput class.
 * brokerInput implements the ibrokerInput interface.
 */
class brokerInput : public glass3::input::input {
 public:
	/**
	 * \brief brokerInput constructor
	 *
	 * The constructor for the brokerInput class.
	 * Initializes members to default values.
	 */
	brokerInput();

	/**
	 * \brief brokerInput advanced constructor
	 *
	 * The advanced constructor for the brokerInput class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 */
	brokerInput(std::shared_ptr<json::Object> &config);

	/**
	 * \brief brokerInput destructor
	 *
	 * The destructor for the brokerInput class.
	 * Stops the work thread
	 */
	~brokerInput();

	/**
	 * \brief brokerInput configuration function
	 *
	 * The this function configures the brokerInput class, and allocates the parsing
	 * objects and data queue.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<json::Object> config) override;

	/**
	 * \brief output clear function
	 *
	 * The clear function for the output class.
	 * Clears all configuration, clears and reallocates the data queue and
	 * parsing objects
	 */
	void clear() override;

 protected:
	/**
	 * \brief get input data type
	 *
	 * A function (overridden from glass3::input) that determines the input data
	 * type
	 *
	 * \param input - A json::Object containing the data to validate
	 * \return returns a std::string containing the input data type
	 */
	virtual std::string getDataType(std::string input) override;

	/**
	 * \brief get input data string
	 *
	 * A function (overridden from glass3::input)that retrieves the next data
	 * message (line) from an input file on disk
	 *
	 * \return returns a std::string containing the input data message
	 */
	virtual std::string fetchRawData() override;

	/**
	 * \brief the function for consumer logging
	 * \param message - A string containing the logging message
	 */
	void logConsumer(const std::string &message);

 private:
	/**
	 * \brief the consumer object to get messages from
	 */
	hazdevbroker::Consumer * m_Consumer;
};
}  // namespace glass
#endif  // BROKERINPUT_H
