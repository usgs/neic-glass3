#include <gtest/gtest.h>
#include <fileInput.h>
#include <config.h>
#include <fileutil.h>

#include <string>
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#endif

#define CONFIGFILENAME "inputtest.d"
#define TESTPATH "testdata"
#define TESTDATAPATH "inputtests"
#define ERRORDIRECTORY "error"
#define ARCHIVEDIRECTORY "archive"

#define SLEEPTIME 100
#define FILESLEEPTIME 5
#define QUEUEMAXSIZE 1000
#define TESTAGENCYID "US"
#define TESTAUTHOR "glassConverter"
#define DATACOUNT 6

#define CCFILE "example.dat"
#define GPICKFILE "example.gpick"
#define JSONPICKFILE "example.jsonpick"
#define JSONCORLFILE "example.jsoncorl"
#define JSONORIGFILE "example.jsondetect"

#define EXPECTEDDATA "{\"Amplitude\":{\"Amplitude\":0.000000,\"Period\":0.000000,\"SNR\":4.630000},\"Filter\":[{\"HighPass\":0.500000,\"LowPass\":4.000000}],\"ID\":\"22637615\",\"Phase\":\"P\",\"Picker\":\"raypicker\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"SHZ\",\"Location\":\"FB\",\"Network\":\"IM\",\"Station\":\"PDAR\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-03-02T23:59:03.849Z\",\"Type\":\"Pick\"}"  // NOLINT

class InputTest : public ::testing::Test {
 protected:
	virtual void SetUp() {
		// create input test
		InputThread = new glass3::fileInput();
		InputConfig = NULL;
		input_config_json = NULL;

		// glass3::util::log_init("inputtest", spdlog::level::debug, true, testpath);

		int nError2 = 0;

		testpath = std::string(TESTPATH);
		testdatapath = std::string(TESTDATAPATH);
		std::string archive = std::string(ARCHIVEDIRECTORY);

		// create archive and error directories
		configdirectory = "./" + testpath;
		inputdirectory = "./" + testpath + "/" + testdatapath;
		archivedirectory = "./" + testpath + "/" + testdatapath + "/" + archive;

		// create testing directories
#ifdef _WIN32
		nError2 = _mkdir(archivedirectory.c_str());
#else
		mode_t nMode = 0733;
		nError2 = mkdir(archivedirectory.c_str(), nMode);
#endif

		if (nError2 != 0) {
			printf("Failed to create testing archive directory %s.\n",
					archivedirectory.c_str());
		}

		// build file names
		// ccfile = archivedirectory + "/" + std::string(CCFILE);
		gpickfile = archivedirectory + "/" + std::string(GPICKFILE);
		// jsonpickfile = archivedirectory + "/" + std::string(JSONPICKFILE);
		//  jsoncorlfile = archivedirectory + "/" + std::string(JSONCORLFILE);
		// jsonorigfile = archivedirectory + "/" + std::string(JSONORIGFILE);
		// badfile = errordirectory + "/" + std::string(BADFILE);
	}

	bool configure() {
		// create configfilestring
		std::string configfile = std::string(CONFIGFILENAME);

		// load configuration
		InputConfig = new glass3::util::Config(configdirectory, configfile);

		// get json formatted configuration
		input_config_json = InputConfig->getJSON();

		// configure input
		return (InputThread->setup(input_config_json));
	}

	virtual void TearDown() {
		// need to move the files in error and archive back to input
		// if (std::ifstream(ccfile).good()) {
		// 	glass3::util::moveFileTo(ccfile, inputdirectory);
		// }
		if (std::ifstream(gpickfile).good()) {
			glass3::util::moveFileTo(gpickfile, inputdirectory);
		}
		// if (std::ifstream(jsonpickfile).good()) {
		// 	glass3::util::moveFileTo(jsonpickfile, inputdirectory);
		// }
		// if (std::ifstream(jsoncorlfile).good()) {
		// 	glass3::util::moveFileTo(jsoncorlfile, inputdirectory);
		// }
		// if (std::ifstream(jsonorigfile).good()) {
		// 	glass3::util::moveFileTo(jsonorigfile, inputdirectory);
		// }
		// if (std::ifstream(badfile).good()) {
		// 	glass3::util::moveFileTo(badfile, inputdirectory);
		// }

		// need to clean up error and archive directories
#ifdef _WIN32
		RemoveDirectory(archivedirectory.c_str());
#else
		rmdir(archivedirectory.c_str());
#endif

		// cleanup input thread
		delete (InputThread);
		if (InputConfig != NULL)
			delete (InputConfig);
	}

	glass3::fileInput * InputThread;
	glass3::util::Config * InputConfig;
	std::shared_ptr<const json::Object> input_config_json;

	std::string testpath;
	std::string testdatapath;

	std::string configdirectory;
	std::string inputdirectory;
	// std::string errordirectory;
	std::string archivedirectory;

	// std::string ccfile;
	std::string gpickfile;
	// std::string jsonpickfile;
	// std::string jsoncorlfile;
	// std::string jsonorigfile;
	// std::string badfile;
};

// tests to see if correlation can successfully
// write json output
TEST_F(InputTest, Construction) {
	// assert that this is an input thread
	ASSERT_STREQ(InputThread->getThreadName().c_str(),
			"input")<< "check input thread name";

	// assert the thread sleeptime
	ASSERT_EQ(InputThread->getSleepTime(), SLEEPTIME)
	<< "check input thread sleep time";

	// assert class is not set up
	ASSERT_FALSE(InputThread->getSetup()) << "input thread is not set up";

	// assert class has no config
	ASSERT_TRUE(InputThread->getConfig() == NULL) << "input config is null";

	// assert class is not running
	ASSERT_FALSE(InputThread->getWorkThreadsState() ==
			glass3::util::ThreadState::Started) << "input thread is not running";

	// assert no data in class
	ASSERT_EQ(InputThread->getInputDataCount(), 0) << "input thread has no data";
}

TEST_F(InputTest, Configuration) {
	// configure input
	ASSERT_TRUE(configure())<< "InputThread->setup returned true";

	// assert class is set up
	ASSERT_TRUE(InputThread->getSetup()) << "input thread is set up";

	// assert class has config
	ASSERT_TRUE(InputThread->getConfig() != NULL) << "input config is notnull";

	// check input directory
	ASSERT_STREQ(InputThread->getInputDir().c_str(),
			inputdirectory.c_str()) << "check input thread input directory";

	// check archive path
	ASSERT_STREQ(InputThread->getArchiveDir().c_str(),
			archivedirectory.c_str()) << "check input thread archive directory";

	// check queue max size
	ASSERT_EQ(InputThread->getInputDataMaxSize(), QUEUEMAXSIZE)
	<< "check queue max size";

	// check agency id
	std::string agencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(InputThread->getDefaultAgencyId().c_str(),
			agencyid.c_str()) << "check agency id";

	// check author
	std::string author = std::string(TESTAUTHOR);
	ASSERT_STREQ(InputThread->getDefaultAuthor().c_str(),
			author.c_str()) << "check author";
}

TEST_F(InputTest, Run) {
	// configure input
	ASSERT_TRUE(configure())<< "InputThread->setup returned true";

	// start input thread
	InputThread->start();

	// give time for files to parse
	std::this_thread::sleep_for(std::chrono::seconds(3));

	// assert that the right amount of data is in class
	ASSERT_EQ(InputThread->getInputDataCount(), DATACOUNT) << "input thread has data";

	// check that the files were archived
	// ASSERT_TRUE(std::ifstream(ccfile).good()) << "ccfile archived";
	ASSERT_TRUE(std::ifstream(gpickfile).good()) << "gpickfile archived";
	// ASSERT_TRUE(std::ifstream(jsonpickfile).good()) << "jsonpickfile archived";
	// ASSERT_TRUE(std::ifstream(jsoncorlfile).good()) << "jsoncorlfile archived";
	// ASSERT_TRUE(std::ifstream(jsonorigfile).good()) << "jsonorigfile archived";
	// ASSERT_TRUE(std::ifstream(badfile).good()) << "badfile errored";

	std::shared_ptr<json::Object> data = InputThread->getInputData();

	// assert input data ok
	ASSERT_TRUE(data != NULL) << "input data is not null";

	// check data
	std::string datastring = json::Serialize(*data);
	std::string expectedstring = std::string(EXPECTEDDATA);
	ASSERT_STREQ(datastring.c_str(),
			expectedstring.c_str()) << "check data";

	// clear data
	InputThread->clear();

	// assert that class is empty
	ASSERT_EQ(InputThread->getInputDataCount(), 0) << "input thread is empty";
}
