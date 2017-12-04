// glass-broker-app.cpp : Defines the entry point for the console application.
//
#include <json.h>
#include <logger.h>
#include <config.h>
#include <input.h>
#include <associator.h>
#include "glass-broker-appCMakeConfig.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "broker_output/brokerOutput.h"

int main(int argc, char* argv[]) {
	std::string configdir = "";
	bool isrunning = true;

	// check our arguments
	if ((argc < 2) || (argc > 4)) {
		std::cout << "glass-broker-app version "
					<< std::to_string(GLASS_VERSION_MAJOR) << "."
					<< std::to_string(GLASS_VERSION_MINOR) << "; Usage: "
					<< "glass-broker-app <configfile> [logname] [noconsole]"
					<< std::endl;
		return 1;
	}

	// Look up our log directory
	std::string logpath;
	char* pLogDir = getenv("GLASS_LOG");
	if (pLogDir != NULL) {
		logpath = pLogDir;
	} else {
		std::cout << "glass-broker-app using default log directory of ./"
					<< std::endl;
		logpath = "./";
	}

	// get our logname if available
	std::string logName = "glass-broker-app";
	bool logConsole = true;
	if (argc >= 3) {
		std::string temp = std::string(argv[2]);

		if (temp == "noconsole") {
			logConsole = false;
		} else {
			logName = std::string(argv[2]);
		}
	}

	// get whether we log to console
	if (argc == 4) {
		std::string temp = std::string(argv[3]);

		if (temp == "noconsole") {
			logConsole = false;
		}
	}

	// now set up our logging
	logger::log_init(logName, spdlog::level::info, logpath, logConsole);

	logger::log(
			"info",
			"glass-broker-app: Glass Version "
					+ std::to_string(GLASS_VERSION_MAJOR) + "."
					+ std::to_string(GLASS_VERSION_MINOR) + "."
					+ std::to_string(GLASS_VERSION_PATCH) + " startup.");

	// get our config file location from the arguments
	std::string configFile = argv[1];

	logger::log("info", "glass-broker-app: using config file: " + configFile);

	// load our basic config
	util::Config * glassConfig = new util::Config("", configFile);

	// check to see if our config is of the right format
	if (glassConfig->getConfigJSON().HasKey("Configuration")
			&& ((glassConfig->getConfigJSON())["Configuration"].GetType()
					== json::ValueType::StringVal)) {
		std::string configType = (glassConfig->getConfigJSON())["Configuration"]
				.ToString();

		if (configType != "glass-broker-app") {
			logger::log("critcal",
						"glass-broker-app: Wrong configuration, exiting.");

			delete (glassConfig);
			return (0);
		}
	} else {
		// no command or type
		logger::log("critcal",
					"glass-broker-app: Missing required Configuration Key.");

		delete (glassConfig);
		return (0);
	}

	// get the directory where the rest of the glass configs are stored
	if (glassConfig->getConfigJSON().HasKey("ConfigDirectory")
			&& ((glassConfig->getConfigJSON())["ConfigDirectory"].GetType()
					== json::ValueType::StringVal)) {
		configdir =
				(glassConfig->getConfigJSON())["ConfigDirectory"].ToString();
		logger::log("info", "Reading glass configurations from: " + configdir);
	} else {
		configdir = "./";
		logger::log(
				"warning",
				"missing <ConfigDirectory>, defaulting to local directory.");
	}

	// set our proper loglevel
	if (glassConfig->getConfigJSON().HasKey("LogLevel")
			&& ((glassConfig->getConfigJSON())["LogLevel"].GetType()
					== json::ValueType::StringVal)) {
		logger::log_update_level((glassConfig->getConfigJSON())["LogLevel"]);
	}

	// get initialize config file location
	std::string initconfigfile;
	if (glassConfig->getConfigJSON().HasKey("InitializeFile")
			&& ((glassConfig->getConfigJSON())["InitializeFile"].GetType()
					== json::ValueType::StringVal)) {
		initconfigfile = (glassConfig->getConfigJSON())["InitializeFile"]
				.ToString();
	} else {
		logger::log(
				"critcal",
				"Invalid configuration, missing <InitializeFile>, exiting.");

		delete (glassConfig);
		return (0);
	}

	// load our initialize config
	util::Config * InitializeConfig = new util::Config(configdir,
														initconfigfile);
	json::Object * InitializeJSON = new json::Object(
			InitializeConfig->getConfigJSON());

	// get stationlist file location
	std::string stationlistfile;
	if (glassConfig->getConfigJSON().HasKey("StationList")
			&& ((glassConfig->getConfigJSON())["StationList"].GetType()
					== json::ValueType::StringVal)) {
		stationlistfile =
				(glassConfig->getConfigJSON())["StationList"].ToString();
	} else {
		logger::log("critcal",
					"Invalid configuration, missing <StationList>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		return (1);
	}

	// Load our initial stationlist
	util::Config * StationList = new util::Config(configdir, stationlistfile);
	json::Object * StationListJSON = new json::Object(
			StationList->getConfigJSON());

	// get detection grid file list
	json::Array gridconfigfilelist;
	if (glassConfig->getConfigJSON().HasKey("GridFiles")
			&& ((glassConfig->getConfigJSON())["GridFiles"].GetType()
					== json::ValueType::ArrayVal)) {
		gridconfigfilelist = (glassConfig->getConfigJSON())["GridFiles"];
	} else {
		logger::log("critcal",
					"Invalid configuration, missing <GridFiles>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		return (1);
	}

	// check to see if any files were in the array
	if (gridconfigfilelist.size() == 0) {
		logger::log("critcal", "No <GridFiles> specified, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		return (1);
	}

	// get input config file location
	std::string inputconfigfile;
	if (glassConfig->getConfigJSON().HasKey("InputConfig")
			&& ((glassConfig->getConfigJSON())["InputConfig"].GetType()
					== json::ValueType::StringVal)) {
		inputconfigfile =
				(glassConfig->getConfigJSON())["InputConfig"].ToString();
	} else {
		logger::log("critcal",
					"Invalid configuration, missing <InputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		return (1);
	}

	// load our input config
	util::Config * InputConfig = new util::Config(configdir, inputconfigfile);

	// get output config file location
	std::string outputconfigfile;
	if (glassConfig->getConfigJSON().HasKey("OutputConfig")
			&& ((glassConfig->getConfigJSON())["OutputConfig"].GetType()
					== json::ValueType::StringVal)) {
		outputconfigfile = (glassConfig->getConfigJSON())["OutputConfig"]
				.ToString();
	} else {
		logger::log("critcal",
					"Invalid configuration, missing <OutputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (InputConfig);
		return (1);
	}

	// load our output config
	util::Config * OutputConfig = new util::Config(configdir, outputconfigfile);

	// create our objects
	glass::input * InputThread = new glass::input(5);
	glass::brokerOutput * OutputThread = new glass::brokerOutput();
	glass::Associator * AssocThread = new glass::Associator();

	// setup input thread
	json::Object * input_config_json = new json::Object(
			InputConfig->getConfigJSON());
	if (InputThread->setup(input_config_json) != true) {
		logger::log("critical", "glass: Failed to setup Input.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (OutputConfig);
		delete (InputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// setup output thread
	json::Object * output_config_json = new json::Object(
			OutputConfig->getConfigJSON());
	if (OutputThread->setup(output_config_json) != true) {
		logger::log("critical", "glass: Failed to setup Output.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (OutputConfig);
		delete (InputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// output needs to know about the associator thread to request
	// information
	OutputThread->Associator = AssocThread;

	// assoc thread needs to know about the input and output threads
	AssocThread->Input = InputThread;
	AssocThread->Output = OutputThread;

	// configure assoc thread (glass)
	// first send in initialize
	AssocThread->setup(InitializeJSON);

	// next send in stationlist
	AssocThread->setup(StationListJSON);

	// finally load and send in grids
	for (int i = 0; i < gridconfigfilelist.size(); i++) {
		std::string gridconfigfile = gridconfigfilelist[i];
		if (gridconfigfile != "") {
			util::Config * GridConfig = new util::Config(configdir,
															gridconfigfile);
			json::Object * GridConfigJSON = new json::Object(
					GridConfig->getConfigJSON());

			// send in grid
			AssocThread->setup(GridConfigJSON);

			// done with grid config and json
			delete (GridConfig);
			delete (GridConfigJSON);
		}
	}

	// startup threads
	InputThread->start();
	OutputThread->start();
	AssocThread->start();

	logger::log("info", "glass: glass is running.");

	// run forever
	while (true) {
		logger::log("trace", "glass: Checking thread status.");

		if (InputThread->check() == false) {
			logger::log("error", "glass: Input thread has exited!!");
			isrunning = false;
			break;
		} else if (OutputThread->check() == false) {
			logger::log("error", "glass: Output thread has exited!!");
			isrunning = false;
			break;
		} else if (AssocThread->check() == false) {
			logger::log("error", "glass: Association thread has exited!!");
			isrunning = false;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}

	logger::log("info", "glass: glass is shutting down.");

	// shutdown
	InputThread->stop();
	OutputThread->stop();
	AssocThread->stop();

	// cleanup
	delete (AssocThread);
	delete (InitializeConfig);
	delete (InputThread);
	delete (InputConfig);
	delete (input_config_json);
	delete (OutputThread);
	delete (OutputConfig);
	delete (output_config_json);
	delete (StationList);
	delete (StationListJSON);
	delete (glassConfig);

	return (0);
}
