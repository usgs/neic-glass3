#include <gtest/gtest.h>

#include <logger.h>

#include "TimeWarp.h"

#define GRIDMINIMUM 0.0
#define GRIDMAXIMUM 360.0
#define DECAYCONSTANT 0.10
#define SLOPEZERO 0.05
#define SLOPEINFINITY 1.0
#define GRIDINDEX 66.6392
#define GRIDVALUE 4

// tests to see if the timewarp can be constructed
TEST(TimeWarpTest, Construction) {
	glass3::util::Logger::disable();

	// construct a timewarp
	traveltime::CTimeWarp timeWarp(GRIDMINIMUM, GRIDMAXIMUM, DECAYCONSTANT,
	SLOPEZERO,
									SLOPEINFINITY);

	// setup?
	ASSERT_TRUE(timeWarp.bSetup);

	// dGridMinimum
	ASSERT_EQ(GRIDMINIMUM, timeWarp.dGridMinimum)<< "Grid Minimum Check";

	// dGridMaximum
	ASSERT_EQ(GRIDMAXIMUM, timeWarp.dGridMaximum)<< "Grid Maximum Check";

	// dDecayConstant
	ASSERT_EQ(DECAYCONSTANT, timeWarp.dDecayConstant)<< "Decay Constant Check";

	// dSlopeZero
	ASSERT_EQ(SLOPEZERO, timeWarp.dSlopeZero)<< "Slope Zero Check";

	// dSlopeInfinity
	ASSERT_EQ(SLOPEINFINITY, timeWarp.dSlopeInfinity)<< "Slope Infinity Check";
}

// tests the time warp copy constructor
TEST(TimeWarpTest, Copy) {
	glass3::util::Logger::disable();

	// construct a timewarp
	traveltime::CTimeWarp timeWarp1;

	// setup?
	ASSERT_FALSE(timeWarp1.bSetup);

	// dGridMinimum
	ASSERT_EQ(0, timeWarp1.dGridMinimum)<< "Grid Minimum Check";

	// dGridMaximum
	ASSERT_EQ(0, timeWarp1.dGridMaximum)<< "Grid Maximum Check";

	// dDecayConstant
	ASSERT_EQ(0, timeWarp1.dDecayConstant)<< "Decay Constant Check";

	// dSlopeZero
	ASSERT_EQ(0, timeWarp1.dSlopeZero)<< "Slope Zero Check";

	// dSlopeInfinity
	ASSERT_EQ(0, timeWarp1.dSlopeInfinity)<< "Slope Infinity Check";

	// construct a second timewarp
	traveltime::CTimeWarp timeWarp2(GRIDMINIMUM, GRIDMAXIMUM, DECAYCONSTANT,
	SLOPEZERO,
									SLOPEINFINITY);
	// copy
	timeWarp1 = traveltime::CTimeWarp(timeWarp2);

	// setup?
	ASSERT_TRUE(timeWarp1.bSetup);

	// dGridMinimum
	ASSERT_EQ(GRIDMINIMUM, timeWarp1.dGridMinimum)<< "Grid Minimum Check";

	// dGridMaximum
	ASSERT_EQ(GRIDMAXIMUM, timeWarp1.dGridMaximum)<< "Grid Maximum Check";

	// dDecayConstant
	ASSERT_EQ(DECAYCONSTANT, timeWarp1.dDecayConstant)<< "Decay Constant Check";

	// dSlopeZero
	ASSERT_EQ(SLOPEZERO, timeWarp1.dSlopeZero)<< "Slope Zero Check";

	// dSlopeInfinity
	ASSERT_EQ(SLOPEINFINITY, timeWarp1.dSlopeInfinity)<< "Slope Infinity Check";
}

// tests the time warp operations
TEST(TimeWarpTest, Operations) {
	glass3::util::Logger::disable();

	// construct a timewarp
	traveltime::CTimeWarp timeWarp(GRIDMINIMUM, GRIDMAXIMUM, DECAYCONSTANT,
	SLOPEZERO,
									SLOPEINFINITY);

	ASSERT_NEAR(GRIDINDEX, timeWarp.grid(GRIDVALUE), 0.0001)<< "Grid Index Check";

	ASSERT_NEAR(GRIDVALUE, timeWarp.value(GRIDINDEX), 0.001)<< "Grid Value Check";
}
