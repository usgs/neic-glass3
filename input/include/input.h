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
#include <associatorinterface.h>
#include <gpickparser.h>
#include <jsonparser.h>
#include <ccparser.h>
#include <queue.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

namespace glass3 {
namespace input {

#define GPICK_TYPE "gpick"
#define GPICKS_TYPE "gpicks"
#define JSON_TYPE "json"
#define CC_TYPE "dat"

/**
 * \brief glass input class
 *
 * The glass input class is a thread class encapsulating the data input logic
 * The input class handles reading input data, parsing it, validating  it, and
 * queuing it for later use by the associator class
 *
 * input inherits from the threadbaseclass class.
 * input implements the ioutput interface.
 */
class input
		: public glass3::util::iInput, public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief input advanced constructor
	 *
	 * The advanced constructor for the input class.
	 * Initializes members to default values.
	 *
	 * \param linesleepms - An integer value holding the time to sleep
	 * between reading input lines from a file, in milliseconds
	 */
	input();

	/**
	 * \brief input advanced constructor
	 *
	 * The advanced constructor for the input class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 */
	explicit input(std::shared_ptr<const json::Object> config);

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
	 * The this function configures the input class, and the tracking cache it
	 * contains.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief input clear function
	 *
	 * The clear function for the input class.
	 * Clears all configuration, clears and reallocates the message queue and
	 * cache
	 */
	void clear() override;

	/**
	 * \brief input data getting function
	 *
	 * The function (from iinput) used to get input data from the data queue.
	 *
	 * \return Returns a pointer to a json::Object containing the data.
	 */
	std::shared_ptr<json::Object> getInputData() override;

	/**
	 * \brief input data count function
	 *
	 * The function (from iinput) used to get the count of how much data is in
	 * the data queue.
	 *
	 * \return Returns a integer containing the current data count.
	 */
	int getInputDataCount() override;

	/**
	 * \brief Function to set the name of the default agency id
	 *
	 * This function sets the name of the default agency id, this name is used in
	 * generating input messages
	 *
	 * \param id = A std::string containing the agency id to set
	 */
	void setDefaultAgency(std::string agency);

	/**
	 * \brief Function to retrieve the name of the default agency id
	 *
	 * This function retrieves the name of default agency id, this name is used
	 * in generating input messages
	 *
	 * \return A std::string containing the agency id
	 */
	const std::string getDefaultAgencyId();

	/**
	 * \brief Function to set the name of the default author
	 *
	 * This function sets the name of the default author, this name is used in
	 * generating input messages
	 *
	 * \param author = A std::string containing the author to set
	 */
	void setDefaultAuthor(std::string author);

	/**
	 * \brief Function to retrieve the name of the default author
	 *
	 * This function retrieves the name of the default author, this name is used
	 * in generating input messages
	 *
	 * \return A std::string containing the author
	 */
	const std::string getDefaultAuthor();

	/**
	 * \brief Function to set the maximum queue size
	 *
	 * This function sets the maximum allowable size of the input data queue
	 *
	 * \param delay = An integer value containing the maximum queue size
	 */
	void setQueueMaxSize(int size);

	/**
	 * \brief Function to retrieve tthe maximum queue size
	 *
	 * This function retrieves the maximum allowable size of the input data
	 * queue
	 *
	 * \return Returns an integer value containing the maximum queue size
	 */
	int getQueueMaxSize();

	/**
	 * \brief Function to set the interval to generate informational reports
	 *
	 * This function sets the interval in seconds between logging informational
	 * reports on output throughput and performance
	 *
	 * \param interval = An integer value containing the interval in seconds
	 */
	void setReportInterval(int interval);

	/**
	 * \brief Function to retrieve the interval to generate informational reports
	 *
	 * This function retrieves the interval in seconds between logging
	 * informationalreports on output throughput and performance
	 *
	 * \return Returns an integer value containing the interval in seconds
	 */
	int getReportInterval();

 protected:
	/**
	 * \brief input work function
	 *
	 * The function (from threadclassbase) used to do work.
	 * \return returns true if work was successful, false otherwise.
	 */
	bool work() override;

	/**
	 * \brief parse line function
	 *
	 * The function that parses an input line, based on the given extension
	 *
	 * \param type - A std::string containing the type of data to parse
	 * \return returns a shared pointer to a json::Object containing the parsed
	 * \param input - A std::string containing the input line to parse
	 * data
	 */
	virtual std::shared_ptr<json::Object> parse(std::string type,
												std::string input);

	/**
	 * \brief validate data function
	 *
	 * The function that validates input data, based on the given extension
	 *
	 * \param type - A std::string containing the type of data to validate
	 * \param input - A json::Object containing the data to validate
	 * \return returns true if valid, false otherwise
	 */
	virtual bool validate(std::string type,
							std::shared_ptr<json::Object> input);

	/**
	 * \brief get input data type
	 *
	 * A pure virutal function that determines the input data type
	 *
	 * \param input - A json::Object containing the data to validate
	 * \return returns a std::string containing the input data type
	 */
	virtual std::string getDataType(std::string input) = 0;

	/**
	 * \brief get input data string
	 *
	 * A pure virtual function that retrieves the next data message from an
	 * input source
	 *
	 * \return returns a std::string containing the input data message
	 */
	virtual std::string fetchRawData() = 0;

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
	 * \brief the integer configuration value indicating the maximum size of the
	 * data queue
	 */
	std::atomic<int> m_QueueMaxSize;

	/**
	 * \brief Information Report interval
	 *
	 * An integer containing the interval (in seconds) between
	 * logging informational reports.
	 */
	std::atomic<int> m_iReportInterval;

	/**
	 * \brief the parse performance counter
	 */
	int m_iParseCounter;

	/**
	 * \brief the last time a performance report was generated
	 */
	std::time_t tLastWorkReport;

	/**
	 * \brief the data queue
	 */
	glass3::util::Queue* m_DataQueue;

	/**
	 * \brief the global pick format parsing object
	 */
	glass3::parse::GPickParser * m_GPickParser;

	/**
	 * \brief the json format parsing object
	 */
	glass3::parse::JSONParser * m_JSONParser;

	/**
	 * \brief the cross correlation format parsing object
	 */
	glass3::parse::CCParser * m_CCParser;
};
}  // namespace input
}  // namespace glass3
#endif  // INPUT_H
