#include <gtest/gtest.h>

#include <string>
#include "Pid.h"
#include "Logit.h"

TEST(PidTest, CombinedTest) {
	glassutil::CLogit::disable();

	std::string pid = glassutil::CPid::pid();
	std::string empty = "";

	// test pid generation
	ASSERT_STRNE(pid.c_str(), empty.c_str()) << "Pid generated";

	// test random generation
	double random = glassutil::CPid::random();

	if (random != 0) {
		ASSERT_NE(0, random) << "Random generated";
	}
}
