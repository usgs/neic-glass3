/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef INPUT_H
#define INPUT_H

#include <json.h>
#include <logger.h>
#include <fileutil.h>
#include <timeutil.h>
#include <threadbaseclass.h>
#include <inputinterface.h>
#include <gpickparser.h>
#include <jsonparser.h>
#include <ccparser.h>
#include <queue.h>

#include <vector>
#include <queue>
#include <chrono>
#include <mutex>
#include <string>
#include <memory>

#define GPICK_EXTENSION "gpick"
#define GPICKS_EXTENSION "gpicks"
#define JSON_EXTENSION "json"
#define CC_EXTENSION "dat"

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
class input : public glass3::util::iInput, public glass3::util::ThreadBaseClass {
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
	input(std::shared_ptr<json::Object> config, int linesleepms);

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
	bool setup(std::shared_ptr<json::Object> config) override;

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
	std::shared_ptr<json::Object> getData() override;

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
	 * \brief getter for the m_bArchive configuration variable
	 */
	bool getBArchive() {
		m_ConfigMutex.lock();
		bool archive = m_bArchive;
		m_ConfigMutex.unlock();
		return archive;
	}

	/**
	 * \brief getter for the m_bError configuration variable
	 */
	bool getBError() {
		m_ConfigMutex.lock();
		bool error = m_bError;
		m_ConfigMutex.unlock();
		return error;
	}

	/**
	 * \brief getter for the m_Formats configuration variable
	 */
	const json::Array getFormats() {
		m_ConfigMutex.lock();
		json::Array formats = m_Formats;
		m_ConfigMutex.unlock();
		return formats;
	}

	/**
	 * \brief getter for the m_QueueMaxSize configuration variable
	 */
	int getQueueMaxSize() {
		m_ConfigMutex.lock();
		int maxsize = m_QueueMaxSize;
		m_ConfigMutex.unlock();
		return maxsize;
	}

	/**
	 * \brief getter for the m_sArchiveDir configuration variable
	 */
	const std::string getSArchiveDir() {
		m_ConfigMutex.lock();
		std::string archivedir = m_sArchiveDir;
		m_ConfigMutex.unlock();
		return archivedir;
	}

	/**
	 * \brief getter for the m_sDefaultAgencyID configuration variable
	 */
	const std::string getSDefaultAgencyId() {
		m_ConfigMutex.lock();
		std::string agencyid = m_sDefaultAgencyID;
		m_ConfigMutex.unlock();
		return agencyid;
	}

	/**
	 * \brief getter for the m_sDefaultAuthor configuration variable
	 */
	const std::string getSDefaultAuthor() {
		m_ConfigMutex.lock();
		std::string author = m_sDefaultAuthor;
		m_ConfigMutex.unlock();
		return author;
	}

	/**
	 * \brief getter for the m_sErrorDir configuration variable
	 */
	const std::string getSErrorDir() {
		m_ConfigMutex.lock();
		std::string errordir = m_sErrorDir;
		m_ConfigMutex.unlock();
		return errordir;
	}

	/**
	 * \brief getter for the m_sInputDir configuration variable
	 */
	const std::string getSInputDir() {
		m_ConfigMutex.lock();
		std::string inputdir = m_sInputDir;
		m_ConfigMutex.unlock();
		return inputdir;
	}

	/**
	 * \brief getter for the m_iInFileSleep variable
	 */
	int getIInFileSleep() const {
		return m_iInFileSleep;
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
	 * \brief read files function
	 *
	 * The function that reads and parses input files, based on the given
	 * extension and input directory
	 *
	 * \param extension - A std::string containing the extension of files to
	 * read
	 * \param inputdir - A std::string containing the input directory to read
	 * from
	 * \param archive - A boolean flag indicating whether to archive input files.
	 * \param archivedir - A std::string containing the directory to archive
	 * files to, if archiving is configured.
	 * \param error - A boolean flag indicating whether to keep input files with
	 * errors.
	 * \param errordir - A std::string containing the directory to keep files
	 * with errors, if configured.
	 * \return returns true if successful, false otherwise.
	 */
	bool readFiles(std::string extension, const std::string &inputdir,
					bool archive, const std::string &archivedir, bool error,
					const std::string &errordir);

	/**
	 * \brief parse line function
	 *
	 * The function that parses an input line, based on the given extension
	 *
	 * \param input - A std::string containing the input line to parse
	 * \param extension - A std::string containing the extension to parse
	 * \return returns a pointer to a json::Object containing the parsed data
	 */
	virtual std::shared_ptr<json::Object> parse(std::string extension,
												std::string input);

	/**
	 * \brief validate data function
	 *
	 * The function that validates input data, based on the given extension
	 *
	 * \param extension - A std::string containing the extension to validate
	 * \param input - A json::Object containing the data to validate
	 * \return returns true if valid, false otherwise
	 */
	virtual bool validate(std::string extension,
							std::shared_ptr<json::Object> input);

	/**
	 * \brief cleanup file function
	 *
	 * The function that moves the file identified by filename to the
	 * destination directory, or deletes it if move is false
	 *
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
	 * \brief the std::string configuration value indicating the input directory
	 */
	std::string m_sInputDir;

	/**
	 * \brief the boolean configuration flag indicating whether to archive our
	 * input
	 */
	bool m_bArchive;

	/**
	 * \brief the std::string configuration value indicating the archive
	 * directory
	 */
	std::string m_sArchiveDir;

	/**
	 * \brief the boolean configuration flag indicating whether to keep files
	 * with errors
	 */
	bool m_bError;

	/**
	 * \brief the std::string configuration value indicating the error directory
	 */
	std::string m_sErrorDir;

	/**
	 * \brief the configuration json::Array containing the input types to
	 * process
	 */
	json::Array m_Formats;

	/**
	 * \brief the boolean configuration flag indicating whether to shut down
	 * input when there is no data
	 */
	bool m_bShutdownWhenNoData;

	/**
	 * \brief the integer value containing the number of seconds to wait when
	 * shutting down due to no data.
	 */
	int m_iShutdownWait;

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
	int m_QueueMaxSize;

	/**
	 * \brief the mutex for configuration
	 */
	std::mutex m_ConfigMutex;

	/**
	 * \brief the data queue
	 */
	glass3::util::Queue* m_DataQueue;

	/**
	 * \brief the time, in milliseconds, to sleep between reading lines from
	 * an input file
	 */
	int m_iInFileSleep;

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
}  // namespace glass
#endif  // INPUT_H
