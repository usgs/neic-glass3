// glass-broker-app.cpp : Defines the entry point for the console application.
//
#include <project_version.h>
#include <json.h>
#include <logger.h>
#include <config.h>
#include <input.h>
#include <associator.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

#include "broker_input/broker_input.h"
#include "broker_output/brokerOutput.h"

int main(int argc, char* argv[]) {
	std::string configdir = "";

	// check our arguments
	if ((argc < 2) || (argc > 4)) {
		std::cout << "glass-broker-app version "
					<< std::to_string(PROJECT_VERSION_MAJOR) << "."
					<< std::to_string(PROJECT_VERSION_MINOR) << "."
					<< std::to_string(PROJECT_VERSION_PATCH) << "; Usage: "
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
	glass3::util::log_init(logName, spdlog::level::info, logpath, logConsole);

	glass3::util::log(
			"info",
			"glass-broker-app: Glass Version "
					+ std::to_string(PROJECT_VERSION_MAJOR) + "."
					+ std::to_string(PROJECT_VERSION_MINOR) + "."
					+ std::to_string(PROJECT_VERSION_PATCH) + " startup.");

	// get our config file location from the arguments
	std::string configFile = argv[1];

	glass3::util::log("info", "glass-broker-app: using config file: " + configFile);

	// load our basic config
	glass3::util::Config * glassConfig = new glass3::util::Config("",
																	configFile);

	// check to see if our config is of the right format
	if (glassConfig->getJSON()->HasKey("Configuration")
			&& ((*glassConfig->getJSON())["Configuration"].GetType()
					== json::ValueType::StringVal)) {
		std::string configType = (*glassConfig->getJSON())["Configuration"]
				.ToString();

		if (configType != "glass-broker-app") {
			glass3::util::log("critcal",
						"glass-broker-app: Wrong configuration, exiting.");

			delete (glassConfig);
			return (0);
		}
	} else {
		// no command or type
		glass3::util::log("critcal",
					"glass-broker-app: Missing required Configuration Key.");

		delete (glassConfig);
		return (0);
	}

	// get the directory where the rest of the glass configs are stored
	if (glassConfig->getJSON()->HasKey("ConfigDirectory")
			&& ((*glassConfig->getJSON())["ConfigDirectory"].GetType()
					== json::ValueType::StringVal)) {
		configdir = (*glassConfig->getJSON())["ConfigDirectory"].ToString();
		glass3::util::log("info", "Reading glass configurations from: " + configdir);
	} else {
		configdir = "./";
		glass3::util::log(
				"warning",
				"missing <ConfigDirectory>, defaulting to local directory.");
	}

	// set our proper loglevel
	if (glassConfig->getJSON()->HasKey("LogLevel")
			&& ((*glassConfig->getJSON())["LogLevel"].GetType()
					== json::ValueType::StringVal)) {
		glass3::util::log_update_level((*glassConfig->getJSON())["LogLevel"]);
	}

	// get initialize config file location
	std::string initconfigfile;
	if (glassConfig->getJSON()->HasKey("InitializeFile")
			&& ((*glassConfig->getJSON())["InitializeFile"].GetType()
					== json::ValueType::StringVal)) {
		initconfigfile = (*glassConfig->getJSON())["InitializeFile"].ToString();
	} else {
		glass3::util::log(
				"critcal",
				"Invalid configuration, missing <InitializeFile>, exiting.");

		delete (glassConfig);
		return (1);
	}

	// load our initialize config
	glass3::util::Config * InitializeConfig = new glass3::util::Config(
			configdir, initconfigfile);
	std::shared_ptr<const json::Object> InitializeJSON = InitializeConfig
			->getJSON();

	// get stationlist file location
	std::string stationlistfile;
	if (glassConfig->getJSON()->HasKey("StationList")
			&& ((*glassConfig->getJSON())["StationList"].GetType()
					== json::ValueType::StringVal)) {
		stationlistfile = (*glassConfig->getJSON())["StationList"].ToString();
	} else {
		glass3::util::log("critcal",
					"Invalid configuration, missing <StationList>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		return (1);
	}

	// Load our initial stationlist
	glass3::util::Config * StationList = new glass3::util::Config(
			configdir, stationlistfile);
	std::shared_ptr<const json::Object> StationListJSON =
			StationList->getJSON();

	// get detection grid file list
	json::Array gridconfigfilelist;
	if (glassConfig->getJSON()->HasKey("GridFiles")
			&& ((*glassConfig->getJSON())["GridFiles"].GetType()
					== json::ValueType::ArrayVal)) {
		gridconfigfilelist = (*glassConfig->getJSON())["GridFiles"];
	} else {
		glass3::util::log("critcal",
					"Invalid configuration, missing <GridFiles>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	// check to see if any files were in the array
	if (gridconfigfilelist.size() == 0) {
		glass3::util::log("critcal", "No <GridFiles> specified, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	// get input config file location
	std::string inputconfigfile;
	if (glassConfig->getJSON()->HasKey("InputConfig")
			&& ((*glassConfig->getJSON())["InputConfig"].GetType()
					== json::ValueType::StringVal)) {
		inputconfigfile = (*glassConfig->getJSON())["InputConfig"].ToString();
	} else {
		glass3::util::log("critcal",
					"Invalid configuration, missing <InputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	// load our input config
	glass3::util::Config * InputConfig = new glass3::util::Config(
			configdir, inputconfigfile);

	// get output config file location
	std::string outputconfigfile;
	if (glassConfig->getJSON()->HasKey("OutputConfig")
			&& ((*glassConfig->getJSON())["OutputConfig"].GetType()
					== json::ValueType::StringVal)) {
		outputconfigfile = (*glassConfig->getJSON())["OutputConfig"].ToString();
	} else {
		glass3::util::log("critcal",
					"Invalid configuration, missing <OutputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		return (1);
	}

	// load our output config
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdir, outputconfigfile);

	// create our objects
	glass::brokerInput * InputThread = new glass::brokerInput();
	glass::brokerOutput * OutputThread = new glass::brokerOutput();
	glass::Associator * AssocThread = new glass::Associator();

	// setup input thread
	std::shared_ptr<const json::Object> input_config_json =
			InputConfig->getJSON();
	if (InputThread->setup(input_config_json) != true) {
		glass3::util::log("critical", "glass: Failed to setup Input.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (OutputConfig);
		delete (InputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// setup output thread
	std::shared_ptr<const json::Object> output_config_json = OutputConfig
			->getJSON();
	if (OutputThread->setup(output_config_json) != true) {
		glass3::util::log("critical", "glass: Failed to setup Output.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (OutputConfig);
		delete (InputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// output needs to know about the associator thread to request
	// information
	OutputThread->setAssociator(AssocThread);

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
			glass3::util::Config * GridConfig = new glass3::util::Config(
					configdir, gridconfigfile);
			std::shared_ptr<const json::Object> GridConfigJSON = GridConfig
					->getJSON();

			// send in grid
			AssocThread->setup(GridConfigJSON);

			// done with grid config and json
			delete (GridConfig);
		}
	}

	// startup threads
	InputThread->start();
	OutputThread->start();
	AssocThread->start();

	glass3::util::log("info", "glass: glass is running.");

	// run forever
	while (true) {
		glass3::util::log("trace", "glass: Checking thread status.");

		if (InputThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Input thread has exited!!");
			break;
		} else if (OutputThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Output thread has exited!!");
			break;
		} else if (AssocThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Association thread has exited!!");
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}

	glass3::util::log("info", "glass: glass is shutting down.");

	// shutdown
	InputThread->stop();
	OutputThread->stop();
	AssocThread->stop();

	// cleanup
	delete (AssocThread);
	delete (InitializeConfig);
	delete (InputThread);
	delete (InputConfig);
	delete (OutputThread);
	delete (OutputConfig);
	delete (StationList);
	delete (glassConfig);

	return (0);
}
