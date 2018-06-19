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
 * \brief neic-glass3 configuration class using JSON formatting
 *
 * The glass config class is a class used to read JSON formatted configuration
 * files from disk.  The config class filters out comment lines (signified by
 * '#'), newlines, and white space, and provides the configuration as a json
 * object.
 *
 * NOTE: This class is NOT thread safe
 */
class Config {
 public:
	/**
	 * \brief config constructor
	 *
	 * The constructor for the config class.
	 * Initializes members to default values.
	 */
	Config();

	/**
	 * \brief An advanced constructor that loads configuration from a JSON
	 * formatted file accessed via filepath/filename
	 *
	 * Loads the provided configuration file identified by filepath/filename
	 * which contains the configuration.
	 *
	 * \param filepath - A std::string containing the path to the configuration
	 * file
	 * \param filename - A std::string containing the configuration file name.
	 */
	Config(std::string filepath, std::string filename);

	/**
	 * \brief An advanced constructor that parses configuration from a JSON
	 * formatted string provided in nuewconfig
	 *
	 * Parses the provided newconfig std::string which contains the
	 * configuration.
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

	/**
	 * \brief A function that loads configuration from a JSON formatted file
	 * accessed via filepath/filename
	 *
	 * Loads the provided configuration file identified by filepath/filename
	 * which contains the configuration.
	 *
	 * \param filepath - A std::string containing the path to the configuration
	 * file
	 * \param filename - A std::string containing the configuration file name.
	 */
	json::Object parseJSONFromFile(std::string filepath, std::string filename);

	/**
	 * \brief A function that parses a configuration from a JSON formatted
	 * string
	 *
	 * \param newconfig - A std::string containing the json formatted
	 * configuration data to parse.
	 */
	json::Object parseJSONFromString(std::string newconfig);

	/**
	 * \brief Get current configuration as json object
	 *
	 * \returns Return a json::Object containing the configuration
	 */
	json::Object getJSON();

	/**
	 * \brief Get parse status as a string
	 *
	 * \returns Return a std::string containing the parse status for error
	 * reporting purposes
	 */
	std::string getParseStatus();

	/**
	 * \brief Config clear function
	 *
	 * Returns class members to default values.
	 */
	void clear();

 protected:
	/**
	 * \brief Opens the configuration file
	 *
	 * \param filepath - A std::string containing the path to the configuration
	 * file
	 * \param filename - A std::string containing the configuration file name.
	 */
	std::ifstream openFile(std::string filepath, std::string filename);

	/**
	 * \brief Parses the next line from the file, removing tabs, and comment
	 * lines
	 */
	std::string parseLineFromFile(std::ifstream &inFile);

	/**
	 * \brief Checks that the configuration file
	 * is still open and is not at the end.
	 */
	bool isFileOpen(std::ifstream &inFile);

	/**
	 * \brief Closes the open configuration file.
	 */
	void closeFile(std::ifstream &inFile);

 private:
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

