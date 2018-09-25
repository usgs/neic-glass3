#include <gtest/gtest.h>

#include <string>
#include "glassid.h"
#include "logger.h"

TEST(GlassIDTest, CombinedTest) {
	glass3::util::Logger::disable();

	std::string id = glass3::util::GlassID::getID();
	std::string empty = "";

	// test id generation
	ASSERT_STRNE(id.c_str(), empty.c_str()) << "id generated";

	// test random generation
	double random = glass3::util::GlassID::random();

	if (random != 0) {
		ASSERT_NE(0, random) << "Random generated";
	}
}
