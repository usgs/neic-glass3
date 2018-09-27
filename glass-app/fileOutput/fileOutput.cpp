#include <fileOutput.h>
#include <json.h>  // NOLINT(build/include)
#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <output.h>

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>

namespace glass3 {

// ---------------------------------------------------------fileOutput
fileOutput::fileOutput()
		: glass3::output::output() {
	glass3::util::Logger::log("debug",
								"fileOutput::fileOutput(): Construction.");

	// init config to defaults and allocate
	clear();
}

// ---------------------------------------------------------fileOutput
fileOutput::fileOutput(const std::shared_ptr<const json::Object> &config)
		: glass3::output::output() {
	glass3::util::Logger::log(
			"debug", "fileOutput::fileOutput(): Advanced Construction.");

	// init config to defaults and allocate
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

// ---------------------------------------------------------~fileOutput
fileOutput::~fileOutput() {
	glass3::util::Logger::log("debug",
								"fileOutput::~fileOutput(): Destruction.");

	// stop the input thread
	stop();
}

// ---------------------------------------------------------setup
bool fileOutput::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::Logger::log(
				"error", "fileOutput::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::Logger::log("debug", "fileOutput::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Configuration"))) {
		glass3::util::Logger::log(
				"error", "fileOutput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"];
		if (configtype != "GlassOutput") {
			glass3::util::Logger::log(
					"error",
					"fileOutput::setup(): Wrong configuration provided, "
							"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	getMutex().lock();

	// fileOutputdir
	if (!(config->HasKey("OutputDirectory"))) {
		// fileOutputdir is required
		m_sOutputDir = "";
		glass3::util::Logger::log(
				"error", "fileOutput::setup(): OutputDirectory not specified.");
		return (false);
	} else {
		m_sOutputDir = (*config)["OutputDirectory"].ToString();
		glass3::util::Logger::log(
				"info",
				"fileOutput::setup(): Using Output Directory: " + m_sOutputDir
						+ " .");
	}

	// fileOutputformat
	if (!(config->HasKey("OutputFormat"))) {
		// fileOutputformat is optional, default to JSON
		m_sOutputFormat = "json";
		glass3::util::Logger::log(
				"info",
				"fileOutput::setup(): OutputMethod not specified, defaulting to "
				"json fileOutput.");
	} else {
		m_sOutputFormat = (*config)["OutputFormat"].ToString();
		glass3::util::Logger::log(
				"info",
				"fileOutput::setup(): Using Output format: " + m_sOutputFormat
						+ " .");
	}

	// author
	if (!(config->HasKey("TimeStampFileName"))) {
		// TimeStampFileName is optional
		m_bTimestampFileName = true;
		glass3::util::Logger::log(
				"info",
				"fileOutput::setup(): Defaulting to including a time stamp in "
				"fileOutput file names.");
	} else {
		m_bTimestampFileName = (*config)["TimeStampFileName"].ToBool();
		glass3::util::Logger::log(
				"info",
				"fileOutput::setup(): Including a time stamp in fileOutput file names "
						"is: " + std::to_string(m_bTimestampFileName) + ".");
	}

	getMutex().unlock();

	glass3::util::Logger::log("debug", "fileOutput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	glass3::output::output::setup(config);

	// we're done
	return (true);
}

// ---------------------------------------------------------clear
void fileOutput::clear() {
	getMutex().lock();
	m_sOutputDir = "";
	m_sOutputFormat = "";
	getMutex().unlock();

	m_bTimestampFileName = true;

	// finally do baseclass clear
	glass3::output::output::clear();
}

// ---------------------------------------------------------sendOutput
void fileOutput::sendOutput(const std::string &type, const std::string &id,
							const std::string &message) {
	if (type == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty type passed in.");
		return;
	}

	if (id == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty id passed in.");
		return;
	}

	if (message == "") {
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput(): empty message passed in.");
		return;
	}

	// ignore message types we can't handle
	if ((type == "StationInfoRequest") || (type == "StationList")) {
		return;
	}

	std::string fileOutputdir = getOutputDir();
	bool timeStampName = getTimestampFileName();

	glass3::util::Logger::log(
			"info",
			"fileOutput::sendOutput(): Writing to output file a " + type
					+ " message with id: " + id + ".");

	std::string OutputData = "";
	std::string Extension = "";

	if (type == "Detection") {
		OutputData = message;
		Extension = std::string(DETECTIONEXTENSION);
	} else if (type == "Retraction") {
		OutputData = message;
		Extension = std::string(RETRACTEXTENSION);
	} else {
		// ignore other message types
		return;
	}

	// build time string if requested
	std::string timestring = "";
	if (timeStampName == true) {
		// what time is it now
		time_t tNow;
		std::time(&tNow);

		timestring = std::to_string(static_cast<int>(tNow));
	}

	// get fileOutput info
	std::string hyponame = id;

	// build file name
	std::string filename = "";
#ifdef _WIN32
	if (timeStampName == true) {
		filename = fileOutputdir + "\\" + timestring + "_" + hyponame + "."
		+ Extension;
	} else {
		filename = fileOutputdir + "\\" + hyponame + "." + Extension;
	}
#else
	if (timeStampName == true) {
		filename = fileOutputdir + "/" + timestring + "_" + hyponame + "."
				+ Extension;
	} else {
		filename = fileOutputdir + "/" + hyponame + "." + Extension;
	}

#endif
	// create file
	std::ofstream outfile;
	outfile.open(filename, std::ios::out);
	if ((outfile.rdstate() & std::ifstream::failbit) != 0) {
		// sleep a little while
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// try again
		outfile.open(filename, std::ios::out);
		if ((outfile.rdstate() & std::ifstream::failbit) != 0) {
			glass3::util::Logger::log(
					"error",
					"fileOutput::sendOutput: Failed to create file " + filename
							+ "; Second try.");
			return;
		} else {
			glass3::util::Logger::log(
					"debug",
					"fileOutput::sendOutput: Created file " + filename
							+ "; Second Try.");
		}
	}

	// write file
	try {
		outfile << OutputData;
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"error",
				"fileOutput::sendOutput: Problem writing json data to disk: "
						+ std::string(e.what()));
		glass3::util::Logger::log(
				"error", "fileOutput::sendOutput: Problem Data: " + OutputData);
		return;
	}

	// done
	outfile.close();

	// done
	return;
}

// ---------------------------------------------------------getOutputDir
const std::string fileOutput::getOutputDir() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sOutputDir);
}

// ---------------------------------------------------------getOutputFormat
const std::string fileOutput::getOutputFormat() {
	std::lock_guard<std::mutex> guard(getMutex());
	return(m_sOutputFormat);
}

// ---------------------------------------------------------getTimestampFileName
bool fileOutput::getTimestampFileName() {
	return (m_bTimestampFileName);
}

}  // namespace glass3
