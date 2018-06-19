// glass-app.cpp : Defines the entry point for the console application.
#include <json.h>
#include <logger.h>
#include <config.h>
#include <input.h>
#include <file_output.h>
#include <associator.h>
#include <glass-appCMakeConfig.h>

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
	std::string configdir = "";

	// check our arguments
	if ((argc < 2) || (argc > 3)) {
		std::cout << "glass-app version " << std::to_string(GLASS_VERSION_MAJOR)
					<< "." << std::to_string(GLASS_VERSION_MINOR) << "; Usage: "
					<< "glass-app <configfile> [noconsole]" << std::endl;
		return 1;
	}

	// Look up our log directory
	std::string logpath;
	char* pLogDir = getenv("GLASS_LOG");
	if (pLogDir != NULL) {
		logpath = pLogDir;
	} else {
		logpath = "./";
	}

	// get whether we log to console
	bool logConsole = true;
	if (argc == 3) {
		std::string temp = std::string(argv[2]);

		if (temp == "noconsole") {
			logConsole = false;
		}
	}

	// now set up our logging
	logger::log_init("glass-app", spdlog::level::info, logpath, logConsole);

	logger::log(
			"info",
			"glass-app: neic-glass3 Version "
					+ std::to_string(GLASS_VERSION_MAJOR) + "."
					+ std::to_string(GLASS_VERSION_MINOR) + "."
					+ std::to_string(GLASS_VERSION_PATCH) + " startup.");

	logger::log(
			"info",
			"glass-app: loading configuration file " + std::string(argv[1]));

	// now load our basic config from file
	// note, main is gonna get cluttered, move this section
	// to it's own function eventually
	util::Config * glassConfig = new util::Config();

	try {
		glassConfig->parseJSONFromFile("", std::string(argv[1]));
	} catch (std::exception& e) {
		logger::log(
				"critcalerror",
				"Failed to load file: " + std::string(argv[1]) + "; "
						+ std::string(e.what()));

		// delete (glassConfig);
		// return (1);
	}

	// check to see if our json config is of the right format
	if (!(glassConfig->getJSON().HasKey("Cmd"))) {
		logger::log("critcalerror", "Invalid configuration, exiting.");

		delete (glassConfig);
		return (1);
	}
	if ((glassConfig->getJSON())["Cmd"] != "Glass") {
		logger::log("critcalerror", "Wrong configuration, exiting.");

		delete (glassConfig);
		return (1);
	}

	// get the directory where the rest of the glass configs are stored
	if (!(glassConfig->getJSON().HasKey("ConfigDirectory"))) {
		configdir = "./";
		logger::log(
				"warning",
				"missing <ConfigDirectory>, defaulting to local directory.");
	} else {
		configdir = (glassConfig->getJSON())["ConfigDirectory"].ToString();
		logger::log("info", "Reading glass configurations from: " + configdir);
	}

	// set our proper loglevel
	if (glassConfig->getJSON().HasKey("LogLevel")) {
		logger::log_update_level((glassConfig->getJSON())["LogLevel"]);
	}

	// load our initialize config
	std::string initconfigfile;
	if (!(glassConfig->getJSON().HasKey("InitializeFile"))) {
		logger::log(
				"critcalerror",
				"Invalid configuration, missing <InitializeFile>, exiting.");

		delete (glassConfig);
		return (1);
	} else {
		initconfigfile = (glassConfig->getJSON())["InitializeFile"].ToString();
	}

	util::Config * InitializeConfig = new util::Config(configdir,
														initconfigfile);
	json::Object * InitializeJSON = new json::Object(
			InitializeConfig->getJSON());

	// Load our initial stationlist
	std::string stationlistfile;
	if (!(glassConfig->getJSON().HasKey("StationList"))) {
		logger::log("critcalerror",
					"Invalid configuration, missing <StationList>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		return (1);
	} else {
		stationlistfile = (glassConfig->getJSON())["StationList"].ToString();
	}
	// get our stationlist
	util::Config * StationList = new util::Config(configdir, stationlistfile);
	json::Object * StationListJSON = new json::Object(StationList->getJSON());

	// get detection grid file list
	json::Array gridconfigfilelist;
	if (glassConfig->getJSON().HasKey("GridFiles")
			&& ((glassConfig->getJSON())["GridFiles"].GetType()
					== json::ValueType::ArrayVal)) {
		gridconfigfilelist = (glassConfig->getJSON())["GridFiles"];
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

	// load our input config
	std::string inputconfigfile;
	if (!(glassConfig->getJSON().HasKey("InputConfig"))) {
		logger::log("critcalerror",
					"Invalid configuration, missing <InputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		return (1);
	} else {
		inputconfigfile = (glassConfig->getJSON())["InputConfig"].ToString();
	}
	util::Config * InputConfig = new util::Config(configdir, inputconfigfile);

	// load our output config
	std::string outputconfigfile;
	if (!(glassConfig->getJSON().HasKey("OutputConfig"))) {
		logger::log("critcalerror",
					"Invalid configuration, missing <OutputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (InputConfig);
		return (1);
	} else {
		outputconfigfile = (glassConfig->getJSON())["OutputConfig"].ToString();
	}
	util::Config * OutputConfig = new util::Config(configdir, outputconfigfile);

	// create our objects
	glass::input * InputThread = new glass::input(5);
	glass::fileOutput * OutputThread = new glass::fileOutput();
	glass::Associator * AssocThread = new glass::Associator();

	// input setup
	json::Object * input_config_json = new json::Object(InputConfig->getJSON());
	if (InputThread->setup(input_config_json) != true) {
		logger::log("critical", "glass: Failed to setup Input.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (InputConfig);
		delete (input_config_json);
		delete (OutputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// output setup
	json::Object * output_config_json = new json::Object(
			OutputConfig->getJSON());
	if (OutputThread->setup(output_config_json) != true) {
		logger::log("critical", "glass: Failed to setup Output.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (InitializeJSON);
		delete (StationList);
		delete (StationListJSON);
		delete (InputConfig);
		delete (input_config_json);
		delete (OutputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		delete (output_config_json);
		return (1);
	}

	// output needs to know about the associator thread to request
	// information
	OutputThread->Associator = AssocThread;

	// assoc thread needs to know about the input and output threads
	AssocThread->Input = InputThread;
	AssocThread->Output = OutputThread;

	// configure glass
	// first send in initialize
	AssocThread->setup(InitializeJSON);

	// send in stationlist
	AssocThread->setup(StationListJSON);

	// send in grids
	for (int i = 0; i < gridconfigfilelist.size(); i++) {
		std::string gridconfigfile = gridconfigfilelist[i];
		if (gridconfigfile != "") {
			util::Config * GridConfig = new util::Config(configdir,
															gridconfigfile);
			json::Object * GridConfigJSON = new json::Object(
					GridConfig->getJSON());

			// send in grid
			AssocThread->setup(GridConfigJSON);

			// done with grid config and json
			delete (GridConfig);
			delete (GridConfigJSON);
		}
	}

	// startup
	InputThread->start();
	OutputThread->start();
	AssocThread->start();

	logger::log("info", "glass: glass is running.");

	// run until stopped
	while (true) {
		// sleep to give up cycles
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		logger::log("trace", "glass: Checking thread status.");

		// check thread health
		if (InputThread->check() == false) {
			logger::log("error", "glass: Input thread has exited!!");
			break;
		} else if (OutputThread->check() == false) {
			logger::log("error", "glass: Output thread has exited!!");
			break;
		} else if (AssocThread->check() == false) {
			logger::log("error", "glass: Association thread has exited!!");
			break;
		}
	}

	logger::log("info", "glass: glass is shutting down.");

	// shutdown
	InputThread->stop();
	OutputThread->stop();
	AssocThread->stop();

	// cleanup
	delete (glassConfig);
	delete (InitializeConfig);
	delete (InitializeJSON);
	delete (StationList);
	delete (StationListJSON);
	delete (InputConfig);
	delete (input_config_json);
	delete (OutputConfig);
	delete (InputThread);
	delete (OutputThread);
	delete (AssocThread);
	delete (output_config_json);

	return 0;
}
