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
#include <memory>

namespace glass3 {
namespace util {
/**
 * \brief neic-glass3 configuration class using JSON formatting
 *
 * The glass3::util::Config class is a class used to read JSON formatted configuration
 * files from disk.  The Config class filters out comment lines (signified by
 * '#'), newlines, and white space, and provides the configuration as a JSON
 * object.
 *
 * \warning This class is NOT thread safe
 */
class Config {
 public:
	/**
	 * \brief Config constructor
	 *
	 * The constructor for the Config class.
	 * Initializes members to default values.
	 */
	Config();

	/**
	 * \brief An advanced constructor that loads configuration from a JSON
	 * formatted file accessed via filePath/fileName
	 *
	 * Loads the provided configuration file identified by filePath/fileName
	 * which contains the configuration.
	 *
	 * \param filePath - A std::string containing the path to the configuration
	 * file
	 * \param fileName - A std::string containing the configuration file name.
	 *
	 * \throws For possible exceptions passed through this constructor see
	 * Config::parseJSONFromString and Config::openFile
	 */
	Config(std::string filePath, std::string fileName);

	/**
	 * \brief An advanced constructor that parses configuration from a JSON
	 * formatted string provided in newConfig
	 *
	 * Parses the provided newConfig std::string which contains the
	 * configuration.
	 *
	 * \param newConfig - A std::string containing the JSON formatted
	 * configuration data to load.
	 *
	 * \throws For possible exceptions passed through this constructor see
	 * Config::parseJSONFromString
	 */
	explicit Config(std::string newConfig);

	/**
	 * \brief Config destructor
	 *
	 * The destructor for the Config class.
	 */
	~Config();

	/**
	 * \brief A function that loads configuration from a JSON formatted file
	 * accessed via filePath/fileName
	 *
	 * Loads the provided configuration file identified by filePath/fileName
	 * which contains the configuration.
	 *
	 * \param filePath - A std::string containing the path to the configuration
	 * file
	 * \param fileName - A std::string containing the configuration file name.
	 *
	 * \return Returns a json::Object containing the loaded configuration
	 *
	 * \throws For possible exceptions passed through this function see
	 * Config::parseJSONFromString and Config::openFile
	 */
	std::shared_ptr<json::Object> parseJSONFromFile(std::string filePath,
													std::string fileName);

	/**
	 * \brief A function that parses a configuration from a JSON formatted
	 * string
	 *
	 * Parses the provided JSON formatted configuration string contained in
	 * newConfig
	 *
	 * \param newConfig - A std::string containing the JSON formatted
	 * configuration data to parse.
	 *
	 * \return Returns a json::Object containing the loaded configuration
	 * \throw Throws std::invalid_argument if the newConfig string is empty
	 * \throw Throws std::invalid_argument if the newConfig string failed to
	 * parse
	 */
	std::shared_ptr<json::Object> parseJSONFromString(std::string newConfig);

	/**
	 * \brief Get configuration as json object
	 *
	 * Get the last loaded/parsed configuration as a json::Object
	 *
	 * \returns Return a json::Object containing the configuration
	 */
	std::shared_ptr<json::Object> getJSON();

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
	 * \param filePath - A std::string containing the path to the configuration
	 * file
	 * \param fileName - A std::string containing the configuration file name.
	 *
	 * \return Returns a std::ifstream for accessing the open file
	 * \throw Throws std::invalid_argument if the fileName is empty
	 * \throw Throws std::ios_base::failure if the file failed to open
	 */
	std::ifstream openFile(std::string filePath, std::string fileName);

	/**
	 * \brief Parses next line from file
	 *
	 * Parses the next line from the provided file referenced by inFile,
	 * removing tabs, and comment lines
	 *
	 * \param inFile - A reference to the std::ifstream to get the next line
	 * from.
	 *
	 * \return Returns a std::string containing the next line
	 */
	std::string parseLineFromFile(std::ifstream &inFile);

	/**
	 * \brief Checks if provided file is open
	 *
	 * Checks that the provided file referenced by inFile is still open and is
	 * not at the end of the file
	 *
	 * \param inFile - A reference to the std::ifstream to check
	 *
	 * \return Returns true if the file is open and valid, false otherwise
	 */
	bool isFileOpen(std::ifstream &inFile);

	/**
	 * \brief Closes the open file.
	 *
	 * This function closes the provided file referenced by inFile
	 *
	 * \param inFile - A reference to the std::ifstream to close
	 */
	void closeFile(std::ifstream &inFile);

 private:
	/**
	 * \brief The "parsed" (whitespace and comments removed) configuration
	 * string
	 */
	std::string m_sConfigString;

	/**
	 * \brief The parsed configuration as a json::Object
	 */
	std::shared_ptr<json::Object> m_ConfigJSON;
};
}  // namespace util
}  // namespace glass3
#endif  // CONFIG_H

