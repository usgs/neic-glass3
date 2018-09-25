#include <config.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <string>
#include <memory>

namespace glass3 {
namespace util {

// ---------------------------------------------------------Config
Config::Config() {
	clear();
}

// ---------------------------------------------------------Config
Config::Config(std::string filePath, std::string fileName) {
	parseJSONFromFile(filePath, fileName);
}

// ---------------------------------------------------------Config
Config::Config(std::string newConfig) {
	parseJSONFromString(newConfig);
}

// ---------------------------------------------------------~Config
Config::~Config() {
	clear();
}

// ---------------------------------------------------------clear
void Config::clear() {
	m_sConfigString = "";
	m_ConfigJSON.reset();
}

// ---------------------------------------------------------parseJSONFromFile
std::shared_ptr<const json::Object> Config::parseJSONFromFile(
		std::string filePath, std::string fileName) {
	clear();

	// first open the file
	std::ifstream inFile = openFile(filePath, fileName);

	std::string currentline = "";
	std::string configline = "";

	// read the file
	while (isFileOpen(inFile) == true) {
		// get the next line, stripped of whitespace and comments
		currentline = parseLineFromFile(inFile);

		// skip
		if (currentline == "") {
			continue;
		}

		// tack the current line on the end of the
		// config string
		configline += currentline;
	}

	// done with file
	closeFile(inFile);

	// parse the whole string into JSON
	return (parseJSONFromString(configline));
}

// ---------------------------------------------------------parseJSONFromString
std::shared_ptr<const json::Object> Config::parseJSONFromString(
		std::string newConfig) {
	clear();

	// nullchecks
	if (newConfig.length() == 0) {
		throw std::invalid_argument("Empty JSON string");
	}

	// deserialize the string into JSON
	json::Value deserializedJSON = json::Deserialize(newConfig);

	// make sure we got valid json
	if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
		// convert our resulting value to a json object
		m_ConfigJSON = std::make_shared<json::Object>(
				json::Object(deserializedJSON.ToObject()));

		glass3::util::Logger::log(
				"debug",
				"config::setconfig_string: json::Deserialize read: {"
						+ json::Serialize(deserializedJSON)
						+ "} from configuration string.");

		m_sConfigString = newConfig;

		return (getJSON());
	} else {
		// we're in trouble, clear our stuff
		clear();

		glass3::util::Logger::log(
				"error",
				"config::parseJSONFromString: Invalid configuration string");
		throw std::invalid_argument("Invalid configuration string");
	}

	// Should never get here
	return (NULL);
}

// ---------------------------------------------------------getJSON
std::shared_ptr<const json::Object> Config::getJSON() {
	if (m_ConfigJSON == NULL) {
		return (NULL);
	}

	return (m_ConfigJSON);
}

// ---------------------------------------------------------openFile
std::ifstream Config::openFile(std::string filePath, std::string fileName) {
	// nullchecks
	if (fileName.length() == 0) {
		glass3::util::Logger::log("error", "config::openFile: Empty file name");
		throw std::invalid_argument("Empty file name");
	}

	// create the fileName
	std::string fileToOpen = "";

	if (filePath.length() == 0) {
		// no path means just the name
		fileToOpen = fileName;
	} else {
		// combine path and name
		fileToOpen = filePath + "/" + fileName;
	}

	// open the file
	std::ifstream inFile;
	inFile.open(fileToOpen, std::ios::in);

	if (!inFile) {
		// yell if we failed to open the file
		glass3::util::Logger::log(
				"error",
				"config::openFile: Failed to open file: " + fileToOpen);
		throw std::ios_base::failure("Failed to open file");
	}

	return (inFile);
}

// ---------------------------------------------------------parseLineFromFile
std::string Config::parseLineFromFile(std::ifstream &inFile) {
	// make sure we've got a file open
	if (inFile.is_open() == false) {
		return ("");
	}

	// get the next line
	std::string line;
	std::getline(inFile, line);

	// empty line
	if (line.length() == 0) {
		return ("");
	}

	// strip tabs
	line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());

	// now look for # in the line, # signify comments, and skip to the
	// the next line
	size_t position = line.find("#");

	if (position == 0) {
		// # found in the first position
		// the whole line is a comment
		// return none of the line
		return ("");
	} else if (position == std::string::npos) {
		// no # found
		// no part of the line is a comment,
		// return entire line
		return (line);
	} else {
		// found # somewhere in the line
		// everything after the # is a comment
		// everything before is the line
		// return the part of the line starting at 0
		// and going to position
		return (line.substr(0, position));
	}

	// should never get here
	return ("");
}

// ---------------------------------------------------------isFileOpen
bool Config::isFileOpen(std::ifstream &inFile) {
	// make sure file is open
	if (inFile.is_open() == false) {
		return (false);
	}

	// make sure file is valid (and not at the end)
	if (inFile.good()) {
		return (true);
	} else {
		return (false);
	}
}

// ---------------------------------------------------------closeFile
void Config::closeFile(std::ifstream &inFile) {
	// don't close it if it's not open
	if (inFile.is_open() == false) {
		return;
	}

	inFile.close();
}
}  // namespace util
}  // namespace glass3
