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
#include <simplepickparser.h>
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

/**
 * \namespace glass3::input
 * \brief The neic-glass3 project namespace containing input class and functions
 *
 * The neic-glass3 input namespace contains the class, and functions used
 * by other components of neic-glass3 to input data.
 */
namespace input {

#define GPICK_TYPE "gpick"
#define JSON_TYPE "json"
#define CC_TYPE "dat"
#define SIMPLE_TYPE "txt"


/**
 * \brief neic-glass3 Input class
 *
 * The neic-glass3 Input class is a thread class encapsulating the data Input
 * logic for neic-glass3. The Input class handles reading Input data, parsing it,
 * validating it, and queuing it for later use byglasscore via the Associator
 * class. If the internal queue isfull, the class will pause reading Input data
 * until space is available.
 *
 * The input class generates detection format json messages as defined in
 * https://github.com/usgs/earthquake-detection-formats/tree/master/format-docs
 * that are then passed to the associator/glasscore via a queue.
 *
 * The Input class is intended to be inherited from to define application
 * specific input mechanisms (i.e. input from disk files).
 *
 * Input inherits from the glass3::util::ThreadBaseClass class.
 * Input implements the glass3::util::iInput interface.
 */
class Input : public glass3::util::iInput,
	public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief Input default constructor
	 *
	 * The default constructor for the Input class.
	 * Initializes members to default values.
	 */
	Input();

	/**
	 * \brief Input advanced constructor
	 *
	 * The advanced constructor for the Input class. This function calls setup
	 * to configure the class, initializing members to the configured values,
	 * and starts the work thread
	 *
	 * \param config - A json::Object shared_ptr to the configuration to use
	 */
	explicit Input(std::shared_ptr<const json::Object> config);

	/**
	 * \brief Input destructor
	 *
	 * The destructor for the Input class.
	 * Stops the work thread
	 */
	~Input();

	/**
	 * \brief Input configuration function
	 *
	 * The this function configures the Input class,
	 * The setup() function can be called multiple times, in order to reload or
	 * update configuration information.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief Input clear function
	 *
	 * The clear function for the Input class.
	 * Clears all configuration, clears and reallocates the message queue and
	 * cache
	 */
	void clear() override;

	/**
	 * \brief Input data getting function
	 *
	 * The function (from iinput) used to get Input data from the data queue.
	 *
	 * \return Returns a pointer to a json::Object containing the data, or NULL
	 * if the Input queue is empty
	 */
	std::shared_ptr<json::Object> getInputData() override;

	/**
	 * \brief Input data count function
	 *
	 * The function (from iInput) used to get the count of how much data is in
	 * the data queue.
	 *
	 * \return Returns a postitve integer containing the count of data currently
	 * in the Input queue
	 */
	int getInputDataCount() override;

	/**
	 * \brief Function to set the maximum queue size
	 *
	 * This function sets the maximum allowable size of the Input data queue.
	 * Setting this value to -1 indicates that there is no maximum size
	 * to the Input queue
	 *
	 * \param size = An integer value containing the maximum queue size
	 */
	void setInputDataMaxSize(int size);

	/**
	 * \brief Function to retrieve the maximum queue size
	 *
	 * This function retrieves the maximum allowable size of the Input data
	 * queue
	 *
	 * \return Returns an integer value containing the maximum queue size
	 */
	int getInputDataMaxSize();

 protected:
	/**
	 * \brief Input work function
	 *
	 * The function (from threadclassbase) used to do work.
	 * \return returns true if work was successful, false otherwise.
	 */
	glass3::util::WorkState work() override;

	/**
	 * \brief parse line function
	 *
	 * The function that parses an Input line, based on the given extension
	 *
	 * \param inputType - A std::string containing the type of data to parse
	 * \return returns a shared pointer to a json::Object containing the parsed
	 * \param inputMessage - A std::string containing the input message line to
	 * parse
	 */
	virtual std::shared_ptr<json::Object> parse(std::string inputType,
												std::string inputMessage);

	/**
	 * \brief get Input data string and type
	 *
	 * A pure virtual function that retrieves the next data message and type
	 * from an Input source
	 *
	 * \param pOutType - A pointer to a std::string used to pass out the type of
	 * the data
	 * \return returns a std::string containing the Input data message
	 */
	virtual std::string fetchRawData(std::string* pOutType) = 0;

 private:
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

	/**
	 * \brief the simple pick format parsing object
	 */
	glass3::parse::SimplePickParser * m_SimplePickParser;
};
}  // namespace input
}  // namespace glass3
#endif  // INPUT_H
