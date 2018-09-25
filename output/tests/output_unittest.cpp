#include <gtest/gtest.h>
#include <output.h>
#include <config.h>
#include <logger.h>
#include <date.h>

#include <string>
#include <ctime>
#include <memory>

#define EMPTYCONFIG "{\"Cmd\":\"GlassOutput\"}"
#define CONFIGFAIL1 "{\"PublicationTimes\":[3, 6]}"
#define CONFIGFAIL2 "{\"Type\":\"BLEH\"}"
#define CONFIGFILENAME "outputtest.d"
#define TESTPATH "./testdata"

#define SLEEPTIME 100

#define REPORTINTERVAL 60
#define PUBDELAY1 3
#define PUBDELAY2 6
#define PUBONEXPIRE true
#define SITELISTDELAY 72
#define STATIONFILE "./testdata/stationlist.d"
#define TESTAGENCYID "US"
#define TESTAUTHOR "glass"

#define OUTPUTID "DB277841F26BB84089FE877BAAB85084"
#define HYPOFILE "hypo.txt"
#define EVENTFILE "event.txt"

#define OUTPUT2ID "7D52AC725BE6FA478A16EC918B80E271"
#define HYPO2FILE "hypo2.txt"
#define EVENT2FILE "event2.txt"
#define HYPO2UPDATEFILE "hypo2update.txt"
#define EVENT2UPDATEFILE "event2update.txt"
#define EVENT2UPDATE2FILE "event2update2.txt"
#define EVENT2EXPIREFILE "event2expire.txt"

#define OUTPUT3ID "0BB39FF59826AA4BA63AAA07FFFE713F"
#define HYPO3FILE "hypo3.txt"
#define EVENT3FILE "event3.txt"
#define CANCEL3FILE "cancel3.txt"

#define TRACKING1 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Pid\":\"7D52AC725BE6FA478A16EC918B80E271\",\"Version\":1}" // NOLINT
#define TRACKING2 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"ID\":\"7D52AC725BE6FA478A16EC918B80E271\",\"Version\":1}" // NOLINT
#define TRACKING3 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"ID\":\"9DD2AC7HJBE6FA478AJMPC2112805098\",\"Version\":1}" // NOLINT
#define ID1 "7D52AC725BE6FA478A16EC918B80E271"
#define ID3 "9DD2AC7HJBE6FA478AJMPC2112805098"

#define BADTRACKING1 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Pid\":\"\",\"Version\":1}" // NOLINT
#define BADTRACKING2 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Version\":1}"

#define SITEREQUESTFILE "siterequest.txt"
#define SITELISTFILE "sitelist.txt"

std::shared_ptr<json::Object> GetDataFromString(std::string line) {
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

	printf("GetDataFromString: Bad json from line %s\n", line.c_str());
	return (NULL);
}

std::shared_ptr<json::Object> GetDataFromFile(std::string filename) {
	std::ifstream infile;
	if (std::ifstream(filename).good() != true) {
		printf("GetDataFromFile: Bad file name %s\n", filename.c_str());
		return (NULL);
	}

	// open the file
	infile.open(filename, std::ios::in);

	// get the line
	std::string line;
	std::getline(infile, line);

	// close the file
	infile.close();

	return (GetDataFromString(line));
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

class AssociatorStub : public glass3::util::iAssociator {
 public:
	AssociatorStub() {
		Output = NULL;

		// build file names
		testpath = std::string(TESTPATH);
		inputdirectory = "./" + testpath;
		hypofile = inputdirectory + "/" + std::string(HYPOFILE);
		hypo2file = inputdirectory + "/" + std::string(HYPO2FILE);
		hypo2updatefile = inputdirectory + "/" + std::string(HYPO2UPDATEFILE);
		hypo3file = inputdirectory + "/" + std::string(HYPO3FILE);

		sitelistfile = inputdirectory + "/" + std::string(SITELISTFILE);

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

		std::string type = "";
		if (message->HasKey("Cmd")) {
			type = (*message)["Cmd"].ToString();
		} else if (message->HasKey("Type")) {
			type = (*message)["Type"].ToString();
		}

		if (type == "ReqSiteList") {
			std::shared_ptr<json::Object> sitelist = GetDataFromFile(
					sitelistfile);
			Output->sendToOutput(sitelist);
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

	std::string sitelistfile;
};

class OutputStub : public glass3::output::output {
 public:
	OutputStub() {
	}

	virtual ~OutputStub() {
	}

	void clearMessages() {
		messages.clear();
	}

	void sendOutput(const std::string &type, const std::string &id,
					const std::string &message) override {
		messages.push_back(message);
	}

	std::vector<std::string> messages;
};

TEST(Output, Construction) {
	OutputStub outputObject;

	// assert that this is an input thread
	ASSERT_STREQ(outputObject.getThreadName().c_str(), "output")<< "check output thread name";

	// assert the thread sleeptime
	ASSERT_EQ(outputObject.getSleepTime(), SLEEPTIME)<< "check output thread sleep time";

	// assert report interval
	ASSERT_EQ(outputObject.getReportInterval(), REPORTINTERVAL)<< "output thread report interval";

	// assert class is not set up
	ASSERT_FALSE(outputObject.getSetup())<< "output thread is not set up";

	// assert class has no config
	ASSERT_TRUE(outputObject.getConfig() == NULL)<< "output config is null";

	// assert class is not started
	ASSERT_TRUE(outputObject.getWorkThreadsState() ==
			glass3::util::ThreadState::Initialized)<< "output thread is not started";

	// assert no associator
	ASSERT_TRUE(outputObject.getAssociator()== NULL)<< "associator is null";
}

TEST(Output, Configuration) {
	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// config failure cases
	ASSERT_FALSE(outputObject->setup(NULL));
	ASSERT_FALSE(
			outputObject->setup(std::make_shared<json::Object>(json::Deserialize(CONFIGFAIL1))));  // NOLINT
	ASSERT_FALSE(
			outputObject->setup(std::make_shared<json::Object>(json::Deserialize(EMPTYCONFIG))));  // NOLINT

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// assert the thread sleeptime
	ASSERT_EQ(outputObject->getSleepTime(), SLEEPTIME)<< "check output thread sleep time";

	// assert report interval
	ASSERT_EQ(outputObject->getReportInterval(), REPORTINTERVAL)<< "output thread report interval";

	// assert class is set up
	ASSERT_TRUE(outputObject->getSetup())<< "output thread is set up";

	// assert class has config
	ASSERT_FALSE(outputObject->getConfig() == NULL)<< "output config is not null";

	// assert associator
	ASSERT_FALSE(outputObject->getAssociator() == NULL)<< "associator is not null";

	// assert pub times
	std::vector<int> pubTimes = outputObject->getPubTimes();
	ASSERT_EQ(pubTimes.size(), 2)<< "correct number of pub times";
	ASSERT_EQ(pubTimes[0], PUBDELAY1)<< "first pub time correct";
	ASSERT_EQ(pubTimes[1], PUBDELAY2)<< "second pub time correct";

	// assert pub on expire
	ASSERT_EQ(outputObject->getPubOnExpiration(), PUBONEXPIRE)<< "pub on expire correct";

	// assert site list delay
	ASSERT_EQ(outputObject->getSiteListRequestInterval(), SITELISTDELAY)<<
	"sitelist delay correct";

	// check agency id
	std::string agencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(outputObject->getDefaultAgencyId().c_str(),
			agencyid.c_str())<< "check agency id";

	// check author
	std::string author = std::string(TESTAUTHOR);
	ASSERT_STREQ(outputObject->getDefaultAuthor().c_str(),
			author.c_str())<< "check author";
}

TEST(Output, ThreadTests) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// configure output
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			std::string(TESTPATH), std::string(CONFIGFILENAME));
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start output thread
	ASSERT_TRUE(outputObject->start())<< "start successful";

	// give time for startup?
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert class is  started
	ASSERT_TRUE(outputObject->getWorkThreadsState() ==
			glass3::util::ThreadState::Started)<< "output thread is started";

	// thread status checks
	ASSERT_TRUE(outputObject->healthCheck());

	// second start doesn't
	ASSERT_FALSE(outputObject->start())<< "second start unsuccessful";

	// stop output thread
	ASSERT_TRUE(outputObject->stop())<< "stop successful";

	// give time for shutdown
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert class is not started
	ASSERT_FALSE(outputObject->getWorkThreadsState() ==
			glass3::util::ThreadState::Started)<< "output thread is not started";

	// stop output thread again
	ASSERT_FALSE(outputObject->stop())<< "second stop unsuccessful";
}

// tests to see if tracking functions are functional
TEST(Output, TrackingTests) {
	// create output stub
	OutputStub outputThread;
	std::shared_ptr<json::Object> nullTrack;

	time_t tNow;
	std::time(&tNow);

	// configure output
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			std::string(TESTPATH), std::string(CONFIGFILENAME));
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	// setup
	ASSERT_TRUE(outputThread.setup(OutputJSON))<< "output config is successful";

	std::shared_ptr<json::Object> tracking1 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING1)));

	(*tracking1)["CreateTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);
	(*tracking1)["ReportTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);

	std::shared_ptr<json::Object> tracking2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING2)));

	(*tracking2)["CreateTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);
	(*tracking2)["ReportTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);

	std::shared_ptr<json::Object> tracking3 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING3)));

	(*tracking3)["CreateTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);
	(*tracking3)["ReportTime"] = glass3::util::Date::convertEpochTimeToISO8601(
			tNow);

	// add success
	ASSERT_TRUE(outputThread.addTrackingData(tracking1));
	ASSERT_TRUE(outputThread.addTrackingData(tracking2));
	ASSERT_TRUE(outputThread.addTrackingData(tracking3));

	// add fails
	ASSERT_FALSE(outputThread.addTrackingData(NULL));
	ASSERT_FALSE(
			outputThread.addTrackingData(std::make_shared<json::Object>(json::Object(json::Deserialize(BADTRACKING1)))));  // NOLINT
	ASSERT_FALSE(
			outputThread.addTrackingData(std::make_shared<json::Object>(json::Object(json::Deserialize(BADTRACKING2)))));  // NOLINT

	std::shared_ptr<json::Object> tracking4 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING1)));
	std::shared_ptr<json::Object> tracking5 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING2)));

	// have successes
	ASSERT_TRUE(outputThread.haveTrackingData(tracking4));
	ASSERT_TRUE(outputThread.haveTrackingData(tracking5));
	ASSERT_TRUE(outputThread.haveTrackingData(std::string(ID1)));

	// have fails
	ASSERT_FALSE(outputThread.haveTrackingData(nullTrack));
	ASSERT_FALSE(outputThread.haveTrackingData(""));

	// remove successes
	ASSERT_TRUE(outputThread.removeTrackingData(tracking4));

	std::shared_ptr<json::Object> tracking6 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING1)));

	ASSERT_FALSE(outputThread.haveTrackingData(tracking6));

	// remove fail
	ASSERT_FALSE(outputThread.removeTrackingData(nullTrack));
	ASSERT_FALSE(outputThread.removeTrackingData(""));

	// clear
	outputThread.clearTrackingData();
	ASSERT_FALSE(outputThread.haveTrackingData(std::string(ID3)));
}

TEST(Output, OutputTest) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string eventfile = std::string(TESTPATH) + "/"
			+ std::string(EVENTFILE);
	std::string hypofile = std::string(TESTPATH) + "/" + std::string(HYPOFILE);
	time_t tNow;
	std::time(&tNow);

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(eventfile);
	(*outputevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*outputevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	// add data to output
	outputObject->sendToOutput(outputevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that output was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "output created";

	// get the data
	std::shared_ptr<json::Object> senthypo = GetDataFromFile(hypofile);
	std::shared_ptr<json::Object> outputdetection = GetDataFromString(
			outputObject->messages[0]);

	// check the output data against the input
	CheckData(senthypo, outputdetection);
}

TEST(Output, UpdateTest) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string event2file = std::string(TESTPATH) + "/"
			+ std::string(EVENT2FILE);
	std::string hypo2file = std::string(TESTPATH) + "/"
			+ std::string(HYPO2FILE);
	time_t tNow;
	std::time(&tNow);

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(event2file);
	(*outputevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*outputevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	// add data to output
	outputObject->sendToOutput(outputevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that output was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "output created";

	// get the data
	std::shared_ptr<json::Object> senthypo2 = GetDataFromFile(hypo2file);
	std::shared_ptr<json::Object> output2detection = GetDataFromString(
			outputObject->messages[0]);

	// check the output data against the update
	CheckData(senthypo2, output2detection);

	std::string event2updatefile = std::string(TESTPATH) + "/"
			+ std::string(EVENT2UPDATEFILE);
	std::string hypo2updatefile = std::string(TESTPATH) + "/"
			+ std::string(HYPO2UPDATEFILE);
	std::time(&tNow);

	std::shared_ptr<json::Object> updateevent = GetDataFromFile(
			event2updatefile);
	(*updateevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*updateevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	// send update to output
	outputObject->sendToOutput(updateevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(10));

	// assert that update was created
	ASSERT_EQ(outputObject->messages.size(), 2)<< "update created";

	// get the data
	std::shared_ptr<json::Object> sentupdatehypo2 = GetDataFromFile(
			hypo2updatefile);
	std::shared_ptr<json::Object> outputupdatedetection2 = GetDataFromString(
			outputObject->messages[1]);

	// check the output data against the update
	CheckData(sentupdatehypo2, outputupdatedetection2);

	std::string event2update2file = std::string(TESTPATH) + "/"
			+ std::string(EVENT2UPDATE2FILE);

	std::time(&tNow);

	std::shared_ptr<json::Object> updateevent2 = GetDataFromFile(
			event2update2file);
	(*updateevent2)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*updateevent2)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	// send update to output
	outputObject->sendToOutput(updateevent2);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that second update was not created
	ASSERT_EQ(outputObject->messages.size(), 2)<< "update2 not created";

}

TEST(Output, CancelTest) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string event3file = std::string(TESTPATH) + "/"
			+ std::string(EVENT3FILE);
	std::string cancel3file = std::string(TESTPATH) + "/"
			+ std::string(CANCEL3FILE);
	time_t tNow;
	std::time(&tNow);

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(event3file);
	(*outputevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*outputevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	std::shared_ptr<json::Object> cancelmessage = GetDataFromFile(cancel3file);

	// add data to output
	outputObject->sendToOutput(outputevent);

	// send cancel to output
	outputObject->sendToOutput(cancelmessage);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that the file is not there
	ASSERT_EQ(outputObject->messages.size(), 0)<< "no output";
}

TEST(Output, RetractTest) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string event3file = std::string(TESTPATH) + "/"
			+ std::string(EVENT3FILE);
	std::string cancel3file = std::string(TESTPATH) + "/"
			+ std::string(CANCEL3FILE);
	time_t tNow;
	std::time(&tNow);

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(event3file);
	(*outputevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*outputevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	std::shared_ptr<json::Object> cancelmessage = GetDataFromFile(cancel3file);

	// add data to output
	outputObject->sendToOutput(outputevent);

	// give time for output to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that output was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "output created";

	// send cancel to output
	outputObject->sendToOutput(cancelmessage);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that output was created
	ASSERT_EQ(outputObject->messages.size(), 2)<< "retract created";
}

TEST(Output, ExpireTest) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);
	outputObject->setReportInterval(2);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string event2file = std::string(TESTPATH) + "/"
			+ std::string(EVENT2FILE);

	time_t tNow;
	std::time(&tNow);

	std::shared_ptr<json::Object> outputevent = GetDataFromFile(event2file);
	(*outputevent)["CreateTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);
	(*outputevent)["ReportTime"] =
			glass3::util::Date::convertEpochTimeToISO8601(tNow);

	// add data to output
	outputObject->sendToOutput(outputevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(4));

	// assert that output was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "output created";

	//remove output for update
	// std::remove(output2file.c_str());

	std::string event2expirefile = std::string(TESTPATH) + "/"
			+ std::string(EVENT2EXPIREFILE);
	std::string hypo2updatefile = std::string(TESTPATH) + "/"
			+ std::string(HYPO2UPDATEFILE);
	std::time(&tNow);

	std::shared_ptr<json::Object> expireevent = GetDataFromFile(
			event2expirefile);

	// send update to output
	outputObject->sendToOutput(expireevent);

	// give time for file to write
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that update was created
	ASSERT_EQ(outputObject->messages.size(), 2)<< "expire created";

	// get the data
	std::shared_ptr<json::Object> sentexpirehypo2 = GetDataFromFile(
			hypo2updatefile);
	std::shared_ptr<json::Object> outputexpiredetection2 = GetDataFromString(
			outputObject->messages[1]);

	// check the output data against the update
	CheckData(sentexpirehypo2, outputexpiredetection2);
}

TEST(Output, StationRequestTest) {
	// glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start input thread
	outputObject->start();

	std::string stationrequestfile = std::string(TESTPATH) + "/"
			+ std::string(SITEREQUESTFILE);
//	std::string hypo2file = std::string(TESTPATH) + "/"
//			+ std::string(HYPO2FILE);

	std::shared_ptr<json::Object> stationrequest = GetDataFromFile(
			stationrequestfile);

	// send request to output
	outputObject->sendToOutput(stationrequest);

	// give time for request to convert
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that request was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "request created";
}

TEST(Output, StationListTest) {
	// glass3::util::log_init("outputtest", "debug", std::string(TESTPATH), true);

	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// set the site list request interval short so it happens during the test
	outputObject->setSiteListRequestInterval(2);

	// start output thread
	outputObject->start();

	// give time sitelist to request and generate
	std::this_thread::sleep_for(std::chrono::seconds(3));

	// assert that sitelist was created
	ASSERT_EQ(outputObject->messages.size(), 1)<< "site list created";
}

TEST(Output, FailTests) {
	//glass3::util::log_init("outputtest", spdlog::level::debug, std::string(TESTPATH),
	//					true);
	OutputStub* outputObject = new OutputStub();

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * OutputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> OutputJSON = OutputConfig->getJSON();

	AssociatorStub * AssocThread = new AssociatorStub();
	AssocThread->Output = outputObject;
	outputObject->setAssociator(AssocThread);

	// assert config successful
	ASSERT_TRUE(outputObject->setup(OutputJSON))<< "output config is successful";

	// start output thread
	outputObject->start();

	// null requests
	outputObject->sendToOutput(NULL);
	outputObject->writeOutput(NULL);
	outputObject->isDataReady(NULL);
	outputObject->isDataChanged(NULL);
	outputObject->isDataPublished(NULL);
	outputObject->isDataFinished(NULL);

	// bad requests
	std::shared_ptr<json::Object> badData = GetDataFromString(
			std::string(CONFIGFAIL2));
	outputObject->sendToOutput(badData);
	outputObject->writeOutput(badData);
	outputObject->isDataReady(badData);
	outputObject->isDataChanged(badData);
	outputObject->isDataPublished(badData);
	outputObject->isDataFinished(badData);

	std::shared_ptr<json::Object> badData2 = GetDataFromString(
			std::string(CONFIGFAIL1));
	outputObject->sendToOutput(badData2);
}
