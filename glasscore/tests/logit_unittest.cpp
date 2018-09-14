#include <gtest/gtest.h>

#include <string>
#include "Logit.h"

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

	glassutil::CLogit::log(std::string("log string test"));

	glassutil::CLogit::log(glassutil::log_level::info, "log with level test");
}
