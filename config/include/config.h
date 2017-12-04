/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <json.h>
#include <string>
#include <fstream>

namespace util {
/**
 * \brief glass configuration class
 *
 * The glass config class is a class used to read json formatted configuration
 * files from disk.  The config class filters out comment lines (signified by
 * '#') and white space, and provides the configuration as a json object.
 */
class Config {
 public:
	/**
	 * \brief config constructor
	 *
	 * The constructor for the config class.
	 * Initilizes members to default values.
	 */
	Config();

	/**
	 * \brief config advanced constructor
	 *
	 * The advanced constructor for the config class.
	 * Loads the provided configuration file containing the configuration.
	 *
	 * \param filepath - A std::string containing the path to the configuration
	 * file
	 * \param filename - A std::string containing the configuration file name.
	 */
	Config(std::string filepath, std::string filename);

	/**
	 * \brief config advanced constructor
	 *
	 * The advanced constructor for the config class.
	 * Loads the provided std::string containing the configuration.
	 *
	 * \param newconfig - A std::string containing the json formatted
	 * configuration data to load.
	 */
	explicit Config(std::string newconfig);

	/**
	 * \brief config destructor
	 *
	 * The destructor for the config class.
	 */
	~Config();

	void clear();

	// setup the config class
	// set up for configuration from a file
	/**
	 * \brief config configuration function
	 *
	 * The this function configures the config class.
	 * \param filepath - A std::string containing the path to the configuration
	 * file
	 * \param filename - A std::string containing the configuration file name.
	 * \return returns true if successful, false otherwise
	 */
	bool setup(std::string filepath, std::string filename);

	/**
	 * \brief Load a configuration file from disk
	 */
	void loadConfigfile();

	/**
	 * \brief Passes a json formatted string into the config class
	 *
	 * \param newconfig - A std::string containing the json formatted
	 * configuration data to load.
	 */
	void loadConfigstring(std::string newconfig);

	/**
	 * \brief Get current configuration as json object
	 *
	 * \returns Return a json::Object containing the configuration
	 */
	json::Object getConfigJSON();

	/**
	 * \brief Get current configuration as a string
	 *
	 * \returns Return a std::string containing the json formatted
	 * configuration string.
	 */
	std::string getConfig_String();

	/**
	 *\brief getter for m_sFileName
	 */
	const std::string& getFileName() const {
		return m_sFileName;
	}

	/**
	 *\brief getter for m_sFilePath
	 */
	const std::string& getFilePath() const {
		return m_sFilePath;
	}

 protected:
	/**
	 * \brief Configuration parsing function
	 *
	 * Parses the provided string into a json::Object.
	 * Sets m_ConfigJSON to the parsed json::Object
	 * Also sets m_sConfigString to the provided string.
	 * \param newconfig - A std::string containing the json formatted
	 * configuration data to parse
	 */
	bool setConfigString(std::string newconfig);

	/**
	 * \brief Opens the configuration file
	 */
	bool openConfigFile();

	/**
	 * \brief Checks that the configuration file
	 * is still open and is not at the end.
	 */
	std::string getNextLineFromConfigFile();

	/**
	 * \brief Opens the configuration file
	 */
	bool hasDataConfigFile();

	/**
	 * \brief Closes teh open configuration file.
	 */
	bool closeConfigFile();

 private:
	/**
	 * \brief The path to the configuration file
	 */
	std::string m_sFilePath;

	/**
	 * \brief The name of the configuration file
	 */
	std::string m_sFileName;

	/**
	 * \brief The configuration file stream
	 */
	std::ifstream m_InFile;

	/**
	 * \brief The configuration loaded from the config file as
	 * a std::string.
	 */
	std::string m_sConfigString;

	/**
	 * \brief The configuration loaded from the config file as
	 * a json::Object
	 */
	json::Object m_ConfigJSON;
};
}  // namespace util
#endif  // CONFIG_H

