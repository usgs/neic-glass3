#include <gtest/gtest.h>
#include <output.h>
#include <config.h>
#include <logger.h>
#include <timeutil.h>

#include <string>
#include <ctime>
#include <memory>

#define EMPTYCONFIG "{\"Cmd\":\"GlassOutput\"}"
#define CONFIGFAIL1 "{\"PublicationTimes\":[3, 6]}"

#define TRACKING1 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Pid\":\"7D52AC725BE6FA478A16EC918B80E271\",\"Version\":1}" // NOLINT
#define TRACKING2 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"ID\":\"7D52AC725BE6FA478A16EC918B80E271\",\"Version\":1}" // NOLINT
#define TRACKING3 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"ID\":\"9DD2AC7HJBE6FA478AJMPC2112805098\",\"Version\":1}" // NOLINT
#define ID1 "7D52AC725BE6FA478A16EC918B80E271"
#define ID3 "9DD2AC7HJBE6FA478AJMPC2112805098"

#define BADTRACKING1 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Pid\":\"\",\"Version\":1}" // NOLINT
#define BADTRACKING2 "{\"Bayes\":16.790485,\"Cmd\":\"Event\",\"Version\":1}"

class OutputStub : public glass::output {
 public:
	OutputStub() {
	}

	virtual ~OutputStub() {
	}

	void sendOutput(const std::string &type, const std::string &id,
					const std::string &message) {
	}
};

// NOTE that other aspects of output are tested in the file_output tests that
// are part of glass-app

// tests to see if remove_chars is functional
TEST(Output, TrackingTests) {
	// create output stub
	OutputStub outputThread;
	std::shared_ptr<json::Object> nullTrack;

	time_t tNow;
	std::time(&tNow);

	// setup
	ASSERT_FALSE(outputThread.setup(NULL));
	ASSERT_FALSE(
			outputThread.setup(new json::Object(json::Deserialize(CONFIGFAIL1))));  // NOLINT
	ASSERT_TRUE(
			outputThread.setup(new json::Object(json::Deserialize(EMPTYCONFIG))));  // NOLINT

	std::shared_ptr<json::Object> tracking1 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING1)));

	(*tracking1)["CreateTime"] = glass3::util::convertEpochTimeToISO8601(tNow);
	(*tracking1)["ReportTime"] = glass3::util::convertEpochTimeToISO8601(tNow);

	std::shared_ptr<json::Object> tracking2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING2)));

	(*tracking2)["CreateTime"] = glass3::util::convertEpochTimeToISO8601(tNow);
	(*tracking2)["ReportTime"] = glass3::util::convertEpochTimeToISO8601(tNow);

	std::shared_ptr<json::Object> tracking3 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(TRACKING3)));

	(*tracking3)["CreateTime"] = glass3::util::convertEpochTimeToISO8601(tNow);
	(*tracking3)["ReportTime"] = glass3::util::convertEpochTimeToISO8601(tNow);

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

