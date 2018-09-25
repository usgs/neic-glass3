// glass-app.cpp : Defines the entry point for the console application.
#include <project_version.h>
#include <json.h>
#include <logger.h>
#include <config.h>
#include <file_input.h>
#include <file_output.h>
#include <associator.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

int main(int argc, char* argv[]) {
	std::string configdir = "";

	// check our arguments
	if ((argc < 2) || (argc > 3)) {
		std::cout << "glass-app version "
					<< std::to_string(PROJECT_VERSION_MAJOR) << "."
					<< std::to_string(PROJECT_VERSION_MINOR) << "."
					<< std::to_string(PROJECT_VERSION_PATCH) << "; Usage: "
					<< "glass-app <configfile>  [logname] [noconsole]"
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
	std::string logName = "glass-app";
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
	glass3::util::Logger::log_init(logName, "info", logpath, logConsole);

	glass3::util::Logger::log(
			"info",
			"glass-app: neic-glass3 Version "
					+ std::to_string(PROJECT_VERSION_MAJOR) + "."
					+ std::to_string(PROJECT_VERSION_MINOR) + "."
					+ std::to_string(PROJECT_VERSION_PATCH) + " startup.");

	glass3::util::Logger::log(
			"info",
			"glass-app: loading configuration file " + std::string(argv[1]));

	// now load our basic config from file
	// note, main is gonna get cluttered, move this section
	// to it's own function eventually
	glass3::util::Config glassConfig;

	try {
		glassConfig.parseJSONFromFile("", std::string(argv[1]));
	} catch (std::exception& e) {
		glass3::util::Logger::log(
				"criticalerror",
				"Failed to load file: " + std::string(argv[1]) + "; "
						+ std::string(e.what()));

		return (1);
	}

	// check to see if our config is of the right format
	if (glassConfig.getJSON()->HasKey("Cmd")
			&& ((*glassConfig.getJSON())["Cmd"].GetType()
					== json::ValueType::StringVal)) {
		std::string configType = (*glassConfig.getJSON())["Cmd"].ToString();

		if (configType != "Glass") {
			glass3::util::Logger::log(
					"critical", "glass-app: Wrong configuration, exiting.");

			return (0);
		}
	} else {
		// no command
		glass3::util::Logger::log("critical",
									"glass-app: Missing required Cmd Key.");

		return (0);
	}

	// get the directory where the rest of the glass configs are stored
	if (glassConfig.getJSON()->HasKey("ConfigDirectory")
			&& ((*glassConfig.getJSON())["ConfigDirectory"].GetType()
					== json::ValueType::StringVal)) {
		configdir = (*glassConfig.getJSON())["ConfigDirectory"].ToString();
		glass3::util::Logger::log(
				"info", "Reading glass configurations from: " + configdir);
	} else {
		configdir = "./";
		glass3::util::Logger::log(
				"warning",
				"missing <ConfigDirectory>, defaulting to local directory.");
	}

	// set our proper loglevel
	if (glassConfig.getJSON()->HasKey("LogLevel")
			&& ((*glassConfig.getJSON())["LogLevel"].GetType()
					== json::ValueType::StringVal)) {
		glass3::util::Logger::log_update_level(
				(*glassConfig.getJSON())["LogLevel"]);
	}

	// get initialize config file location
	std::string initconfigfile;
	if (glassConfig.getJSON()->HasKey("InitializeFile")
			&& ((*glassConfig.getJSON())["InitializeFile"].GetType()
					== json::ValueType::StringVal)) {
		initconfigfile = (*glassConfig.getJSON())["InitializeFile"].ToString();
	} else {
		glass3::util::Logger::log(
				"critical",
				"Invalid configuration, missing <InitializeFile>, exiting.");

		return (1);
	}

	// load our initialize config
	glass3::util::Config InitializeConfig;
	try {
		InitializeConfig.parseJSONFromFile(configdir, initconfigfile);
	} catch (std::exception& e) {
		glass3::util::Logger::log(
				"criticalerror",
				"Failed to load file: " + initconfigfile + "; "
						+ std::string(e.what()));
		return (1);
	}

	// get stationlist file location
	std::string stationlistfile;
	if (glassConfig.getJSON()->HasKey("StationList")
			&& ((*glassConfig.getJSON())["StationList"].GetType()
					== json::ValueType::StringVal)) {
		stationlistfile = (*glassConfig.getJSON())["StationList"].ToString();
	} else {
		glass3::util::Logger::log(
				"critical",
				"Invalid configuration, missing <StationList>, exiting.");
		return (1);
	}

	// get our stationlist
	glass3::util::Config StationList;
	try {
		StationList.parseJSONFromFile(configdir, stationlistfile);
	} catch (std::exception& e) {
		glass3::util::Logger::log(
				"criticalerror",
				"Failed to load file: " + stationlistfile + "; "
						+ std::string(e.what()));

		return (1);
	}

	// get detection grid file list
	json::Array gridconfigfilelist;
	if (glassConfig.getJSON()->HasKey("GridFiles")
			&& ((*glassConfig.getJSON())["GridFiles"].GetType()
					== json::ValueType::ArrayVal)) {
		gridconfigfilelist = (*glassConfig.getJSON())["GridFiles"];
	} else {
		glass3::util::Logger::log(
				"critical",
				"Invalid configuration, missing <GridFiles>, exiting.");

		return (1);
	}

	// check to see if any files were in the array
	if (gridconfigfilelist.size() == 0) {
		glass3::util::Logger::log("critical",
									"No <GridFiles> specified, exiting.");

		return (1);
	}

	// get input config file location
	std::string inputconfigfile;
	if (glassConfig.getJSON()->HasKey("InputConfig")
			&& ((*glassConfig.getJSON())["InputConfig"].GetType()
					== json::ValueType::StringVal)) {
		inputconfigfile = (*glassConfig.getJSON())["InputConfig"].ToString();
	} else {
		glass3::util::Logger::log(
				"critical",
				"Invalid configuration, missing <InputConfig>, exiting.");
		return (1);
	}

	// load our input config
	glass3::util::Config InputConfig;
	try {
		InputConfig.parseJSONFromFile(configdir, inputconfigfile);
	} catch (std::exception& e) {
		glass3::util::Logger::log(
				"criticalerror",
				"Failed to load file: " + inputconfigfile + "; "
						+ std::string(e.what()));

		return (1);
	}

	// get output config file location
	std::string outputconfigfile;
	if (glassConfig.getJSON()->HasKey("OutputConfig")
			&& ((*glassConfig.getJSON())["OutputConfig"].GetType()
					== json::ValueType::StringVal)) {
		outputconfigfile = (*glassConfig.getJSON())["OutputConfig"].ToString();
	} else {
		glass3::util::Logger::log(
				"critical",
				"Invalid configuration, missing <OutputConfig>, exiting.");
		return (1);
	}

	// load our output config
	glass3::util::Config OutputConfig;
	try {
		OutputConfig.parseJSONFromFile(configdir, outputconfigfile);
	} catch (std::exception& e) {
		glass3::util::Logger::log(
				"criticalerror",
				"Failed to load file: " + outputconfigfile + "; "
						+ std::string(e.what()));

		return (1);
	}

	// create our objects
	glass::fileInput InputThread;
	glass::fileOutput OutputThread;
	glass3::process::Associator AssocThread(&InputThread, &OutputThread);

	// input setup
	if (InputThread.setup(InputConfig.getJSON()) != true) {
		glass3::util::Logger::log("critical",
									"glass: Failed to setup Input.  Exiting.");

		return (1);
	}

	// output setup
	if (OutputThread.setup(OutputConfig.getJSON()) != true) {
		glass3::util::Logger::log("critical",
									"glass: Failed to setup Output.  Exiting.");

		return (1);
	}

	// output needs to know about the associator thread to request
	// information
	OutputThread.setAssociator(&AssocThread);

	// configure glass
	// first send in initialize
	AssocThread.setup(InitializeConfig.getJSON());

	// then send in stationlist
	AssocThread.setup(StationList.getJSON());

	// finally send in grids
	for (int i = 0; i < gridconfigfilelist.size(); i++) {
		std::string gridconfigfile = gridconfigfilelist[i];
		if (gridconfigfile != "") {
			glass3::util::Config GridConfig(configdir, gridconfigfile);

			// send in grid
			AssocThread.setup(GridConfig.getJSON());
		}
	}

	// startup
	InputThread.start();
	OutputThread.start();
	AssocThread.start();

	glass3::util::Logger::log("info", "glass-app: glass3 is running.");

	// run until stopped
	while (true) {
		// sleep to give up cycles
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		glass3::util::Logger::log("trace",
									"glass-app: Checking thread status.");

		// check thread health
		if (InputThread.healthCheck() == false) {
			glass3::util::Logger::log("error",
										"glass-app: Input thread has exited!!");
			break;
		} else if (OutputThread.healthCheck() == false) {
			glass3::util::Logger::log(
					"error", "glass-app: Output thread has exited!!");
			break;
		} else if (AssocThread.healthCheck() == false) {
			glass3::util::Logger::log(
					"error", "glass-app: Association thread has exited!!");
			break;
		}
	}

	glass3::util::Logger::log("info", "glass-app: glass3 is shutting down.");

	// shutdown
	InputThread.stop();
	OutputThread.stop();
	AssocThread.stop();

	return (0);
}
