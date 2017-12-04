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
	clear();

	setup(filepath, filename);

	loadConfigfile();
}

Config::Config(std::string newconfig) {
	clear();

	loadConfigstring(newconfig);
}

Config::~Config() {
	clear();
}

// set up for configuration from a file
bool Config::setup(std::string filepath, std::string filename) {
	m_sFilePath = filepath;
	m_sFileName = filename;

	m_ConfigJSON.Clear();
	m_sConfigString = "";

	return (true);
}

void Config::clear() {
        m_sFilePath = "";
        m_sFileName = "";

        m_sConfigString = "";
        m_ConfigJSON.Clear();
}

// load the provided config file
void Config::loadConfigfile() {
	// first open the file
	if (openConfigFile() == false) {
		return;
	}

	std::string currentline = "";
	std::string configline = "";

	// read the file
	while (hasDataConfigFile() == true) {
		currentline = getNextLineFromConfigFile();

		if (currentline == "") {
			continue;
		}

		// tack the current line on the end of the
		// config string
		configline += currentline;
	}

	logger::log("debug",
				"config::load_configfile: read from configfile: " + configline);

	// set the whole config line as our current config
	if (setConfigString(configline) == true)
		logger::log(
				"info",
				"config::load_configfile: successfully read configuration from "
				"file.");

	// done with file
	closeConfigFile();
}

void Config::loadConfigstring(std::string newconfig) {
	// set the provided line as our new config
	if (setConfigString(newconfig) == true)
		logger::log(
				"info",
				"config::load_configstring: successfully read configuration from"
				" string.");
}

// get the configuration as a JSON object
json::Object Config::getConfigJSON() {
	return (m_ConfigJSON);
}

// get the configuration as a string
std::string Config::getConfig_String() {
	return (m_sConfigString);
}

// set (and parse into JSON) a configuration string
bool Config::setConfigString(std::string newconfig) {
	// now that we have the whole config string
	// deserailize the config string into a json value
	// I think (hope) this ignores newlines and whitespace
	json::Value deserializedJSON = json::Deserialize(newconfig);

	// make sure we got valid json
	if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
		// save our config string
		m_sConfigString = newconfig;

		// convert our resulting value to a json object
		m_ConfigJSON = deserializedJSON.ToObject();

		logger::log(
				"debug",
				"config::setconfig_string: json::Deserialize read: {"
						+ json::Serialize(deserializedJSON)
						+ "} from configuration string.");
		return (true);
	} else {
		// we're in trouble, clear our stuff
		m_sConfigString = "";
		m_ConfigJSON.Clear();

		// yell
		logger::log(
				"error",
				"config::setconfig_string: json::Deserialize returned null, "
				"invalid configuration string.");
		return (false);
	}
}

// file operations
bool Config::openConfigFile() {
	// nullchecks
	if (m_sFileName.length() == 0) {
		return (false);
	}
	if (m_InFile.is_open() == true) {
		return (false);
	}

	// create the filename
	std::string filename = "";

	if (m_sFilePath.length() == 0)
		filename = m_sFileName;
	else
		filename = m_sFilePath + "/" + m_sFileName;

	// open the file
	m_InFile.open(filename, std::ios::in);

	if (!m_InFile) {
		// yell if we failed to open the file
		logger::log(
				"error",
				"config::open_configfile: failed to open file: " + filename);
		return (false);
	}

	return (true);
}

std::string Config::getNextLineFromConfigFile() {
	// make sure we've got a file open
	if (m_InFile.is_open() == false) {
		return ("");
	}

	// get the next line
	std::string line;
	std::getline(m_InFile, line);

	// empty line
	if (line.length() == 0) {
		return ("");
	}

	// strip tabs
	line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());

	// now look for # in the line, # are comments
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

bool Config::hasDataConfigFile() {
	// make sure file is open
	if (m_InFile.is_open() == false) {
		return (false);
	}

	// make sure file is valid (and not at the end)
	if (m_InFile) {
		return (true);
	} else {
		return (false);
	}
}

bool Config::closeConfigFile() {
	// don't close it if it's not open
	if (m_InFile.is_open() == false) {
		return (false);
	}

	m_InFile.close();

	// can you fail to close a file?
	return (true);
}
}  // namespace util
