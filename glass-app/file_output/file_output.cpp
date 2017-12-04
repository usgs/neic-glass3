#include <file_output.h>
#include <json.h>

#include <detection-formats.h>
#include <logger.h>
#include <fileutil.h>
#include <output.h>

#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <fstream>

namespace glass {

fileOutput::fileOutput()
		: output() {
	logger::log("debug", "fileOutput::fileOutput(): Construction.");

	// init config to defaults and allocate
	clear();
}

fileOutput::fileOutput(json::Object *config)
		: output() {
	logger::log("debug", "fileOutput::fileOutput(): Advanced Construction.");

	// init config to defaults and allocate
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

fileOutput::~fileOutput() {
	logger::log("debug", "fileOutput::~fileOutput(): Destruction.");

	// stop the input thread
	stop();
}

// configuration
bool fileOutput::setup(json::Object *config) {
	if (config == NULL) {
		logger::log("error",
					"fileOutput::setup(): NULL configuration passed in.");
		return (false);
	}

	logger::log("debug", "fileOutput::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		logger::log("error",
					"fileOutput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"];
		if (configtype != "GlassOutput") {
			logger::log("error",
						"fileOutput::setup(): Wrong configuration provided, "
								"configuration is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_FileOutputConfigMutex.lock();

	// fileOutputdir
	if (!(config->HasKey("OutputDirectory"))) {
		// fileOutputdir is required
		m_sOutputDir = "";
		logger::log("error",
					"fileOutput::setup(): OutputDirectory not specified.");
		return (false);
	} else {
		m_sOutputDir = (*config)["OutputDirectory"].ToString();
		logger::log(
				"info",
				"fileOutput::setup(): Using Output Directory: " + m_sOutputDir
						+ " .");
	}

	// fileOutputformat
	if (!(config->HasKey("OutputFormat"))) {
		// fileOutputformat is optional, default to JSON
		m_sOutputFormat = "json";
		logger::log(
				"info",
				"fileOutput::setup(): OutputMethod not specified, defaulting to "
				"json fileOutput.");
	} else {
		m_sOutputFormat = (*config)["OutputFormat"].ToString();
		logger::log(
				"info",
				"fileOutput::setup(): Using Output format: " + m_sOutputFormat
						+ " .");
	}

	// author
	if (!(config->HasKey("TimeStampFileName"))) {
		// TimeStampFileName is optional
		m_bTimestampFileName = true;
		logger::log(
				"info",
				"fileOutput::setup(): Defaulting to including a time stamp in "
				"fileOutput file names.");
	} else {
		m_bTimestampFileName = (*config)["TimeStampFileName"].ToBool();
		logger::log(
				"info",
				"fileOutput::setup(): Including a time stamp in fileOutput file names "
						"is: " + std::to_string(m_bTimestampFileName) + ".");
	}

	// unlock our configuration
	m_FileOutputConfigMutex.unlock();

	logger::log("debug", "fileOutput::setup(): Done Setting Up.");

	// finally do baseclass setup;
	output::setup(config);

	// we're done
	return (true);
}

void fileOutput::clear() {
	logger::log("debug", "fileOutput::clear(): clearing configuration.");

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_FileOutputConfigMutex.lock();

	m_sOutputDir = "";
	m_sOutputFormat = "";

	m_bTimestampFileName = true;

	// unlock our configuration
	m_FileOutputConfigMutex.unlock();

	// finally do baseclass clear
	output::clear();
}

// send output
void fileOutput::sendOutput(const std::string &type, const std::string &id,
							const std::string &message) {
	if (type == "") {
		logger::log(
				"error",
				"fileOutput::sendOutput(): empty type passed in.");
		return;
	}

	if (id == "") {
		logger::log("error",
					"fileOutput::sendOutput(): empty id passed in.");
		return;
	}

	if (message == "") {
		logger::log(
				"error",
				"fileOutput::sendOutput(): empty message passed in.");
		return;
	}

	// ignore message types we can't handle
	if ((type == "StationInfoRequest") || (type == "StationList")) {
		return;
	}

	std::string fileOutputdir = getSOutputDir();
	bool timeStampName = getBTimestampFileName();

	logger::log(
			"info",
			"fileOutput::sendOutput(): Writing to output file a "
					+ type + " message with id: " + id + ".");

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
			logger::log(
					"error",
					"fileOutput::sendOutput: Failed to create file "
							+ filename + "; Second try.");
			return;
		} else {
			logger::log(
					"debug",
					"fileOutput::sendOutput: Created file " + filename
							+ "; Second Try.");
		}
	}

	// write file
	try {
		outfile << OutputData;
	} catch (const std::exception &e) {
		logger::log(
				"error",
				"fileOutput::sendOutput: Problem writing json data to disk: "
						+ std::string(e.what()));
		logger::log(
				"error",
				"fileOutput::sendOutput: Problem Data: "
						+ OutputData);
		return;
	}

	// done
	outfile.close();

	// done
	return;
}
}  // namespace glass
