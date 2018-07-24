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
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

namespace glass {
/**
 * \brief glass fileOutput class
 *
 * The glass fileOutput class is a thread class encapsulating the detection fileOutput
 * logic.  The fileOutput class handles fileOutput messages from from glasscore,
 * and writes the messages out to disk.
 *
 * fileOutput inherits from the threadbaseclass class.
 * fileOutput implements the ifileOutput interface.
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
	explicit fileOutput(std::shared_ptr<json::Object> &config);

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
	 * Clears all configuration, clears and reallocates the message queue and
	 * cache
	 */
	void clear() override;

	const std::string getSOutputDir() {
		m_FileOutputConfigMutex.lock();
		std::string fileOutputdir = m_sOutputDir;
		m_FileOutputConfigMutex.unlock();
		return fileOutputdir;
	}

	const std::string getSOutputFormat() {
		m_FileOutputConfigMutex.lock();
		std::string fileOutputformat = m_sOutputFormat;
		m_FileOutputConfigMutex.unlock();
		return fileOutputformat;
	}

	bool getBTimestampFileName() {
		m_FileOutputConfigMutex.lock();
		bool timestamp = m_bTimestampFileName;
		m_FileOutputConfigMutex.unlock();
		return timestamp;
	}

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
	 * \brief the mutex for configuration
	 */
	std::mutex m_FileOutputConfigMutex;

	/**
	 * \brief the boolean configuration flag determining whether to include
	 * an epoch timestamp in the file name.
	 */
	bool m_bTimestampFileName;
};
}  // namespace glass
#endif  // FILEOUTPUT_H
