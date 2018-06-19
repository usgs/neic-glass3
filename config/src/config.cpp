#include <config.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <string>

namespace util {

Config::Config() {
	clear();
}

Config::Config(std::string filepath, std::string filename) {
	parseJSONFromFile(filepath, filename);
}

Config::Config(std::string newconfig) {
	parseJSONFromString(newconfig);
}

Config::~Config() {
	clear();
}

void Config::clear() {
	m_sConfigString = "";
	m_ConfigJSON.Clear();
}

json::Object Config::parseJSONFromFile(std::string filepath,
										std::string filename) {
	clear();

	// first open the file
	std::ifstream inFile = openFile(filepath, filename);

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

json::Object Config::parseJSONFromString(std::string newconfig) {
	clear();

	// nullchecks
	if (newconfig.length() == 0) {
		throw std::invalid_argument("Empty JSON string");
	}

	json::Value deserializedJSON;
	json::Object jsonObject;

	// deserialize the string into JSON
	deserializedJSON = json::Deserialize(newconfig);

	// make sure we got valid json
	if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
		// convert our resulting value to a json object
		jsonObject = deserializedJSON.ToObject();

		logger::log(
				"debug",
				"config::setconfig_string: json::Deserialize read: {"
						+ json::Serialize(deserializedJSON)
						+ "} from configuration string.");
	} else {
		// we're in trouble, clear our stuff
		clear();

		logger::log(
				"error",
				"config::parseJSONFromString: Invalid configuration string");
		throw std::invalid_argument("Invalid configuration string");
	}

	// save our config string and JSON
	m_sConfigString = newconfig;
	m_ConfigJSON = jsonObject;

	return (jsonObject);
}

// get the configuration as a JSON object
json::Object Config::getJSON() {
	return (m_ConfigJSON);
}

// file operations
std::ifstream Config::openFile(std::string filepath, std::string filename) {
	// nullchecks
	if (filename.length() == 0) {
		logger::log("error", "config::openFile: Empty file name");
		throw std::invalid_argument("Empty file name");
	}

	// create the filename
	std::string fileToOpen = "";

	if (filepath.length() == 0) {
		// no path means just the name
		fileToOpen = filename;
	} else {
		// combine path and name
		fileToOpen = filepath + "/" + filename;
	}

	// open the file
	std::ifstream inFile;
	inFile.open(fileToOpen, std::ios::in);

	if (!inFile) {
		// yell if we failed to open the file
		logger::log("error",
					"config::openFile: Failed to open file: " + fileToOpen);
		throw std::ios_base::failure("Failed to open file");
	}

	return (inFile);
}

// parse the next line from the file
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

// check to see if the file is open and not at the end
bool Config::isFileOpen(std::ifstream &inFile) {
	// make sure file is open
	if (inFile.is_open() == false) {
		return (false);
	}

	// make sure file is valid (and not at the end)
	if (inFile) {
		return (true);
	} else {
		return (false);
	}
}

void Config::closeFile(std::ifstream &inFile) {
	// don't close it if it's not open
	if (inFile.is_open() == false) {
		return;
	}

	inFile.close();
}
}  // namespace util
