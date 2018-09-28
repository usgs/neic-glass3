/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef FILEOUTPUT_H
#define FILEOUTPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <output.h>

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

namespace glass3 {
/**
 * \brief glass fileOutput class
 *
 * The glass fileOutput class is a class encapsulating the file output logic.
 * The fileOutput class handles writing messages from glasscore out to disk.
 *
 * fileOutput inherits from the glass3::output::output  class.
 */
class fileOutput : public glass3::output::output {
 public:
	/**
	 * \brief fileOutput constructor
	 *
	 * The constructor for the fileOutput class.
	 * Initializes members to default values.
	 */
	fileOutput();

	/**
	 * \brief fileOutput advanced constructor
	 *
	 * The advanced constructor for the fileOutput class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 */
	explicit fileOutput(const std::shared_ptr<const json::Object> &config);

	/**
	 * \brief fileOutput destructor
	 *
	 * The destructor for the fileOutput class.
	 * Stops the work thread
	 */
	~fileOutput();

	/**
	 * \brief fileOutput configuration function
	 *
	 * The this function configures the fileOutput class, and the tracking cache it
	 * contains.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief fileOutput clear function
	 *
	 * The clear function for the fileOutput class.
	 * Clears all configuration
	 */
	void clear() override;

	/**
	 * \brief Function to retrieve the output directory
	 *
	 * This function retrieves the output directory
	 * \return A std::string containing the output directory
	 */
	const std::string getOutputDir();

	/**
	 * \brief Function to retrieve the output format
	 *
	 * This function retrieves the output format
	 * \return A std::string containing the output format
	 */
	const std::string getOutputFormat();

	/**
	 * \brief Get whether to timestamp output file names
	 *
	 * This function retrieves whether to timestamp output file names
	 * \return A boolean flag indicating whether to timestamp output file names
	 */
	bool getTimestampFileName();

 protected:
	/**
	 * \brief fileOutput file writing function
	 *
	 * The function used fileOutput detection data
	 *
	 * \param type - A std::string containing the output message type
	 * \param id - A std::string containing the output message id
	 * \param message - A std::string containing the output message
	 */
	void sendOutput(const std::string &type, const std::string &id,
					const std::string &message) override;

 private:
	/**
	 * \brief the std::string configuration value defining the fileOutput
	 * directory to write files.
	 */
	std::string m_sOutputDir;

	/**
	 * \brief the std::string configuration value defining the fileOutput
	 * format.
	 */
	std::string m_sOutputFormat;

	/**
	 * \brief the boolean configuration flag determining whether to include
	 * an epoch timestamp in the file name.
	 */
	std::atomic<bool> m_bTimestampFileName;
};
}  // namespace glass3
#endif  // FILEOUTPUT_H
