#include <gtest/gtest.h>

#include <string>
#include "logger.h"

// tests to see if the logger can be disabled
TEST(LoggerTest, EnableDisable) {
	glass3::util::Logger::disable();

	// bDisable
	ASSERT_TRUE(glass3::util::Logger::m_bDisable)<< "bDisable Check";

	glass3::util::Logger::enable();

	// bDisable
	ASSERT_FALSE(glass3::util::Logger::m_bDisable)<< "bDisable Check";
}

// tests to see if the logger can be used
TEST(LoggerTest, Logging) {
	glass3::util::Logger::log("log test");

	glass3::util::Logger::log(std::string("log string test"));

	glass3::util::Logger::log("info", "log with level test");
}
