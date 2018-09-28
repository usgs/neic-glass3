#include <gtest/gtest.h>

#include "taper.h"
#include "logger.h"

#define TESTPATH "testdata"
#define PHASE "P"
#define PHASEFILENAME "P.trv"

#define NDISTANCEWARP 550
#define NDEPTHWARP 105

#define X1 0.0
#define X2 0.25
#define X3 0.25
#define X4 1.0

#define calculateValue1 0.6545
#define calculateValue2 0.9567

// tests to see if the taper can be constructed
TEST(TaperTest, Construction) {
	glass3::util::Logger::disable();

	glass3::util::Taper taper;

	// dX1
	ASSERT_NEAR(0.0, taper.m_dX1, 0.001)<< "dX1 Default Check";

	// dX2
	ASSERT_NEAR(0.5, taper.m_dX2, 0.001)<< "dX2 Default Check";

	// dX3
	ASSERT_NEAR(0.5, taper.m_dX3, 0.001)<< "dX3 Default Check";

	// dX4
	ASSERT_NEAR(1.0, taper.m_dX4, 0.001)<< "dX4 Default Check";

	glass3::util::Taper taper2(X1, X2, X3, X4);

	// dX1
	ASSERT_NEAR(X1, taper2.m_dX1, 0.001)<< "dX1 Check";

	// dX2
	ASSERT_NEAR(X2, taper2.m_dX2, 0.001)<< "dX2 Check";

	// dX3
	ASSERT_NEAR(X3, taper2.m_dX3, 0.001)<< "dX3 Check";

	// dX4
	ASSERT_NEAR(X4, taper2.m_dX4, 0.001)<< "dX4 Check";
}

// tests to see if the taper can be copied
TEST(TaperTest, Copy) {
	glass3::util::Logger::disable();

	glass3::util::Taper taper2(X1, X2, X3, X4);

	glass3::util::Taper taper(taper2);

	// dX1
	ASSERT_NEAR(X1, taper.m_dX1, 0.001)<< "dX1 Check";

	// dX2
	ASSERT_NEAR(X2, taper.m_dX2, 0.001)<< "dX2 Check";

	// dX3
	ASSERT_NEAR(X3, taper.m_dX3, 0.001)<< "dX3 Check";

	// dX4
	ASSERT_NEAR(X4, taper.m_dX4, 0.001)<< "dX4 Check";
}

TEST(TaperTest, calculatecalculateValueue) {
	glass3::util::Logger::disable();

	glass3::util::Taper taper(X1, X2, X3, X4);

	ASSERT_EQ(0, taper.calculateValue(X1 - 1))<<
			"calculateValue X less than X1";

	ASSERT_EQ(0, taper.calculateValue(X4 + 1))<<
			"calculateValue X greater than X4";

	ASSERT_NEAR(1, taper.calculateValue(X2), 0.001)<<
			"calculateValue X at X2";

	ASSERT_NEAR(calculateValue1, taper.calculateValue(X2 - 0.1), 0.001)<<
			"calculateValue X between X1 and X2";

	ASSERT_NEAR(calculateValue2, taper.calculateValue(X2 + 0.1), 0.001)<<
			"calculateValue X between X3 and X4";
}
