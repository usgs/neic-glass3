// gen-travel-times-app.cpp : Defines the entry point for the console
// application.
#include <json.h>
#include <logger.h>
#include <config.h>
#include <Logit.h>
#include <GenTrv.h>
#include <Terra.h>
#include <Ray.h>
#include <TTT.h>
#include <TravelTime.h>
#include "gen-travel-times-appCMakeConfig.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

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

int main(int argc, char* argv[]) {
	std::string configdir = "";
	bool isrunning = true;

	// check our arguments
	if ((argc < 2) || (argc > 3)) {
		std::cout << "gen-travel-times-app version "
				<< std::to_string(GEN_TRAVELTIMES_VERSION_MAJOR) << "."
				<< std::to_string(GEN_TRAVELTIMES_VERSION_MINOR) << "; Usage: "
				<< "gen-travel-times-app <configfile> [logname]" << std::endl;
		return 1;
	}

	// Look up our log directory
	std::string logpath;
	char* pLogDir = getenv("GLASS_LOG");
	if (pLogDir != NULL) {
		logpath = pLogDir;
	} else {
		std::cout << "gen-travel-times-app using default log director of ./"
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
					+ std::to_string(GEN_TRAVELTIMES_VERSION_MAJOR) + "."
					+ std::to_string(GEN_TRAVELTIMES_VERSION_MINOR) + "."
					+ std::to_string(GEN_TRAVELTIMES_VERSION_PATCH)
					+ " startup.");

	// get our config file location from the arguments
	std::string configFile = argv[1];

	logger::log("info",
				"gen-travel-times-app: using config file: " + configFile);

	// load our basic config
	util::Config * genConfig = new util::Config("", configFile);

	// check to see if our config is of the right format
	if (genConfig->getConfigJSON().HasKey("Configuration")
			&& ((genConfig->getConfigJSON())["Configuration"].GetType()
					== json::ValueType::StringVal)) {
		std::string configType = (genConfig->getConfigJSON())["Configuration"]
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
	if (genConfig->getConfigJSON().HasKey("Model")
			&& ((genConfig->getConfigJSON())["Model"].GetType()
					== json::ValueType::StringVal)) {
		model = (genConfig->getConfigJSON())["Model"].ToString();
		glassutil::CLogit::log(glassutil::log_level::info,
								"gen-travel-times-app: Using Model: " + model);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"gen-travel-times-app: Missing required Model Key.");

		delete (genConfig);
		return (1);
	}

	// output path
	std::string path = "./";
	if (genConfig->getConfigJSON().HasKey("OutputPath")
			&& ((genConfig->getConfigJSON())["OutputPath"].GetType()
					== json::ValueType::StringVal)) {
		path = (genConfig->getConfigJSON())["OutputPath"].ToString();
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"gen-travel-times-app: Using OutputPath: " + path);
	}

	// file extension
	std::string extension = ".trv";
	if (genConfig->getConfigJSON().HasKey("FileExtension")
			&& ((genConfig->getConfigJSON())["FileExtension"].GetType()
					== json::ValueType::StringVal)) {
		extension = (genConfig->getConfigJSON())["FileExtension"].ToString();
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"gen-travel-times-app: Using FileExtension: " + extension);
	}

	logger::log("info", "gen-travel-times-app: Setup.");

	// create generator
	traveltime::CGenTrv *travelGenerator = new traveltime::CGenTrv();

	// set up traveltime to use our logging
	glassutil::CLogit::setLogCallback(
			std::bind(logTravelTimes, std::placeholders::_1));

	travelGenerator->setup(model, path, extension);

	logger::log("info", "gen-travel-times-app: Startup.");

	if (genConfig->getConfigJSON().HasKey("Branches")
			&& ((genConfig->getConfigJSON())["Branches"].GetType()
					== json::ValueType::ArrayVal)) {
		// get the array of phase entries
		json::Array branches =
				(genConfig->getConfigJSON())["Branches"].ToArray();

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
}
