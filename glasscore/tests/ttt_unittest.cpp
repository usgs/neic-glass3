#include <gtest/gtest.h>

#include <logger.h>

#include <string>
#include "TTT.h"

#define TESTPATH "testdata"
#define PHASE1 "P"
#define PHASE1FILENAME "P.trv"
#define PHASE2 "S"
#define PHASE2FILENAME "S.trv"

#define LATITUDE 10.0
#define GEOLATITUDE 9.93411
#define LONGITUDE 20.0
#define DEPTH 50.0
#define GEODEPTH 6321.0
#define DISTANCE 20.0
#define BADDISTANCE 160.0
#define BADDEPTH 800
#define TIME1 265.0485
#define TIME2 484.4298
#define TIME3 268.3447
#define TIME4 490.4684
#define BADTIME -1

// tests to see if the ttt can be constructed
TEST(TTTTest, Construction) {
	glass3::util::Logger::disable();

	// construct a traveltime
	traveltime::CTTT ttt;

	// nTrv
	ASSERT_EQ(0, ttt.nTrv)<< "nTrv Check";

	// m_dGeocentricLatitude
	ASSERT_EQ(0, ttt.geoOrg.m_dGeocentricLatitude)<<
			"m_dGeocentricLatitude Check";

	// m_dGeocentricLongitude
	ASSERT_EQ(0, ttt.geoOrg.m_dGeocentricLongitude)<<
			"m_dGeocentricLongitude Check";

	// m_dGeocentricRadius
	ASSERT_EQ(0, ttt.geoOrg.m_dGeocentricRadius)<<
			"m_dGeocentricRadius Check";

	// dWeight
	ASSERT_EQ(0, ttt.dWeight)<< "dWeight Check";
}

// tests to see if phases can be added to the ttt
TEST(TTTTest, AddPhase) {
	glass3::util::Logger::disable();

	std::string phase1file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE1FILENAME);
	std::string phase1name = std::string(PHASE1);

	std::string phase2file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE2FILENAME);
	std::string phase2name = std::string(PHASE2);

	// construct a traveltime
	traveltime::CTTT ttt;

	// set ranges
	double * weightRange = new double[4];
	weightRange[0] = 0;
	weightRange[1] = 0;
	weightRange[2] = 120;
	weightRange[3] = 180;

	double * assocRange = new double[2];
	assocRange[0] = 10;
	assocRange[1] = 90;

	// add first phase
	ttt.addPhase(phase1name, weightRange, NULL, phase1file);

	// nTrv
	ASSERT_EQ(1, ttt.nTrv)<< "nTrv Check";

	// phase name
	ASSERT_STREQ(ttt.pTrv[ttt.nTrv - 1]->sPhase.c_str(), phase1name.c_str());

	// add second phase
	ttt.addPhase(phase2name, NULL, assocRange, phase2file);

	// nTrv
	ASSERT_EQ(2, ttt.nTrv)<< "nTrv Check";

	// phase name
	ASSERT_STREQ(ttt.pTrv[ttt.nTrv - 1]->sPhase.c_str(), phase2name.c_str());

	delete[] (weightRange);
	delete[] (assocRange);
}

// tests to see if set origin works
TEST(TTTTest, SetOrigin) {
	glass3::util::Logger::disable();

	// construct a traveltime
	traveltime::CTTT ttt;

	// call setorigin
	ttt.setOrigin(LATITUDE, LONGITUDE, DEPTH);

	// m_dGeocentricLatitude
	ASSERT_NEAR(GEOLATITUDE, ttt.geoOrg.m_dGeocentricLatitude, 0.001)<<
			"m_dGeocentricLatitude Check";

	// m_dGeocentricLongitude
	ASSERT_EQ(LONGITUDE, ttt.geoOrg.m_dGeocentricLongitude)<<
			"m_dGeocentricLongitude Check";

	// m_dGeocentricRadius
	ASSERT_NEAR(GEODEPTH, ttt.geoOrg.m_dGeocentricRadius, 0.01)<<
			"m_dGeocentricRadius Check";
}

// tests to see if copy constructor works
TEST(TTTTest, Copy) {
	glass3::util::Logger::disable();

	std::string phase1file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE1FILENAME);
	std::string phase1name = std::string(PHASE1);

	std::string phase2file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE2FILENAME);
	std::string phase2name = std::string(PHASE2);

	// construct a traveltime
	traveltime::CTTT ttt2;

	// set ranges
	double * weightRange = new double[4];
	weightRange[0] = 0;
	weightRange[1] = 0;
	weightRange[2] = 120;
	weightRange[3] = 180;

	double * assocRange = new double[2];
	assocRange[0] = 10;
	assocRange[1] = 90;

	// add phases
	ttt2.addPhase(phase1name, weightRange, NULL, phase1file);
	ttt2.addPhase(phase2name, NULL, assocRange, phase2file);

	// set origin
	ttt2.setOrigin(LATITUDE, LONGITUDE, DEPTH);

	traveltime::CTTT ttt(ttt2);

	// nTrv
	ASSERT_EQ(2, ttt.nTrv)<< "nTrv Check";

	// phase1 name
	ASSERT_STREQ(ttt.pTrv[ttt.nTrv - 2]->sPhase.c_str(), phase1name.c_str());

	// phase2 name
	ASSERT_STREQ(ttt.pTrv[ttt.nTrv - 1]->sPhase.c_str(), phase2name.c_str());

	// m_dGeocentricLatitude
	ASSERT_NEAR(GEOLATITUDE, ttt.geoOrg.m_dGeocentricLatitude, 0.001)<<
			"m_dGeocentricLatitude Check";

	// m_dGeocentricLongitude
	ASSERT_EQ(LONGITUDE, ttt.geoOrg.m_dGeocentricLongitude)<<
			"m_dGeocentricLongitude Check";

	// m_dGeocentricRadius
	ASSERT_NEAR(GEODEPTH, ttt.geoOrg.m_dGeocentricRadius, 0.01)<<
			"m_dGeocentricRadius Check";

	delete[] (weightRange);
	delete[] (assocRange);
}

// tests to see if various T functions work
TEST(TTTTest, TTests) {
	glass3::util::Logger::disable();

	std::string phase1file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE1FILENAME);
	std::string phase1name = std::string(PHASE1);

	std::string phase2file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE2FILENAME);
	std::string phase2name = std::string(PHASE2);

	// construct a traveltime
	traveltime::CTTT ttt;

	// set ranges
	double * weightRange = new double[4];
	weightRange[0] = 0;
	weightRange[1] = 0;
	weightRange[2] = 120;
	weightRange[3] = 180;

	double * assocRange = new double[2];
	assocRange[0] = 10;
	assocRange[1] = 90;

	// add phases
	ttt.addPhase(phase1name, weightRange, NULL, phase1file);
	ttt.addPhase(phase2name, NULL, assocRange, phase2file);

	// set origin
	ttt.setOrigin(LATITUDE, LONGITUDE, DEPTH);

	glass3::util::Geo testGeo;
	testGeo.setGeographic(LATITUDE, LONGITUDE + DISTANCE, DEPTH);

	// t(geo, phase)
	ASSERT_NEAR(TIME1, ttt.T(&testGeo, phase1name), 0.001)<< "T(geo, phase1) Check";  // NOLINT
	ASSERT_NEAR(TIME2, ttt.T(&testGeo, phase2name), 0.001)<< "T(geo, phase2) Check";  // NOLINT

	// t(delta, phase)
	ASSERT_NEAR(TIME3, ttt.T(DISTANCE, phase1name), 0.001)<< "T(delta, phase1) Check";  // NOLINT
	ASSERT_NEAR(TIME4, ttt.T(DISTANCE, phase2name), 0.001)<< "T(delta, phase2) Check";  // NOLINT

	// t(geo, tobs)
	ASSERT_NEAR(TIME1, ttt.T(&testGeo, TIME1), 0.001)<< "T(geo, tobs) Check";  // NOLINT

	// td(delta, phase, depth)
	ASSERT_NEAR(TIME3, ttt.Td(DISTANCE, phase1name, DEPTH), 0.001)<< "Td(delta, phase1, depth) Check";  // NOLINT
	ASSERT_NEAR(TIME4, ttt.Td(DISTANCE, phase2name, DEPTH), 0.001)<< "Td(delta, phase2, depth) Check";  // NOLINT

	delete[] (weightRange);
	delete[] (assocRange);
}

// tests to see if various T functions work
TEST(TTTTest, TFailTests) {
	glass3::util::Logger::disable();

	std::string phase1file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE1FILENAME);
	std::string phase1name = std::string(PHASE1);

	std::string phase2file = "./" + std::string(TESTPATH) + "/"
			+ std::string(PHASE2FILENAME);
	std::string phase2name = std::string(PHASE2);

	// construct a traveltime
	traveltime::CTTT ttt;

	// set ranges
	double * weightRange = new double[4];
	weightRange[0] = 0;
	weightRange[1] = 0;
	weightRange[2] = 120;
	weightRange[3] = 180;

	double * assocRange = new double[2];
	assocRange[0] = 10;
	assocRange[1] = 90;

	// add phases
	ttt.addPhase(phase1name, weightRange, NULL, phase1file);
	ttt.addPhase(phase2name, NULL, assocRange, phase2file);

	// set origin
	ttt.setOrigin(LATITUDE, LONGITUDE, BADDEPTH);

	glass3::util::Geo testGeo;
	testGeo.setGeographic(LATITUDE, LONGITUDE + BADDISTANCE, DEPTH);

	// t(geo, phase)
	ASSERT_NEAR(BADTIME, ttt.T(&testGeo, phase1name), 0.001)<< "T(geo, phase1) Check";  // NOLINT
	ASSERT_NEAR(BADTIME, ttt.T(&testGeo, phase2name), 0.001)<< "T(geo, phase2) Check";  // NOLINT

	// t(delta, phase)
	ASSERT_NEAR(BADTIME, ttt.T(BADDISTANCE, phase1name), 0.001)<< "T(delta, phase1) Check";  // NOLINT
	ASSERT_NEAR(BADTIME, ttt.T(BADDISTANCE, phase2name), 0.001)<< "T(delta, phase2) Check";  // NOLINT

	// t(geo, tobs)
	ASSERT_NEAR(BADTIME, ttt.T(&testGeo, TIME1), 0.001)<< "T(geo, tobs) Check";  // NOLINT

	// td(delta, phase, depth)
	ASSERT_NEAR(BADTIME, ttt.Td(BADDISTANCE, phase1name, BADDEPTH), 0.001)<< "Td(delta, phase1, depth) Check";  // NOLINT
	ASSERT_NEAR(BADTIME, ttt.Td(BADDISTANCE, phase2name, BADDEPTH), 0.001)<< "Td(delta, phase2, depth) Check";  // NOLINT

	delete[] (weightRange);
	delete[] (assocRange);
}

