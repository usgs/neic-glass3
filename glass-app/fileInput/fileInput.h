/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef FILEINPUT_H
#define FILEINPUT_H

#include <json.h>
#include <threadbaseclass.h>
#include <input.h>

#include <chrono>
#include <mutex>
#include <string>
#include <memory>

#define GPICK_EXTENSION "gpick"
#define GPICKS_EXTENSION "gpicks"
#define JSON_EXTENSION "json"
#define CC_EXTENSION "dat"

namespace glass3 {
/**
 * \brief glass fileInput class
 *
 * The glass fileInput class is a class encapsulating the file input logic.
 * The fileInput class handles reading data from disk, and sending the
 * data to glasscore via the associator class.
 *
 * fileInput inherits from the glass3::input::Input class.
 */
class fileInput : public glass3::input::Input {
 public:
	/**
	 * \brief fileInput constructor
	 *
	 * The constructor for the fileInput class.
	 * Initializes members to default values.
	 */
	fileInput();

	/**
	 * \brief fileInput advanced constructor
	 *
	 * The advanced constructor for the fileInput class.
	 * Initializes members to default values.
	 * Calls setup to configure the class
	 * Starts the work thread
	 *
	 * \param config - A json::Object pointer to the configuration to use
	 */
	explicit fileInput(const std::shared_ptr<const json::Object> &config);

	/**
	 * \brief fileInput destructor
	 *
	 * The destructor for the fileInput class.
	 * Stops the work thread
	 */
	~fileInput();

	/**
	 * \brief fileInput configuration function
	 *
	 * The this function configures the fileInput class, and allocates the parsing
	 * objects and data queue.
	 *
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	bool setup(std::shared_ptr<const json::Object> config) override;

	/**
	 * \brief output clear function
	 *
	 * The clear function for the output class.
	 * Clears all configuration.
	 */
	void clear() override;

	/**
	 * \brief Function to retrieve the name of the input directory
	 *
	 * This function retrieves the name of the input directory
	 * \return A std::string containing the input directory name
	 */
	const std::string getInputDir();

	/**
	 * \brief Function to set the name of  the input directory
	 *
	 * This function sets the name of of the input directory
	 * \param dir = A std::string containing the input directory to set
	 */
	void setInputDir(const std::string &dir);

	/**
	 * \brief Function to retrieve the name of the archive directory
	 *
	 * This function retrieves the name of the archive directory
	 *
	 * \return A std::string containing the archive directory name
	 */
	const std::string getArchiveDir();

	/**
	 * \brief Function to set the name of  the archive directory
	 *
	 * This function sets the name of of the archive directory
	 * \param dir = A std::string containing the archive directory to set
	 */
	void setArchiveDir(const std::string &dir);

	/**
	 * \brief Function to retrieve the format type (extension)
	 *
	 * This function retrieves the format type (extension)
	 *
	 * \return A std::string containing the format type (extension)
	 */
	const std::string getFormat();

	/**
	 * \brief Function to set the format type (extension)
	 *
	 * This function sets the format type (extension)
	 * \param format = A std::string containing the format type
	 */
	void setFormat(const std::string &format);

	/**
	 * \brief Function to set whether to shutdown when no data
	 *
	 * This function sets the boolean flag indicating whether to shutdown when
	 * there is no data
	 * \param shutdown = A boolean flag indicating whether to shut down
	 */
	void setShutdownWhenNoData(bool shutdown);

	/**
	 * \brief Function to retrieve whether to shutdown when no data
	 *
	 * This function retrieves the boolean flag indicating whether to shutdown
	 * when there is no data
	 *
	 * \return A boolean flag indicating whether to shut down
	 */
	bool getShutdownWhenNoData();

	/**
	 * \brief Function to set the shutdown wait time
	 *
	 * This function sets the wait time before shutting down due to no
	 * data
	 * \param waitTime = An integer value containing the shutdown wait in seconds
	 */
	void setShutdownWait(int waitTime);

	/**
	 * \brief Function to retrieve the shutdown wait time
	 *
	 * This function retrieves the wait time before shutting down due to no
	 * data
	 * \return Returns an integer value containing the shutdown wait in seconds
	 */
	int getShutdownWait();

 protected:
	/**
	 * \brief get input data string and type
	 *
	 * A function (overridden from glass3::input) that that retrieves the next
	 * data message and type from an input source
	 * \param pOutType - A pointer to a std::string used to pass out the type of
	 * the data
	 * \return returns a std::string containing the input data message
	 */
	std::string fetchRawData(std::string* pOutType) override;

	/**
	 * \brief cleanup file function
	 *
	 * The function that moves the file identified by filename to the
	 * destination directory, or deletes it if move is false
	 * \param filename - A std::string containing the file to clean up
	 * \param move - A boolean flag indicating whether to move or delete the
	 * file
	 * \param destinationdir - A std::string containing the directory to move
	 * the file to, if desired.
	 */
	void cleanupFile(std::string filename, bool move,
						std::string destinationdir);

 private:
	/**
	 * \brief the std::string configuration value indicating the fileInput directory
	 */
	std::string m_sInputDir;

	/**
	 * \brief the std::string configuration value indicating the archive
	 * directory
	 */
	std::string m_sArchiveDir;

	/**
	 * \brief the std::string configuration value indicating the format
	 * (extension) of files to input
	 */
	std::string m_sFormat;

	/**
	 * \brief the boolean configuration flag indicating whether to shut down
	 * fileInput when there is no data
	 */
	std::atomic<bool> m_bShutdownWhenNoData;

	/**
	 * \brief the integer value containing the number of seconds to wait when
	 * shutting down due to no data.
	 */
	std::atomic<int> m_iShutdownWait;

	/**
	 * \brief the current input file handle
	 */
	std::ifstream m_InputFile;

	/**
	 * \brief the current input file name
	 */
	std::string m_sFileName;

	/**
	 * \brief the time that the input file started processing
	 */
	std::chrono::high_resolution_clock::time_point m_tFileStartTime;

	/**
	 * \brief the count of data read from the input file
	 */
	int m_iDataCount;
};
}  // namespace glass3
#endif  // FILEINPUT_H
