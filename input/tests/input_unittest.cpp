#include <logger.h>
#include <input.h>
#include <config.h>
#include <gtest/gtest.h>

#define TESTPATH "testdata"
#define CONFIGFILENAME "inputtest.d"
#define GPICKFILENAME "test_gpick.gpick"
#define CCFILENAME "test_cc.dat"
#define JSONFILENAME "test_json.jsonpick"

#define QUEUESIZE 10
#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"
#define DATACOUNT 5

// glass3::input::Input is an abstract class and
// must be derived into a concrete class before use.
class inputStub : public glass3::input::Input {
 public:
	inputStub()
			: glass3::input::Input() {
		m_bFileProcessed = false;
	}

	~inputStub() {
	}

	std::string m_DataType;
	bool m_bFileProcessed;

 protected:
	std::string fetchRawData(std::string* type) override {
		*type = m_DataType;

		if (m_bFileProcessed == true) {
			return("");
		}

		if ((m_InputFile.good() == true) && (m_InputFile.eof() != true)) {
			std::string line = "";

			// we're processing an input file, get the next line
			std::getline(m_InputFile, line);

			return (line);
		} else {
			if (m_InputFile.is_open()) {
				m_InputFile.close();
				m_bFileProcessed = true;
			}

			std::string fileName;

			if (m_DataType == std::string(GPICK_TYPE)) {
				fileName = "./" + std::string(TESTPATH) + "/"
						+ std::string(GPICKFILENAME);
			} else if (m_DataType == std::string(CC_TYPE)) {
				fileName = "./" + std::string(TESTPATH) + "/"
						+ std::string(CCFILENAME);
			} else if (m_DataType == std::string(JSON_TYPE)) {
				fileName = "./" + std::string(TESTPATH) + "/"
						+ std::string(JSONFILENAME);
			}

			m_InputFile.open(fileName, std::ios::in);
			return ("");
		}
	}
 private:
	std::ifstream m_InputFile;
};

// tests to see input can be constructed
TEST(InputTest, Construction) {
	inputStub TestInput;

	// assert that m_AgencyId is empty
	ASSERT_STREQ(TestInput.getDefaultAgencyId().c_str(), "")<<
	"AgencyID check";

	// assert that m_Author is empty
	ASSERT_STREQ(TestInput.getDefaultAuthor().c_str(), "")<< "Author check";

	// assert queue max size is -1
	ASSERT_EQ(TestInput.getQueueMaxSize(), -1)<< "queue max size check";
}

// tests to see if the input can be configured
TEST(InputTest, Configuration) {
	inputStub TestInput;

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * InputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> InputJSON = InputConfig->getJSON();

	TestInput.setup(InputJSON);

	// assert that m_AgencyId is not empty
	ASSERT_STREQ(TestInput.getDefaultAgencyId().c_str(),
			std::string(TESTAGENCYID).c_str())<< "AgencyID check";

	// assert that m_Author is not empty
	ASSERT_STREQ(TestInput.getDefaultAuthor().c_str(),
			std::string(TESTAUTHOR).c_str())<< "Author check";

	// assert queue max size is -1
	ASSERT_EQ(TestInput.getQueueMaxSize(), QUEUESIZE)<< "queue max size check";
}

// tests to see if input can process gicks
TEST(InputTest, GPickTest) {
	inputStub TestInput;

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * InputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> InputJSON = InputConfig->getJSON();

	TestInput.setup(InputJSON);
	TestInput.m_DataType = std::string(GPICK_TYPE);

	// start input
	TestInput.start();

	// wait a bit for the file to process
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// check that the right ammount of data is in the queue
	ASSERT_EQ(TestInput.getInputDataCount(), DATACOUNT)<< "queue size check";
}

// tests to see if input can process ccData
TEST(InputTest, CCTest) {
	inputStub TestInput;

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * InputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> InputJSON = InputConfig->getJSON();

	TestInput.setup(InputJSON);
	TestInput.m_DataType = std::string(CC_TYPE);

	// start input
	TestInput.start();

	// wait a bit for the file to process
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// check that the right ammount of data is in the queue
	ASSERT_EQ(TestInput.getInputDataCount(), DATACOUNT)<< "queue size check";
}

// tests to see if input can process json data
TEST(InputTest, JSONTest) {
	inputStub TestInput;

	// create configfilestring
	std::string configfile = std::string(CONFIGFILENAME);
	std::string configdirectory = std::string(TESTPATH);

	// load configuration
	glass3::util::Config * InputConfig = new glass3::util::Config(
			configdirectory, configfile);
	std::shared_ptr<const json::Object> InputJSON = InputConfig->getJSON();

	TestInput.setup(InputJSON);
	TestInput.m_DataType = std::string(JSON_TYPE);

	// start input
	TestInput.start();

	// wait a bit for the file to process
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// check that the right ammount of data is in the queue
	ASSERT_EQ(TestInput.getInputDataCount(), DATACOUNT)<< "queue size check";
}
