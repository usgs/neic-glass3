#include <gtest/gtest.h>

#include <string>

#include <logger.h>

#include "TravelTime.h"

#define TESTPATH "testdata"
#define PHASE "P"
#define PHASEFILENAME "P.trv"

#define NDISTANCEWARP 550
#define NDEPTHWARP 105

#define LATITUDE 0.0
#define LONGITUDE 0.0
#define DEPTH 50.0
#define DISTANCE 50.0
#define TIME 529.2
#define TIME2 50.553
#define BILINEAR 50.553

// tests to see if the traveltime can be constructed
TEST(TravelTimeTest, Construction) {
	glass3::util::Logger::disable();

	// construct a traveltime
	traveltime::CTravelTime traveltime;

	// nDistanceWarp
	ASSERT_EQ(0, traveltime.nDistanceWarp)<< "nDistanceWarp Check";

	// nDepthWarp
	ASSERT_EQ(0, traveltime.nDepthWarp)<< "nDepthWarp Check";

	// dDepth
	ASSERT_EQ(0, traveltime.dDepth)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime.dDelta)<< "Delta Check";

	// pointers
	ASSERT_EQ(NULL, traveltime.pDistanceWarp)<< "pDistanceWarp null";
	ASSERT_EQ(NULL, traveltime.pDepthWarp)<< "pDepthWarp null";
	ASSERT_EQ(NULL, traveltime.pTravelTimeArray)<< "pTravelTimeArray null";
	ASSERT_EQ(NULL, traveltime.pDepthDistanceArray)<< "pDepthDistanceArray null";
	ASSERT_EQ(NULL, traveltime.pPhaseArray)<< "pPhaseArray null";
}

// tests to see if the traveltime can be setup
TEST(TravelTimeTest, Setup) {
	glass3::util::Logger::disable();

	std::string phasefile = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASEFILENAME);
	std::string phasename = std::string(PHASE);

	// construct a traveltime
	traveltime::CTravelTime traveltime;

	// setup
	traveltime.setup(phasename, phasefile);

	// phase name
	ASSERT_STREQ(traveltime.sPhase.c_str(), phasename.c_str());

	// nDistanceWarp
	ASSERT_EQ(NDISTANCEWARP, traveltime.nDistanceWarp)<< "nDistanceWarp Check";

	// nDepthWarp
	ASSERT_EQ(NDEPTHWARP, traveltime.nDepthWarp)<< "nDepthWarp Check";

	// dDepth
	ASSERT_EQ(0, traveltime.dDepth)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime.dDelta)<< "Delta Check";

	// pointers
	ASSERT_TRUE(NULL != traveltime.pDistanceWarp)<< "pDistanceWarp not null";
	ASSERT_TRUE(NULL != traveltime.pDepthWarp)<< "pDepthWarp not null";
	ASSERT_TRUE(NULL != traveltime.pTravelTimeArray)<< "pTravelTimeArray not "
			"null";
	ASSERT_TRUE(NULL != traveltime.pDepthDistanceArray)<< "pDepthDistanceArray "
			"not null";
	ASSERT_TRUE(NULL != traveltime.pPhaseArray)<< "pPhaseArray not null";
}

// tests the time warp copy constructor
TEST(TravelTimeTest, Copy) {
	glass3::util::Logger::disable();

	std::string phasefile = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASEFILENAME);
	std::string phasename = std::string(PHASE);

	// construct a second traveltime
	traveltime::CTravelTime traveltime2;

	// setup
	traveltime2.setup(phasename, phasefile);

	// set origin
	traveltime2.setOrigin(LATITUDE, LONGITUDE, DEPTH);

	// copy
	traveltime::CTravelTime traveltime1(traveltime2);

	// phase name
	ASSERT_STREQ(traveltime1.sPhase.c_str(), phasename.c_str());

	// nDistanceWarp
	ASSERT_EQ(NDISTANCEWARP, traveltime1.nDistanceWarp)<< "nDistanceWarp Check";

	// nDepthWarp
	ASSERT_EQ(NDEPTHWARP, traveltime1.nDepthWarp)<< "nDepthWarp Check";

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime1.dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime1.dDelta)<< "Delta Check";

	// pointers
	ASSERT_TRUE(NULL != traveltime1.pDistanceWarp)<< "pDistanceWarp not null";
	ASSERT_TRUE(NULL != traveltime1.pDepthWarp)<< "pDepthWarp not null";
	ASSERT_TRUE(NULL != traveltime1.pTravelTimeArray)<< "pTravelTimeArray not "
			"null";
	ASSERT_TRUE(NULL != traveltime1.pDepthDistanceArray)<< "pDepthDistanceArray "
			"not null";
	ASSERT_TRUE(NULL != traveltime1.pPhaseArray)<< "pPhaseArray not null";
}

// tests traveltime operations
TEST(TravelTimeTest, Operations) {
	glass3::util::Logger::disable();

	std::string phasefile = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASEFILENAME);
	std::string phasename = std::string(PHASE);

	// construct a traveltime
	traveltime::CTravelTime traveltime;

	// setup
	traveltime.setup(phasename, phasefile);

	// set origin
	traveltime.setOrigin(LATITUDE, LONGITUDE, DEPTH);

	// T(delta)
	ASSERT_NEAR(TIME, traveltime.T(DISTANCE), 0.001)<< "T(delta) Check";

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime.dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_NEAR(DISTANCE, traveltime.dDelta, 0.001)<< "Delta Check";

	glass3::util::Geo testGeo;
	testGeo.setGeographic(LATITUDE, LONGITUDE + DISTANCE, DEPTH);

	// T(geo)
	ASSERT_NEAR(TIME, traveltime.T(&testGeo), 0.001)<< "T(geo) Check";

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime.dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_NEAR(DISTANCE, traveltime.dDelta, 0.001)<< "Delta Check";

	// T(delta, distance)
	ASSERT_NEAR(TIME2, traveltime.T(DISTANCE,DEPTH), 0.001)<< "T(delta, distance) Check"; // NOLINT

	// bilinear
	ASSERT_NEAR(BILINEAR, traveltime.bilinear(DISTANCE,DEPTH), 0.001)<< "bilinear Check"; // NOLINT
}
