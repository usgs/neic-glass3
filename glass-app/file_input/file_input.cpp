#include <file_input.h>
#include <json.h>
#include <logger.h>
#include <fileutil.h>

#include <vector>
#include <queue>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <string>
#include <memory>

namespace glass {
// Construction/Destruction
fileInput::fileInput()
		: glass3::input::Input() {
	glass3::util::Logger::log("debug", "fileInput::fileInput(): Construction.");

	// init config to defaults and allocate
	clear();
}

fileInput::fileInput(std::shared_ptr<const json::Object> &config)
		: glass3::input::Input() {
	glass3::util::Logger::log(
			"debug", "fileInput::fileInput(): Advanced Construction.");
	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the fileInput thread
	start();
}

fileInput::~fileInput() {
	glass3::util::Logger::log("debug", "fileInput::~fileInput(): Destruction.");

	// stop the input thread
	stop();
}

// configuration
bool fileInput::setup(std::shared_ptr<const json::Object> config) {
	if (config == NULL) {
		glass3::util::Logger::log(
				"error", "fileInput::setup(): NULL configuration passed in.");
		return (false);
	}

	glass3::util::Logger::log("debug", "fileInput::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Configuration"))) {
		glass3::util::Logger::log(
				"error", "fileInput::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Configuration"].ToString();
		if (configtype != "GlassInput") {
			glass3::util::Logger::log(
					"error",
					"fileInput::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// fileInputdir
	if (!(config->HasKey("InputDirectory"))) {
		// fileInputdir is required
		setInputDir("");
		glass3::util::Logger::log(
				"error",
				"fileInput::setup(): Required configuration value InputDirectory not "
				"specified.");
		return (false);
	} else {
		setInputDir((*config)["InputDirectory"].ToString());
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Using Input Directory: " + getInputDir()
						+ ".");
	}

	// archive fileInput?
	if (!(config->HasKey("ArchiveDirectory"))) {
		// archive is optional
		setArchiveDir("");
		glass3::util::Logger::log(
				"info", "fileInput::setup(): Not Archiving fileInput.");
	} else {
		setArchiveDir((*config)["ArchiveDirectory"].ToString());
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Using Archive Directory: "
						+ getArchiveDir() + " to archive fileInput.");
	}

	// formats
	if (!(config->HasKey("Format"))) {
		// formats is required
		setFormat("gpick");
		glass3::util::Logger::log(
				"warning",
				"fileInput::setup(): No formats specified in configuration, "
				"defaulting to gpick.");
	} else {
		setFormat((*config)["Format"].ToString());
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Using Format: " + getFormat() + ".");
	}

	// shutdown when no data
	if (!(config->HasKey("ShutdownWhenNoData"))) {
		// m_bShutdownWhenNoData is optional
		setShutdownWhenNoData(true);
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Defaulting to true for ShutdownWhenNoData");
	} else {
		setShutdownWhenNoData((*config)["ShutdownWhenNoData"].ToBool());
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Using ShutdownWhenNoData: = "
						+ std::to_string(getShutdownWhenNoData()));
	}

	// shutdown wait when no data
	if (!(config->HasKey("ShutdownWait"))) {
		// m_iShutdownWait is optional
		setShutdownWait(60);
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Defaulting to 60 for ShutdownWait");
	} else {
		setShutdownWait((*config)["ShutdownWait"].ToInt());
		glass3::util::Logger::log(
				"info",
				"fileInput::setup(): Using ShutdownWait: "
						+ std::to_string(getShutdownWait()));
	}

	// finally do baseclass setup;
	// mostly remembering our config object
	glass3::input::Input::setup(config);

	// we're done
	return (true);
}

void fileInput::clear() {
	glass3::util::Logger::log("debug",
								"fileInput::clear(): clearing configuration.");

	setInputDir("");
	setArchiveDir("");
	setFormat("gpick");

	setShutdownWhenNoData(true);
	setShutdownWait(60);

	// finally do baseclass clear
	glass3::input::Input::clear();
}

std::string fileInput::fetchRawData(std::string* pOutType) {
	// our pOutType is our format (extension)
	*pOutType = getFormat();

	// check to see if we've got a file
	if ((m_InputFile.good() == true) && (m_InputFile.eof() != true)) {
		std::string line = "";

		// we're processing an input file, get the next line
		std::getline(m_InputFile, line);

		// skip empty lines
		while ((line.length() == 0) && (m_InputFile.eof() != true)) {
			// get the next line
			std::getline(m_InputFile, line);
		}

		// skip a timestamp line (gpick format)
		// timestamp format: 1425340828\n
		// so a line with less than or equal to 11 characters is a timestamp
		// (with newline)
		while ((line.length() <= 11) && (m_InputFile.eof() != true)) {
			// get the next line
			std::getline(m_InputFile, line);
		}

		// otherwise, return the line
		m_iDataCount++;
		return (line);
	} else {
		// need to get a new file
		// make sure we close an old file
		if (m_InputFile.is_open()) {
			// log some throughput statistics
			std::chrono::high_resolution_clock::time_point tFileEndTime =
					std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> tFileProcDuration =
					std::chrono::duration_cast<std::chrono::duration<double>>(
							tFileEndTime - m_tFileStartTime);
			double tAverageTime = tFileProcDuration.count() / m_iDataCount;

			glass3::util::Logger::log(
					"info",
					"fileInput::fetchRawData(): Processed "
							+ std::to_string(m_iDataCount) + " data from file: "
							+ m_sFileName + " in "
							+ std::to_string(tFileProcDuration.count())
							+ " seconds. (Average: "
							+ std::to_string(tAverageTime) + " seconds)");

			m_InputFile.close();

			// cleanup
			bool move = false;
			if (getArchiveDir() != "") {
				move = true;
			}
			// archive (or not) our fileInput
			cleanupFile(m_sFileName, move, getArchiveDir());
			m_sFileName = "";
		}

		// now look for a new file
		if (glass3::util::getFirstFileNameByExtension(getInputDir(),
														getFormat(),
														m_sFileName) == true) {
			// found one
			// open the file
			// next time we'll start reading from the file
			m_InputFile.open(m_sFileName, std::ios::in);

			glass3::util::Logger::log(
					"info",
					"fileInput::fetchRawData(): Opened file: " + m_sFileName);

			// reset performance counters
			m_tFileStartTime = std::chrono::high_resolution_clock::now();
			m_iDataCount = 0;
		} else {
			// no file to process, check to see if we still have data in the
			// queue and if we're supposed to autoshutdown
			if ((getInputDataCount() <= 0)
					&& (getShutdownWhenNoData() == true)) {
				// we don't
				glass3::util::Logger::log(
						"warning",
						"fileInput::fetchRawData(): No more input files and/or "
								"pending data in queue, shutting down in "
								+ std::to_string(getShutdownWait())
								+ " seconds.");

				// wait for glass to finish processing
				for (int i = 0; i < getShutdownWait(); i++) {
					// signal that we're still running
					setThreadHealth();

					// sleep for one second
					std::this_thread::sleep_for(
							std::chrono::milliseconds(1000));
				}

				// times up
				glass3::util::Logger::log(
						"warning", "fileInput::fetchRawData(): shutting down.");

				// shut it down
				setWorkThreadsState(glass3::util::ThreadState::Stopping);
			}
		}
	}

	// 'till next time
	return ("");
}

// file cleanup
void fileInput::cleanupFile(std::string filename, bool move,
							std::string destinationdir) {
	// if we are archiving our fileInput
	if (move == true) {
		// move the file to the archive dir
		glass3::util::moveFileTo(filename, destinationdir);
	} else {
		// delete the file
		if (!glass3::util::deleteFileFrom(filename)) {
			glass3::util::Logger::log(
					"error",
					"fileInput::archivefile(): Unable to delete file: "
							+ filename + ".");
		}
	}
}

const std::string fileInput::getInputDir() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sInputDir);
}

void fileInput::setInputDir(std::string dir) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sInputDir = dir;
}

const std::string fileInput::getArchiveDir() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sArchiveDir);
}

void fileInput::setArchiveDir(std::string dir) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sArchiveDir = dir;
}

const std::string fileInput::getFormat() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_sFormat);
}

void fileInput::setFormat(std::string format) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_sFormat = format;
}

void fileInput::setShutdownWhenNoData(bool shutdown) {
	m_bShutdownWhenNoData = shutdown;
}

bool fileInput::getShutdownWhenNoData() {
	return (m_bShutdownWhenNoData);
}

void fileInput::setShutdownWait(int waitTime) {
	m_iShutdownWait = waitTime;
}

int fileInput::getShutdownWait() {
	return (m_iShutdownWait);
}

}  // namespace glass
