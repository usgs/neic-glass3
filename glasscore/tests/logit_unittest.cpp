#include <gtest/gtest.h>

#include <string>
#include "Logit.h"

void logTest(glassutil::logMessageStruct message) {
	printf("%s\n", message.message.c_str());
}

// tests to see if the traveltime can be constructed
TEST(LogitTest, EnableDisable) {
	glassutil::CLogit::disable();

	// bDisable
	ASSERT_TRUE(glassutil::CLogit::bDisable)<< "bDisable Check";

	glassutil::CLogit::enable();

	// bDisable
	ASSERT_FALSE(glassutil::CLogit::bDisable)<< "bDisable Check";
}

TEST(LogitTest, Logging) {
	glassutil::CLogit::Out("out test");

	glassutil::CLogit::log("log test");

	glassutil::CLogit::log(glassutil::log_level::info, "log with level test");
}

TEST(LogitTest, Callback) {
	glassutil::CLogit::setLogCallback(
				std::bind(logTest, std::placeholders::_1));

	glassutil::CLogit::log(glassutil::log_level::info, "callback test");
}
