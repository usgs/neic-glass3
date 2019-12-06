#include <gtest/gtest.h>

#include <string>

#include <logger.h>

#include "TravelTime.h"

#define TESTPATH "testdata"
#define PHASE "P"
#define PHASEFILENAME "P.trv"

#define NDISTANCES 720
#define MINDIST 0.0
#define MAXDIST 180.0

#define NDEPTHS 160
#define MINDEPTH 0.0
#define MAXDEPTH 800.0

#define LATITUDE 0.0
#define LONGITUDE 0.0
#define DEPTH 50.0
#define DISTANCE 50.0
#define DELTATIME 529.2172
#define GEOTIME 527.31964
#define TIME2 169.71368
#define BILINEAR 169.71368

// tests to see if the traveltime can be constructed
TEST(TravelTimeTest, Construction) {
	glass3::util::Logger::disable();

	// construct a traveltime
	traveltime::CTravelTime traveltime;

	// m_iNumDistances
	ASSERT_EQ(0, traveltime.m_iNumDistances)<< "m_iNumDistances Check";

	// m_dMinimumDistance
	ASSERT_EQ(0, traveltime.m_dMinimumDistance)<< "m_dMinimumDistance Check";

	// m_dMaximumDistance
	ASSERT_EQ(0, traveltime.m_dMaximumDistance)<< "m_dMaximumDistance Check";

	// m_iNumDepths
	ASSERT_EQ(0, traveltime.m_iNumDepths)<< "m_iNumDepths Check";	

	// m_dMinimumDepth
	ASSERT_EQ(0, traveltime.m_dMinimumDepth)<< "m_dMinimumDepth Check";	

		// m_dMaximumDepth
	ASSERT_EQ(0, traveltime.m_dMaximumDepth)<< "m_dMaximumDepth Check";	

	// dDepth
	ASSERT_EQ(0, traveltime.m_dDepth)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime.m_dDelta)<< "Delta Check";

	// pointers
	ASSERT_EQ(NULL, traveltime. m_pTravelTimeArray)<< "pTravelTimeArray null";
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
	ASSERT_STREQ(traveltime.m_sPhase.c_str(), phasename.c_str());

	// m_iNumDistances
	ASSERT_EQ(NDISTANCES, traveltime.m_iNumDistances)<< "m_iNumDistances Check";

	// m_dMinimumDistance
	ASSERT_NEAR(MINDIST, traveltime.m_dMinimumDistance, .001)<< "m_dMinimumDistance Check";	

	// m_dMaximumDistance
	ASSERT_NEAR(MAXDIST, traveltime.m_dMaximumDistance, .001)<< "m_dMaximumDistance Check";	

	// m_iNumDepths
	ASSERT_EQ(NDEPTHS, traveltime.m_iNumDepths)<< "m_iNumDepths Check";

	// m_dMinimumDepth
	ASSERT_NEAR(MINDEPTH, traveltime.m_dMinimumDepth, .001)<< "m_dMinimumDepth Check";	

		// m_dMaximumDepth
	ASSERT_NEAR(MAXDEPTH, traveltime.m_dMaximumDepth, .001)<< "m_dMaximumDepth Check";	

	// dDepth
	ASSERT_EQ(0, traveltime.m_dDepth)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime.m_dDelta)<< "Delta Check";

	// pointers
	ASSERT_TRUE(NULL != traveltime. m_pTravelTimeArray)<< "pTravelTimeArray not "
			"null";
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
	traveltime2.setTTOrigin(LATITUDE, LONGITUDE, DEPTH);

	// copy
	traveltime::CTravelTime traveltime1(traveltime2);

	// phase name
	ASSERT_STREQ(traveltime1.m_sPhase.c_str(), phasename.c_str());

	// m_iNumDistances
	ASSERT_EQ(NDISTANCES, traveltime1.m_iNumDistances)<< "m_iNumDistances Check";

	// m_dMinimumDistance
	ASSERT_NEAR(MINDIST, traveltime1.m_dMinimumDistance, .001)<< "m_dMinimumDistance Check";	

	// m_dMaximumDistance
	ASSERT_NEAR(MAXDIST, traveltime1.m_dMaximumDistance, .001)<< "m_dMaximumDistance Check";	

	// m_iNumDepths
	ASSERT_EQ(NDEPTHS, traveltime1.m_iNumDepths)<< "m_iNumDepths Check";

	// m_dMinimumDepth
	ASSERT_NEAR(MINDEPTH, traveltime1.m_dMinimumDepth, .001)<< "m_dMinimumDepth Check";	

	// m_dMaximumDepth
	ASSERT_NEAR(MAXDEPTH, traveltime1.m_dMaximumDepth, .001)<< "m_dMaximumDepth Check";	

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime1.m_dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_EQ(0, traveltime1.m_dDelta)<< "Delta Check";

	// pointers
	ASSERT_TRUE(NULL != traveltime1. m_pTravelTimeArray)<< "pTravelTimeArray not "
			"null";
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
	traveltime.setTTOrigin(LATITUDE, LONGITUDE, DEPTH);

	// T(delta)
	ASSERT_NEAR(DELTATIME, traveltime.T(DISTANCE), 0.001)<< "T(delta) Check";

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime.m_dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_NEAR(DISTANCE, traveltime.m_dDelta, 0.001)<< "Delta Check";

	glass3::util::Geo testGeo;
	testGeo.setGeographic(LATITUDE, LONGITUDE + DISTANCE, DEPTH);

	// T(geo)
	ASSERT_NEAR(GEOTIME, traveltime.T(&testGeo), 0.001)<< "T(geo) Check";

	// dDepth
	ASSERT_NEAR(DEPTH, traveltime.m_dDepth, 0.001)<< "Depth Check";

	// dDelta
	ASSERT_NEAR(DISTANCE, traveltime.m_dDelta, 0.001)<< "Delta Check";

	// T(delta, distance)
	ASSERT_NEAR(TIME2, traveltime.T(DISTANCE,DEPTH), 0.001)<< "T(delta, distance) Check"; // NOLINT

	// bilinear
	ASSERT_NEAR(BILINEAR, traveltime.bilinear(DISTANCE,DEPTH), 0.001)<< "bilinear Check"; // NOLINT
}
