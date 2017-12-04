#include <input.h>
#include <json.h>
#include <logger.h>
#include <fileutil.h>
#include <timeutil.h>

#include <vector>
#include <queue>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <string>

namespace glass {
// Construction/Destruction
input::input()
		: util::ThreadBaseClass("input", 100) {
	logger::log("debug", "input::input(): Construction.");

	m_iInFileSleep = 10;

	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = NULL;

	clear();
}

input::input(int linesleepms)
		: util::ThreadBaseClass("input", 100) {
	m_iInFileSleep = linesleepms;

	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = NULL;

	clear();
}

input::input(json::Object *config, int linesleepms)
		: util::ThreadBaseClass("input", 100) {
	m_iInFileSleep = linesleepms;

	m_GPickParser = NULL;
	m_JSONParser = NULL;
	m_CCParser = NULL;
	m_DataQueue = NULL;

	// do basic construction
	clear();

	// configure ourselves
	setup(config);

	// start up the input thread
	start();
}

input::~input() {
	logger::log("debug", "input::~input(): Destruction.");

	// stop the input thread
	stop();

	if (m_DataQueue != NULL) {
		// clear the queue
		m_DataQueue->clearQueue();

		delete (m_DataQueue);
	}

	if (m_GPickParser != NULL)
		delete (m_GPickParser);

	if (m_JSONParser != NULL)
		delete (m_JSONParser);

	if (m_CCParser != NULL)
		delete (m_CCParser);
}

// configuration
bool input::setup(json::Object *config) {
	if (config == NULL) {
		logger::log("error", "input::setup(): NULL configuration passed in.");
		return (false);
	}

	logger::log("debug", "input::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		logger::log("error", "input::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"].ToString();
		if (configtype != "GlassInput") {
			logger::log(
					"error",
					"input::setup(): Wrong configuration provided, configuration "
							"is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	// inputdir
	if (!(config->HasKey("InputDirectory"))) {
		// inputdir is required
		m_sInputDir = "";
		logger::log(
				"error",
				"input::setup(): Required configuration value InputDirectory not "
				"specified.");
		return (false);
	} else {
		m_sInputDir = (*config)["InputDirectory"].ToString();
		logger::log(
				"info",
				"input::setup(): Using Input Directory: " + m_sInputDir + ".");
	}

	// formats
	if (!(config->HasKey("Formats"))) {
		// formats is required
		m_Formats.Clear();
		logger::log("error",
					"input::setup(): No formats specified in configuration.");
		return (false);
	} else {
		if ((*config)["Formats"].size() <= 0) {
			logger::log(
					"error",
					"input::setup(): No formats specified in configuration.");
			return (false);
		}
		m_Formats = (*config)["Formats"].ToArray();
	}

	// archive input?
	if (!(config->HasKey("ArchiveDirectory"))) {
		// archive is optional
		m_bArchive = false;
		m_sArchiveDir = "";
		logger::log("info", "input::setup(): Not Archiving input.");
	} else {
		m_bArchive = true;
		m_sArchiveDir = (*config)["ArchiveDirectory"].ToString();
		logger::log(
				"info",
				"input::setup(): Using Archive Directory: " + m_sArchiveDir
						+ " to archive input.");
	}

	// keep errors?
	if (!(config->HasKey("ErrorDirectory"))) {
		// error is optional
		m_bError = false;
		m_sErrorDir = "";
		logger::log("info", "input::setup(): Not keeping erronious input.");
	} else {
		m_bError = true;
		m_sErrorDir = (*config)["ErrorDirectory"].ToString();
		logger::log(
				"info",
				"input::setup(): Using Error Directory: " + m_sErrorDir
						+ " to keep erronious input.");
	}

	// default agencyid
	if (!(config->HasKey("DefaultAgencyID"))) {
		// agencyid is optional
		m_sDefaultAgencyID = "US";
		logger::log("info", "input::setup(): Defaulting to US as AgencyID.");
	} else {
		m_sDefaultAgencyID = (*config)["DefaultAgencyID"].ToString();
		logger::log(
				"info",
				"input::setup(): Using AgencyID: " + m_sDefaultAgencyID
						+ " as default.");
	}

	// default author
	if (!(config->HasKey("DefaultAuthor"))) {
		// agencyid is optional
		m_sDefaultAuthor = "glassConverter";
		logger::log("info",
					"input::setup(): Defaulting to glassConverter as Author.");
	} else {
		m_sDefaultAuthor = (*config)["DefaultAuthor"].ToString();
		logger::log(
				"info",
				"input::setup(): Using Author: " + m_sDefaultAuthor
						+ " as default.");
	}

	// queue max size
	if (!(config->HasKey("QueueMaxSize"))) {
		// agencyid is optional
		m_QueueMaxSize = -1;
		logger::log(
				"info",
				"input::setup(): Defaulting to -1 for QueueMaxSize (no maximum "
				"queue size).");
	} else {
		m_QueueMaxSize = (*config)["QueueMaxSize"].ToInt();
		logger::log(
				"info",
				"input::setup(): Using QueueMaxSize: "
						+ std::to_string(m_QueueMaxSize) + ".");
	}

	// shutdown when no data
	if (!(config->HasKey("ShutdownWhenNoData"))) {
		// m_bShutdownWhenNoData is optional
		m_bShutdownWhenNoData = true;
		logger::log(
				"info",
				"input::setup(): Defaulting to true for ShutdownWhenNoData");
	} else {
		m_bShutdownWhenNoData = (*config)["ShutdownWhenNoData"].ToBool();
		logger::log(
				"info",
				"input::setup(): Using ShutdownWhenNoData: = "
						+ std::to_string(m_bShutdownWhenNoData));
	}

	// shutdown wait when no data
	if (!(config->HasKey("ShutdownWait"))) {
		// m_iShutdownWait is optional
		m_iShutdownWait = 60;
		logger::log("info",
					"input::setup(): Defaulting to 30 for ShutdownWait");
	} else {
		m_iShutdownWait = (*config)["ShutdownWait"].ToInt();
		logger::log("info", "input::setup(): Using ShutdownWait: "
						+ std::to_string(m_iShutdownWait));
	}

	// unlock our configuration
	m_ConfigMutex.unlock();

	if (m_GPickParser != NULL)
		delete (m_GPickParser);
	m_GPickParser = new parse::GPickParser(m_sDefaultAgencyID,
											m_sDefaultAuthor);

	if (m_JSONParser != NULL)
		delete (m_JSONParser);
	m_JSONParser = new parse::JSONParser(m_sDefaultAgencyID, m_sDefaultAuthor);

	if (m_CCParser != NULL)
		delete (m_CCParser);
	m_CCParser = new parse::CCParser(m_sDefaultAgencyID, m_sDefaultAuthor);

	if (m_DataQueue != NULL)
		delete (m_DataQueue);
	m_DataQueue = new util::Queue();

	logger::log("debug", "input::setup(): Done Setting Up.");

	// finally do baseclass setup;
	// mostly remembering our config object
	util::BaseClass::setup(config);

	// we're done
	return (true);
}

void input::clear() {
	logger::log("debug", "input::clear(): clearing configuration.");

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	m_sInputDir = "";
	m_bArchive = false;
	m_sArchiveDir = "";
	m_bError = false;
	m_sErrorDir = "";
	m_Formats.Clear();
	m_sDefaultAgencyID = "";
	m_sDefaultAuthor = "";
	m_QueueMaxSize = -1;
	m_bShutdownWhenNoData = true;
	m_iShutdownWait = 60;

	// unlock our configuration
	m_ConfigMutex.unlock();

	if (m_DataQueue != NULL)
		m_DataQueue->clearQueue();

	// finally do baseclass clear
	util::BaseClass::clear();
}

// get next data from input
json::Object* input::getData() {
	// just get the value from the queue
	return (m_DataQueue->getDataFromQueue());
}

int input::dataCount() {
	if (m_DataQueue == NULL) {
		return (-1);
	}

	return (m_DataQueue->size());
}

bool input::work() {
	// pull data from our config at the start of each loop
	// so that we can have config that changes
	// should I do this?
	m_ConfigMutex.lock();
	std::string inputdir = m_sInputDir;
	json::Array formats = m_Formats;
	bool archive = m_bArchive;
	std::string archivedir = m_sArchiveDir;
	bool error = m_bError;
	std::string errordir = m_sErrorDir;
	m_ConfigMutex.unlock();

	// make sure we have formats
	if (formats.size() == 0)
		return (true);

	bool foundFile = false;

	// run through all the formats
	// checking to see if we have any files
	for (int i = 0; i < formats.size(); i++) {
		std::string extension = formats[i];
		if (readFiles(extension, inputdir, archive, archivedir, error, errordir)
				== true) {
			foundFile = true;
		} else {
			foundFile = false;
		}
	}

	// don't shutdown if we're not allowed to
	if (m_bShutdownWhenNoData == false) {
		return (true);
	}

	// work was successful, found a file to process, keep going
	if (foundFile == true) {
		return (true);
	} else {
		// no file to process, check to see if we still have data in the queue
		if (dataCount() <= 0) {
			logger::log(
					"warning",
					"input::work(): No more input files and/or "
							"pending data in queue, shutting down after "
							+ std::to_string(m_iShutdownWait) + " seconds.");

			for (int i = 0; i < m_iShutdownWait; i++) {
				// signal that we're still running
				setWorkCheck();

				// sleep for one second
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

			// we don't, we have nothing to do now, return false
			// so we exit
			return (false);
		} else {
			// we do, don't exit
			return (true);
		}
	}
}

bool input::readFiles(std::string extension, const std::string &inputdir,
						bool archive, const std::string &archivedir, bool error,
						const std::string &errordir) {
	std::string filename = "";

	// look for files with json picks
	if (util::getNextFileName(inputdir, extension, filename) == true) {
		logger::log("debug",
					"input::readfiles(): Processing  file: " + filename + " .");

		// this file contains a large number of data in a supported format.
		// format is:
		// <optional timestamp>\n
		// <data>\n
		// ...
		std::chrono::high_resolution_clock::time_point tFileStartTime =
				std::chrono::high_resolution_clock::now();
		int datacount = 0;
		std::string line;

		// open the file
		std::ifstream infile;
		infile.open(filename, std::ios::in);

		// while infile is valid
		while (infile) {
			// make sure we've not been told to stop
			if (isRunning() == false) {
				infile.close();
				logger::log(
						"warning",
						"input::readfiles(): Shutdown detected while in loop "
						"that was parsing file.");
				return (false);
			}

			// signal that we're still running
			setWorkCheck();

			// check to see if we have room
			if ((m_QueueMaxSize != -1)
					&& (m_DataQueue->size() > m_QueueMaxSize)) {
				// we don't, yet
				continue;
			}

			// each line is potential
			std::getline(infile, line);

			// skip an empty line
			if (line.length() == 0)
				continue;

			// skip a timestamp line
			// timestamp format: 1425340828\n
			// so a line with less than or equal to 11 characters is a timestamp
			// (with newline)
			if (line.length() <= 11)
				continue;

			// parse the line
			json::Object * newdata = parse(extension, line);

			// validate the data
			if (validate(extension, newdata) == true) {
				m_DataQueue->addDataToQueue(newdata);
				datacount++;

				std::this_thread::sleep_for(
						std::chrono::milliseconds(m_iInFileSleep));
			} else {
				infile.close();
				logger::log(
						"error",
						"input::readfiles(): Failed to parse data from file: "
								+ filename + " .");

				// keep (or not) our errors
				cleanupFile(filename, error, errordir);

				return (true);
			}
		}

		infile.close();

		std::chrono::high_resolution_clock::time_point tFileEndTime =
				std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tFileProcDuration =
				std::chrono::duration_cast<std::chrono::duration<double>>(
						tFileEndTime - tFileStartTime);
		double tAverageTime = tFileProcDuration.count() / datacount;
		logger::log(
				"debug",
				"input::readfiles(): Processed " + std::to_string(datacount)
						+ " data from file: " + filename + " in "
						+ std::to_string(tFileProcDuration.count())
						+ " seconds. (Average: " + std::to_string(tAverageTime)
						+ " seconds)");

		// archive (or not) our input
		cleanupFile(filename, archive, archivedir);

		return (true);
	} else {
		return (false);
	}
}

// parse a json object from an input string
json::Object* input::parse(std::string extension, std::string input) {
	// choose the parser based on the extension
	// global pick
	if (((extension == GPICK_EXTENSION) || (extension == GPICKS_EXTENSION))
			&& (m_GPickParser != NULL))
		return (m_GPickParser->parse(input));
	// all json formats share the same parser
	else if ((extension.find(JSON_EXTENSION) != std::string::npos)
			&& (m_JSONParser != NULL))
		return (m_JSONParser->parse(input));
	// cc data
	else if ((extension == CC_EXTENSION) && (m_CCParser != NULL))
		return (m_CCParser->parse(input));
	else
		return (NULL);
}

// validate a json object
bool input::validate(std::string extension, json::Object* input) {
	// choose the validator based on the extension
	// global pick
	if (((extension == GPICK_EXTENSION) || (extension == GPICKS_EXTENSION))
			&& (m_GPickParser != NULL))
		return (m_GPickParser->validate(input));
	// all json formats share the same validator
	else if ((extension.find(JSON_EXTENSION) != std::string::npos)
			&& (m_JSONParser != NULL))
		return (m_JSONParser->validate(input));
	// cc data
	else if ((extension == CC_EXTENSION) && (m_CCParser != NULL))
		return (m_CCParser->validate(input));
	else
		return (false);
}

// file cleanup
void input::cleanupFile(std::string filename, bool move,
						std::string destinationdir) {
	// if we are archiving our input
	if (move == true) {
		// move the file to the archive dir
		util::moveFileTo(filename, destinationdir);
	} else {
		// delete the file
		if (!util::deleteFileFrom(filename)) {
			logger::log(
					"error",
					"input::archivefile(): Unable to delete file: " + filename
							+ ".");
		}
	}
}
}  // namespace glass
