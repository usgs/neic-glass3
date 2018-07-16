/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief gen-travel-times-app.cpp
 *
 * gen-travel-times-app is an application that uses the glasscore traveltime
 * libraries to generate the traveltime lookup files (.trv) used by neic-glass3
 * from a model file.
 *
 * gen-travel-times-app uses the environment variable GLASS_LOG to define
 * the location to write log files
 *
 * \par Usage
 * \parblock
 * <tt>gen-travel-times-app <configfile> [logname]</tt>
 *
 * \par Where
 * \parblock
 *    \b configfile is the required path to the configuration file for
 * gen-travel-times-app
 *
 *    \b logname is an optional string defining an alternate
 * name for the gen-travel-times-app log file.
 * \endparblock
 * \endparblock
 *
 * Please note that this application is currently not optimized, and is
 * extremely slow
 */
#include <project_version.h>
#include <json.h>
#include <logger.h>
#include <config.h>
#include <Logit.h>
#include <GenTrv.h>
#include <Terra.h>
#include <Ray.h>
#include <TTT.h>
#include <TravelTime.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

/**
 * \brief glassutil::CLogit call back function
 *
 * A function bound to glassutil::CLogit (via glassutil::CLogit::setLogCallback)
 * to enable logging messages to be written to gen-travel-times-app's log file
 *
 * \param message - a glassutil::logMessageStruct containg the message to log
 */
void logTravelTimes(glassutil::logMessageStruct message) {
	if (message.level == glassutil::log_level::info) {
		logger::log("info", "traveltime: " + message.message);
	} else if (message.level == glassutil::log_level::debug) {
		logger::log("debug", "traveltime: " + message.message);
	} else if (message.level == glassutil::log_level::warn) {
		logger::log("warning", "traveltime: " + message.message);
	} else if (message.level == glassutil::log_level::error) {
		logger::log("error", "traveltime: " + message.message);
	}
}

/**
 * \brief gen-travel-times-app main function
 *
 * \param  argc An integer argument count of the command line arguments
 * \param  argv An argument vector of the command line arguments
 * \return an integer 0 upon exit success, nonzero otherwise
 */
int main(int argc, char* argv[]) {
	// check our arguments
	if ((argc < 2) || (argc > 3)) {
		std::cout << "gen-travel-times-app version "
				<< std::to_string(PROJECT_VERSION_MAJOR) << "."
				<< std::to_string(PROJECT_VERSION_MINOR) << "."
				<< std::to_string(PROJECT_VERSION_PATCH) << "; Usage: "
				<< "gen-travel-times-app <configfile> [logname]" << std::endl;
		return 1;
	}

	// Look up our log directory
	std::string logpath;
	char* pLogDir = getenv("GLASS_LOG");
	if (pLogDir != NULL) {
		logpath = pLogDir;
	} else {
		std::cout << "gen-travel-times-app using default log directory of ./"
					<< std::endl;
		logpath = "./";
	}

	// get our logname if available
	std::string logName = "gen-travel-times-app";
	if (argc >= 3) {
		logName = std::string(argv[2]);
	}

	// now set up our logging
	logger::log_init(logName, spdlog::level::debug, logpath, true);

	logger::log(
			"info",
			"gen-travel-times-app: Version "
					+ std::to_string(PROJECT_VERSION_MAJOR) + "."
					+ std::to_string(PROJECT_VERSION_MINOR) + "."
					+ std::to_string(PROJECT_VERSION_PATCH) + " startup.");

	// get our config file location from the arguments
	std::string configFile = argv[1];

	logger::log("info",
				"gen-travel-times-app: using config file: " + configFile);

	// load our basic config
	glass3::util::Config * genConfig = new glass3::util::Config("", configFile);

	// check to see if our config is of the right format
	if (genConfig->getJSON().HasKey("Configuration")
			&& ((genConfig->getJSON())["Configuration"].GetType()
					== json::ValueType::StringVal)) {
		std::string configType = (genConfig->getJSON())["Configuration"]
				.ToString();

		if (configType != "gen-travel-times-app") {
			logger::log("critcal",
						"gen-travel-times-app: Wrong configuration, exiting.");

			delete (genConfig);
			return (1);
		}
	} else {
		// no command or type
		logger::log(
				"critcal",
				"gen-travel-times-app: Missing required Configuration Key.");

		delete (genConfig);
		return (1);
	}

	// model
	std::string model = "";
	if (genConfig->getJSON().HasKey("Model")
			&& ((genConfig->getJSON())["Model"].GetType()
					== json::ValueType::StringVal)) {
		model = (genConfig->getJSON())["Model"].ToString();
		logger::log("info",
								"gen-travel-times-app: Using Model: " + model);
	} else {
		logger::log("critical",
				"gen-travel-times-app: Missing required Model Key.");

		delete (genConfig);
		return (1);
	}

	// output path
	std::string path = "./";
	if (genConfig->getJSON().HasKey("OutputPath")
			&& ((genConfig->getJSON())["OutputPath"].GetType()
					== json::ValueType::StringVal)) {
		path = (genConfig->getJSON())["OutputPath"].ToString();
	}
	logger::log("info", "gen-travel-times-app: Using OutputPath: " + path);

	// file extension
	std::string extension = ".trv";
	if (genConfig->getJSON().HasKey("FileExtension")
			&& ((genConfig->getJSON())["FileExtension"].GetType()
					== json::ValueType::StringVal)) {
		extension = (genConfig->getJSON())["FileExtension"].ToString();
	}
	logger::log("info",
				"gen-travel-times-app: Using FileExtension: " + extension);

	logger::log("info", "gen-travel-times-app: Setup.");

	// create generator
	traveltime::CGenTrv *travelGenerator = new traveltime::CGenTrv();

	// set up traveltime to use our logging
	glassutil::CLogit::setLogCallback(
			std::bind(logTravelTimes, std::placeholders::_1));

	travelGenerator->setup(model, path, extension);

	logger::log("info", "gen-travel-times-app: Startup.");

	if (genConfig->getJSON().HasKey("Branches")
			&& ((genConfig->getJSON())["Branches"].GetType()
					== json::ValueType::ArrayVal)) {
		// get the array of phase entries
		json::Array branches = (genConfig->getJSON())["Branches"].ToArray();

		// for each branch in the array
		for (auto branchVal : branches) {
			// make sure the phase is an object
			if (branchVal.GetType() != json::ValueType::ObjectVal) {
				continue;
			}

			// get this branch object
			json::Object branchObj = branchVal.ToObject();

			if (travelGenerator->generate(&branchObj) != true) {
				logger::log(
						"error",
						"gen-travel-times-app: Failed to generate travel time "
						"file.");

				// cleanup
				delete (genConfig);
				delete (travelGenerator);
				return (1);
			}
		}
	}

	logger::log("info", "gen-travel-times-app: Shutdown.");

	// cleanup
	delete (genConfig);
	delete (travelGenerator);

	// done
	return (0);
}  // main()
