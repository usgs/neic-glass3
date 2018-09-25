#include <gtest/gtest.h>
#include <file_output.h>
#include <config.h>
#include <logger.h>
#include <date.h>
#include <associatorinterface.h>
#include <string>

#define CONFIGFILENAME "outputtest.d"
#define TESTPATH "testdata"
#define TESTDATAPATH "outputtests"
#define OUTPUTDIRECTORY "output"

#define EMPTYCONFIG "{\"Cmd\":\"GlassOutput\",\"OutputDirectory\":\"huh\"}"
#define CONFIGFAIL1 "{\"PublicationTimes\":[3, 6]}"
#define CONFIGFAIL2 "{\"Cmd\":\"BLEH\"}"

#define SLEEPTIME 100
#define TESTAGENCYID "US"
#define TESTAUTHOR "glass"
#define REPORTINTERVAL 60
#define OUTPUTFORMAT "json"

#define OUTPUTID "DB277841F26BB84089FE877BAAB85084"
#define HYPOFILE "hypo.txt"
#define EVENTFILE "event.txt"
#define OUTPUTFILE "DB277841F26BB84089FE877BAAB85084.jsondetect"

#define OUTPUT2ID "7D52AC725BE6FA478A16EC918B80E271"
#define HYPO2FILE "hypo2.txt"
#define EVENT2FILE "event2.txt"
#define HYPO2UPDATEFILE "hypo2update.txt"
#define EVENT2UPDATEFILE "event2update.txt"
#define OUTPUT2FILE "7D52AC725BE6FA478A16EC918B80E271.jsondetect"

#define OUTPUT3ID "0BB39FF59826AA4BA63AAA07FFFE713F"
#define HYPO3FILE "hypo3.txt"
#define EVENT3FILE "event3.txt"
#define CANCEL3FILE "cancel3.txt"
#define OUTPUT3FILE "0BB39FF59826AA4BA63AAA07FFFE713F.jsondetect"
#define RETRACT3FILE "0BB39FF59826AA4BA63AAA07FFFE713F.jsonrtct"

std::shared_ptr<json::Object> GetDataFromFile(std::string filename) {
	std::ifstream infile;
	if (std::ifstream(filename).good() != true) {
		return (NULL);
	}

	// open the file
	infile.open(filename, std::ios::in);

	// get the line
	std::string line;
	std::getline(infile, line);

	// close the file
	infile.close();

	if (line.length() == 0)
		return (NULL);

	json::Value deserializedJSON = json::Deserialize(line);

	// make sure we got valid json
	if (deserializedJSON.GetType() != json::ValueType::NULLVal) {
		// convert our resulting value to a json object
		std::shared_ptr<json::Object> newdata = std::make_shared<json::Object>(
				json::Object(deserializedJSON.ToObject()));
		return (newdata);
	}

	return (NULL);
}

class AssociatorStub : public glass3::util::iAssociator {
 public:
	AssociatorStub() {
		Output = NULL;

		testpath = std::string(TESTPATH);
		testdatapath = std::string(TESTDATAPATH);

		// build file names
		inputdirectory = "./" + testpath + "/" + testdatapath;
		hypofile = inputdirectory + "/" + std::string(HYPOFILE);
		hypo2file = inputdirectory + "/" + std::string(HYPO2FILE);
		hypo2updatefile = inputdirectory + "/" + std::string(HYPO2UPDATEFILE);
		hypo3file = inputdirectory + "/" + std::string(HYPO3FILE);

		sentone = false;
	}

	virtual ~AssociatorStub() {
	}

	void sendToAssociator(std::shared_ptr<json::Object> &message) override {
		if (message == NULL) {
			return;
		}
		if (Output == NULL) {
			return;
		}

		std::string id = "";
		if ((*message).HasKey("ID")) {
			id = (*message)["ID"].ToString();
		} else if ((*message).HasKey("Pid")) {
			id = (*message)["Pid"].ToString();
		} else {
			return;
		}

		if (id == OUTPUTID) {
			std::shared_ptr<json::Object> hypo = GetDataFromFile(hypofile);
			Output->sendToOutput(hypo);
		} else if (id == OUTPUT2ID) {
			if (sentone == false) {
				std::shared_ptr<json::Object> hypo2 = GetDataFromFile(
						hypo2file);
				Output->sendToOutput(hypo2);
				sentone = true;
			} else {
				std::shared_ptr<json::Object> hypo2Update = GetDataFromFile(
						hypo2updatefile);
				Output->sendToOutput(hypo2Update);
			}

		} else if (id == OUTPUT3ID) {
			std::shared_ptr<json::Object> hypo3 = GetDataFromFile(hypo3file);
			Output->sendToOutput(hypo3);
		}
	}
	bool sentone;

	glass3::util::iOutput* Output;

	std::string testpath;
	std::string testdatapath;
	std::string inputdirectory;

	std::string hypofile;
	std::string hypo2file;
	std::string hypo2updatefile;
	std::string hypo3file;
};

class OutputTest : public ::testing::Test {
 protected:
	virtual void SetUp() {
		testpath = std::string(TESTPATH);
		testdatapath = std::string(TESTDATAPATH);
		std::string output = std::string(OUTPUTDIRECTORY);

		// glass3::util::log_init("outputtest", spdlog::level::debug, testpath, true);

		// create input test
		OutputThread = NULL;
		OutputConfig = NULL;
		output_config_json = NULL;
		AssocThread = NULL;

		int nError = 0;

		// create archive and error directories
		configdirectory = "./" + testpath;
		inputdirectory = "./" + testpath + "/" + testdatapath;
		outputdirectory = "./" + testpath + "/" + testdatapath + "/" + output;

		// create testing directories
#ifdef _WIN32
		nError = _mkdir(outputdirectory.c_str());
#else
		mode_t nMode = 0733;
		nError = mkdir(outputdirectory.c_str(), nMode);
#endif

		if (nError != 0) {
			printf("Failed to create testing directory %s.\n",
					outputdirectory.c_str());
		}

		// build file names
		eventfile = inputdirectory + "/" + std::string(EVENTFILE);
		event2file = inputdirectory + "/" + std::string(EVENT2FILE);
		event2updatefile = inputdirectory + "/" + std::string(EVENT2UPDATEFILE);
		event3file = inputdirectory + "/" + std::string(EVENT3FILE);

		hypofile = inputdirectory + "/" + std::string(HYPOFILE);
		hypo2file = inputdirectory + "/" + std::string(HYPO2FILE);
		hypo2updatefile = inputdirectory + "/" + std::string(HYPO2UPDATEFILE);
		hypo3file = inputdirectory + "/" + std::string(HYPO3FILE);
		cancel3file = inputdirectory + "/" + std::string(CANCEL3FILE);

		outputfile = outputdirectory + "/" + std::string(OUTPUTFILE);
		output2file = outputdirectory + "/" + std::string(OUTPUT2FILE);
		output3file = outputdirectory + "/" + std::string(OUTPUT3FILE);
		retract3file = outputdirectory + "/" + std::string(RETRACT3FILE);
	}

	bool configure() {
		// create configfilestring
		std::string configfile = std::string(CONFIGFILENAME);

		// load configuration
		OutputConfig = new glass3::util::Config(configdirectory, configfile);

		// get json formatted configuration
		output_config_json = OutputConfig->getJSON();

		OutputThread = new glass::fileOutput(output_config_json);

		AssocThread = new AssociatorStub();
		AssocThread->Output = OutputThread;
		OutputThread->setAssociator(AssocThread);

		// configure output
		return (true);
	}

	bool configurefail1() {
		// configure fail
		OutputThread = new glass::fileOutput();
		return (OutputThread->setup(NULL));
	}

	bool configurefail2() {
		// configure fail
		OutputThread = new glass::fileOutput();
		return (OutputThread->setup(
				std::make_shared<json::Object>(json::Deserialize(CONFIGFAIL1))));
	}

	bool configurefail3() {
		// configure fail
		OutputThread = new glass::fileOutput();
		return (OutputThread->setup(
				std::make_shared<json::Object>(json::Deserialize(CONFIGFAIL2))));
	}

	bool emptyconfig() {
		// configure empty
		OutputThread = new glass::fileOutput();
		return (OutputThread->setup(
				std::make_shared<json::Object>(json::Deserialize(EMPTYCONFIG))));
	}

	void CheckData(std::shared_ptr<json::Object> dataone,
					std::shared_ptr<json::Object> datatwo) {
		if (dataone == NULL) {
			return;
		}
		if (datatwo == NULL) {
			return;
		}

		double lat1 = 0;
		double lat2 = 0;
		double lon1 = 0;
		double lon2 = 0;
		double depth1 = 0;
		double depth2 = 0;
		std::string time1 = "";
		std::string time2 = "";

		// latitude
		if (dataone->HasKey("Latitude"))
			lat1 = (*dataone)["Latitude"].ToDouble();
		if (datatwo->HasKey("Hypocenter")) {
			json::Object hypo = (*datatwo)["Hypocenter"].ToObject();
			if (hypo.HasKey("Latitude"))
				lat2 = hypo["Latitude"].ToDouble();
		}
		ASSERT_EQ(lat1, lat2)<< "Latitude matches";

		// longitude
		if (dataone->HasKey("Longitude"))
			lon1 = (*dataone)["Longitude"].ToDouble();
		if (datatwo->HasKey("Hypocenter")) {
			json::Object hypo = (*datatwo)["Hypocenter"].ToObject();
			if (hypo.HasKey("Longitude"))
				lon2 = hypo["Longitude"].ToDouble();
		}
		ASSERT_EQ(lon1, lon2)<< "Longitude matches";

		// depth
		if (dataone->HasKey("Depth"))
			depth1 = (*dataone)["Depth"].ToDouble();
		if (datatwo->HasKey("Hypocenter")) {
			json::Object hypo = (*datatwo)["Hypocenter"].ToObject();
			if (hypo.HasKey("Depth"))
				depth2 = hypo["Depth"].ToDouble();
		}
		ASSERT_EQ(depth1, depth2)<< "Depth matches";

		// time
		if (dataone->HasKey("Time"))
			time1 = (*dataone)["Time"].ToString();
		if (datatwo->HasKey("Hypocenter")) {
			json::Object hypo = (*datatwo)["Hypocenter"].ToObject();
			if (hypo.HasKey("Time"))
				time2 = hypo["Time"].ToString();
		}
		ASSERT_STREQ(time1.c_str(), time2.c_str())<< "Time matches";
	}

	virtual void TearDown() {
		// need to clean up output files
		if (std::ifstream(outputfile).good()) {
			std::remove(outputfile.c_str());
		}

		if (std::ifstream(output2file).good()) {
			std::remove(output2file.c_str());
		}

		if (std::ifstream(output3file).good()) {
			std::remove(output3file.c_str());
		}

		if (std::ifstream(retract3file).good()) {
			std::remove(retract3file.c_str());
		}

		// need to clean up output directories
#ifdef _WIN32
		RemoveDirectory(outputdirectory.c_str());
#else
		rmdir(outputdirectory.c_str());
#endif

		// cleanup output thread
		if (OutputThread != NULL)
		delete (OutputThread);
		if (OutputConfig != NULL)
		delete(OutputConfig);

	}

	glass::fileOutput * OutputThread;
	AssociatorStub * AssocThread;

	glass3::util::Config * OutputConfig;
	std::shared_ptr<const json::Object> output_config_json;

	std::string testpath;
	std::string testdatapath;

	std::string configdirectory;
	std::string inputdirectory;
	std::string outputdirectory;

	std::string eventfile;
	std::string event2file;
	std::string event2updatefile;
	std::string event3file;

	std::string hypofile;
	std::string hypo2file;
	std::string hypo2updatefile;
	std::string hypo3file;
	std::string cancel3file;

	std::string outputfile;
	std::string output2file;
	std::string output3file;
	std::string retract3file;
};

// tests to see if correlation can successfully
// write json output
TEST_F(OutputTest, Construction) {
	OutputThread = new glass::fileOutput();

	// assert that this is an input thread
	ASSERT_STREQ(OutputThread->getThreadName().c_str(), "output")<< "check output thread name";

	// assert the thread sleeptime
	ASSERT_EQ(OutputThread->getSleepTime(), SLEEPTIME)<< "check output thread sleep time";

	// assert class is not set up
	ASSERT_FALSE(OutputThread->getSetup())<< "output thread is not set up";

	// assert class has no config
	ASSERT_TRUE(OutputThread->getConfig() == NULL)<< "output config is null";

	// assert class is not running
	ASSERT_FALSE(OutputThread->getWorkThreadsState() ==
			glass3::util::ThreadState::Started)<< "output thread is not running";

	// assert no data in class
	ASSERT_EQ(OutputThread->getReportInterval(), REPORTINTERVAL)<< "output thread report interval";
}

TEST_F(OutputTest, Configuration) {
	// failes
	ASSERT_FALSE(configurefail1())<< "OutputThread->setup returned false 1";
	ASSERT_FALSE(configurefail2())<< "OutputThread->setup returned false 2";
	ASSERT_FALSE(configurefail3())<< "OutputThread->setup returned false 3";
	ASSERT_FALSE(emptyconfig())<< "OutputThread->setup returned true";

	// configure output
	ASSERT_TRUE(configure())<< "OutputThread->setup returned true";

	// assert class is set up
	ASSERT_TRUE(OutputThread->getSetup()) << "input thread is set up";

	// assert class has config
	ASSERT_TRUE(OutputThread->getConfig() != NULL) << "input config is notnull";

	// check input directory
	ASSERT_STREQ(OutputThread->getSOutputDir().c_str(),
			outputdirectory.c_str()) << "check output thread output directory";

	// check input directory
	std::string outputformat = std::string(OUTPUTFORMAT);
	ASSERT_STREQ(OutputThread->getSOutputFormat().c_str(),
			outputformat.c_str()) << "check output thread output format";

	// check agency id
	std::string agencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(OutputThread->getDefaultAgencyId().c_str(),
			agencyid.c_str()) << "check agency id";

	// check author
	std::string author = std::string(TESTAUTHOR);
	ASSERT_STREQ(OutputThread->getDefaultAuthor().c_str(),
			author.c_str()) << "check author";
}

TEST_F(OutputTest, Output) {
	// configure output
	ASSERT_TRUE(configure())<< "OutputThread->setup returned true";

	// start input thread
	OutputThread->start();
	OutputThread->clearTrackingData();

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(event3file);
	(*outputevent)["CreateTime"] = glass3::util::Date::encodeISO8601Time(
			glass3::util::Date::now());
	(*outputevent)["ReportTime"] = glass3::util::Date::encodeISO8601Time(
			glass3::util::Date::now());

	// add data to output
	OutputThread->sendToOutput(outputevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that the file is there
	ASSERT_TRUE(std::ifstream(output3file).good())
	<< "output file created";

	std::shared_ptr<json::Object> cancelmessage = GetDataFromFile(cancel3file);

	// send cancel to output
	OutputThread->sendToOutput(cancelmessage);

	// give time for files to write
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that the file is not there
	ASSERT_TRUE(std::ifstream(retract3file).good())
	<< "retract output file created";
}

/*

 TEST_F(OutputTest, Update) {
 // configure output
 ASSERT_TRUE(configure())<< "OutputThread->setup returned true";

 // start input thread
 OutputThread->start();
 OutputThread->clearTrackingData();

 std::shared_ptr<json::Object> outputevent = GetDataFromFile(event2file);
 (*outputevent)["CreateTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());
 (*outputevent)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());

 // add data to output
 OutputThread->sendToOutput(outputevent);

 // give time for file to write
 std::this_thread::sleep_for(std::chrono::seconds(4));

 // assert that the file is there
 ASSERT_TRUE(std::ifstream(output2file).good())
 << "hypo output file created";

 // get the data
 std::shared_ptr<json::Object> senthypo2 = GetDataFromFile(hypo2file);
 std::shared_ptr<json::Object> output2origin = GetDataFromFile(output2file);

 // check the output data against the update
 CheckData(senthypo2, output2origin);

 //remove output for update
 std::remove(output2file.c_str());

 std::shared_ptr<json::Object> updateevent = GetDataFromFile(event2updatefile);
 (*updateevent)["CreateTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());
 (*updateevent)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());

 // send update to output
 OutputThread->sendToOutput(updateevent);

 // give time for file to write
 std::this_thread::sleep_for(std::chrono::seconds(10));

 // assert that the file is there
 ASSERT_TRUE(std::ifstream(output2file).good())
 << "hypo update file created";

 // get the data
 std::shared_ptr<json::Object> sentupdatehypo2 = GetDataFromFile(hypo2updatefile);
 std::shared_ptr<json::Object> output2origin2 = GetDataFromFile(output2file);

 // check the output data against the update
 CheckData(sentupdatehypo2, output2origin2);
 }

 TEST_F(OutputTest, Cancel) {
 // configure output
 ASSERT_TRUE(configure())<< "OutputThread->setup returned true";

 // start input thread
 OutputThread->start();
 OutputThread->clearTrackingData();

 std::shared_ptr<json::Object> outputevent = GetDataFromFile(event3file);
 (*outputevent)["CreateTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());
 (*outputevent)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());

 std::shared_ptr<json::Object> cancelmessage = GetDataFromFile(cancel3file);

 // add data to output
 OutputThread->sendToOutput(outputevent);

 // send cancel to output
 OutputThread->sendToOutput(cancelmessage);

 // give time for file to write
 std::this_thread::sleep_for(std::chrono::seconds(4));

 // assert that the file is not there
 ASSERT_FALSE(std::ifstream(output3file).good())
 << "hypo output file not created";

 ASSERT_TRUE(OutputThread->check());
 }

 TEST_F(OutputTest, Retract) {
 // configure output
 ASSERT_TRUE(configure())<< "OutputThread->setup returned true";

 // start input thread
 OutputThread->start();
 OutputThread->clearTrackingData();

 std::shared_ptr<json::Object> outputevent = GetDataFromFile(event3file);
 (*outputevent)["CreateTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());
 (*outputevent)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
 glassutil::CDate::now());

 // add data to output
 OutputThread->sendToOutput(outputevent);

 // give time for file to write
 std::this_thread::sleep_for(std::chrono::seconds(4));

 // assert that the file is there
 ASSERT_TRUE(std::ifstream(output3file).good())
 << "output file created";

 std::shared_ptr<json::Object> cancelmessage = GetDataFromFile(cancel3file);

 // send cancel to output
 OutputThread->sendToOutput(cancelmessage);

 // give time for files to write
 std::this_thread::sleep_for(std::chrono::seconds(1));

 // assert that the file is not there
 ASSERT_TRUE(std::ifstream(retract3file).good())
 << "retract output file created";
 }

 */
