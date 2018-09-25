#include <gtest/gtest.h>
#include <string>

#include <logger.h>

#include "Terra.h"


#define TESTPATH "testdata"
#define MODELFILE "ak135_mod.d"

#define NLAYER 138
#define EARTHRADIUS 6371.0
#define OUTERDISCONTINUITY 68
#define INNERDISCONTINUITY 23
#define NDISCONTINUITIES 8

#define PARSESTRING "   510.000      9.6960      5.2920      3.9248      3.8793"
#define PARSESIZE 5;
#define PARSEVALUE1 "510.000"
#define PARSEVALUE2 "9.6960"
#define PARSEVALUE3 "5.2920"
#define PARSEVALUE4 "3.9248"
#define PARSEVALUE5 "3.8793"

#define TESTRADIUS 5523.0
#define PINTERPOLATION 11.2028
#define SINTERPOLATION 6.2715

#define TURNRADIUS 3479.5

#define EVALUATETEST 0.01345
#define EVALUATETIMEP 0.10425
#define EVALUATEDISTANCEP 0.0001
#define EVALUATETAUP 0.076425
#define EVALUATETIMES 0.18983
#define EVALUATEDISTANCES 0.00010
#define EVALUATETAUS 0.13393

#define INTEGRATETEST 13.46972
#define INTEGRATETIMEP 107.62532
#define INTEGRATEDISTANCEP 0.10845
#define INTEGRATETAUP 79.99607
#define INTEGRATETIMES 197.63114
#define INTEGRATEDISTANCES 0.11443
#define INTEGRATETAUS 142.94343

#define ROMBERGTEST 13.46972
#define ROMBERGTIMEP 107.61990
#define ROMBERGDISTANCEP 0.10845
#define ROMBERGTAUP 79.99345
#define ROMBERGTIMES 197.62740
#define ROMBERGDISTANCES 0.11443
#define ROMBERGTAUS 142.93856

// test to see if the hypo can be constructed
TEST(TerraTest, Construction) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// assert default values
	ASSERT_EQ(0, testTerra->nLayer)<< "nLayer is zero";
	ASSERT_EQ(0, testTerra->dEarthRadius)<< "dEarthRadius is zero";
	ASSERT_EQ(-1, testTerra->iOuterDiscontinuity)<< "iOuterDiscontinuity is -1";
	ASSERT_EQ(-1, testTerra->iInnerDiscontinuity)<< "iInnerDiscontinuity is -1";
	ASSERT_EQ(0, testTerra->nDiscontinuities)<< "nDiscontinuities is zero";
}

// test to see if the hypo can be constructed
TEST(TerraTest, Load) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// assert loaded values
	// check nLayer
	int nLayer = testTerra->nLayer;
	int expectedNLayer = NLAYER;
	ASSERT_EQ(nLayer, expectedNLayer);

	// check earthRadius
	double earthRadius = testTerra->dEarthRadius;
	double expectedEarthRadius = EARTHRADIUS;
	ASSERT_NEAR(earthRadius, expectedEarthRadius, 0.0001);

	// check outerDiscontinuity
	int outerDiscontinuity = testTerra->iOuterDiscontinuity;
	int expectedOuterDiscontinuity = OUTERDISCONTINUITY;
	ASSERT_EQ(outerDiscontinuity, expectedOuterDiscontinuity);

	// check innerDiscontinuity
	int innerDiscontinuity = testTerra->iInnerDiscontinuity;
	int expectedInnerDiscontinuity = INNERDISCONTINUITY;
	ASSERT_EQ(innerDiscontinuity, expectedInnerDiscontinuity);

	// check nDiscontinuities
	int nDiscontinuities = testTerra->nDiscontinuities;
	int expectedNDiscontinuities = NDISCONTINUITIES;
	ASSERT_EQ(nDiscontinuities, expectedNDiscontinuities);

	// now clear
	testTerra->clear();

	// assert default values
	ASSERT_EQ(0, testTerra->nLayer)<< "nLayer is zero";
	ASSERT_EQ(0, testTerra->dEarthRadius)<< "dEarthRadius is zero";
	ASSERT_EQ(-1, testTerra->iOuterDiscontinuity)<< "iOuterDiscontinuity is -1";
	ASSERT_EQ(-1, testTerra->iInnerDiscontinuity)<< "iInnerDiscontinuity is -1";
	ASSERT_EQ(0, testTerra->nDiscontinuities)<< "nDiscontinuities is zero";
}

TEST(TerraTest, Parse) {
	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	std::string parseLine = std::string(PARSESTRING);

	json::Array results = testTerra->parse(parseLine.c_str());

	// check data
	// size
	int size = results.size();
	int expectedSize = PARSESIZE;
	ASSERT_EQ(size, expectedSize);

	// check value 1
	std::string value1 = results[0];
	std::string expectedValue1 = std::string(PARSEVALUE1);
	ASSERT_STREQ(value1.c_str(), expectedValue1.c_str());

	// check value 2
	std::string value2 = results[1];
	std::string expectedValue2 = std::string(PARSEVALUE2);
	ASSERT_STREQ(value2.c_str(), expectedValue2.c_str());

	// check value 3
	std::string value3 = results[2];
	std::string expectedValue3 = std::string(PARSEVALUE3);
	ASSERT_STREQ(value3.c_str(), expectedValue3.c_str());

	// check value 4
	std::string value4 = results[3];
	std::string expectedValue4 = std::string(PARSEVALUE4);
	ASSERT_STREQ(value4.c_str(), expectedValue4.c_str());

	// check value 5
	std::string value5 = results[4];
	std::string expectedValue5 = std::string(PARSEVALUE5);
	ASSERT_STREQ(value5.c_str(), expectedValue5.c_str());
}

// test Interpolate velocity for P
TEST(TerraTest, PTest) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// check p interpolation
	double pInterpolation = testTerra->P(TESTRADIUS);
	double expectedPInterpolation = PINTERPOLATION;
	ASSERT_NEAR(pInterpolation, expectedPInterpolation, 0.0001);
}

// test Interpolate velocity for S
TEST(TerraTest, STest) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// check p interpolation
	double sInterpolation = testTerra->S(TESTRADIUS);
	double expectedSInterpolation = SINTERPOLATION;
	ASSERT_NEAR(sInterpolation, expectedSInterpolation, 0.0001);
}

// Note that  testTerra->P and testTerra->S just call interpolateVelocity
// so why test it.

// test calcuating turning radius
TEST(TerraTest, TurnRadius) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// compute test values (for P)
	int lowerIndex = testTerra->iOuterDiscontinuity + 1;
	int upperIndex = testTerra->nLayer - 1;
	int layerIndex = testTerra->iOuterDiscontinuity + 1;
	double rayParam = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerPVel[layerIndex];

	// Check P turning radius
	double turnRadius = testTerra->calculateTurnRadius(lowerIndex, upperIndex,
														testTerra->dLayerPVel,
														rayParam);
	double expectedTurnRadius = TURNRADIUS;
	ASSERT_NEAR(turnRadius, expectedTurnRadius, 0.0001);
}

// test calcuating evaluate function
TEST(TerraTest, EvaluateFunction) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// compute ray parameters
	int layerIndex = testTerra->iOuterDiscontinuity + 1;
	double rayParamP = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerPVel[layerIndex];
	double rayParamS = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerSVel[layerIndex];

	double earthRadius = TESTRADIUS;

	// Check evaluateFunction FUN_TEST
	double test = testTerra->evaluateFunction(FUN_TEST, earthRadius, rayParamP);
	double expectedTest = EVALUATETEST;
	ASSERT_NEAR(test, expectedTest, 0.0001);

	// Check evaluateFunction FUN_P_TIME
	double timeP = testTerra->evaluateFunction(FUN_P_TIME, earthRadius,
												rayParamP);
	double expectedTimeP = EVALUATETIMEP;
	ASSERT_NEAR(timeP, expectedTimeP, 0.0001);

	// Check evaluateFunction FUN_P_DELTA
	double distanceP = testTerra->evaluateFunction(FUN_P_DELTA, earthRadius,
													rayParamP);
	double expectedDistanceP = EVALUATEDISTANCEP;
	ASSERT_NEAR(distanceP, expectedDistanceP, 0.0001);

	// Check evaluateFunction FUN_P_TAU
	double tauP = testTerra->evaluateFunction(FUN_P_TAU, earthRadius,
												rayParamP);
	double expectedTauP = EVALUATETAUP;
	ASSERT_NEAR(tauP, expectedTauP, 0.0001);

	// Check evaluateFunction FUN_S_TIME
	double timeS = testTerra->evaluateFunction(FUN_S_TIME, earthRadius,
												rayParamS);
	double expectedTimeS = EVALUATETIMES;
	ASSERT_NEAR(timeS, expectedTimeS, 0.0001);

	// Check evaluateFunction FUN_S_DELTA
	double distanceS = testTerra->evaluateFunction(FUN_S_DELTA, earthRadius,
													rayParamS);
	double expectedDistanceS = EVALUATEDISTANCES;
	ASSERT_NEAR(distanceS, expectedDistanceS, 0.0001);

	// Check evaluateFunction FUN_S_TAU
	double tauS = testTerra->evaluateFunction(FUN_S_TAU, earthRadius,
												rayParamS);
	double expectedTauS = EVALUATETAUS;
	ASSERT_NEAR(tauS, expectedTauS, 0.0001);
}

// test integrating ray segment
TEST(TerraTest, IntegrateRaySegment) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// compute ray parameters
	int layerIndex = testTerra->iOuterDiscontinuity + 1;
	double rayParamP = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerPVel[layerIndex];
	double rayParamS = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerSVel[layerIndex];

	// set up radius
	double startRadius = TESTRADIUS - 500;
	double endRadius = TESTRADIUS + 500;

	// Check integrateRaySegment FUN_TEST
	double test = testTerra->integrateRaySegment(FUN_TEST, startRadius,
													endRadius, rayParamP);
	double expectedTest = INTEGRATETEST;
	ASSERT_NEAR(test, expectedTest, 0.0001);

	// Check integrateRaySegment FUN_P_TIME
	double timeP = testTerra->integrateRaySegment(FUN_P_TIME, startRadius,
													endRadius, rayParamP);
	double expectedTimeP = INTEGRATETIMEP;
	ASSERT_NEAR(timeP, expectedTimeP, 0.0001);

	// Check integrateRaySegment FUN_P_DELTA
	double distanceP = testTerra->integrateRaySegment(FUN_P_DELTA, startRadius,
														endRadius, rayParamP);
	double expectedDistanceP = INTEGRATEDISTANCEP;
	ASSERT_NEAR(distanceP, expectedDistanceP, 0.0001);

	// Check integrateRaySegment FUN_P_TAU
	double tauP = testTerra->integrateRaySegment(FUN_P_TAU, startRadius,
													endRadius, rayParamP);
	double expectedTauP = INTEGRATETAUP;
	ASSERT_NEAR(tauP, expectedTauP, 0.0001);

	// Check integrateRaySegment FUN_S_TIME
	double timeS = testTerra->integrateRaySegment(FUN_S_TIME, startRadius,
													endRadius, rayParamS);
	double expectedTimeS = INTEGRATETIMES;
	ASSERT_NEAR(timeS, expectedTimeS, 0.0001);

	// Check integrateRaySegment FUN_S_DELTA
	double distanceS = testTerra->integrateRaySegment(FUN_S_DELTA, startRadius,
														endRadius, rayParamS);
	double expectedDistanceS = INTEGRATEDISTANCES;
	ASSERT_NEAR(distanceS, expectedDistanceS, 0.0001);

	// Check integrateRaySegment FUN_S_TAU
	double tauS = testTerra->integrateRaySegment(FUN_S_TAU, startRadius,
													endRadius, rayParamS);
	double expectedTauS = INTEGRATETAUS;
	ASSERT_NEAR(tauS, expectedTauS, 0.0001);
}

// test integrating ray segment
TEST(TerraTest, RombergIntegration) {
	glass3::util::Logger::disable();

	// construct a Terra object
	traveltime::CTerra * testTerra = new traveltime::CTerra();

	// build model path
	std::string testModelPath = "./" + std::string(TESTPATH) + "/"
			+ std::string(MODELFILE);

	// load model
	bool result = testTerra->load(testModelPath.c_str());

	// check success
	ASSERT_TRUE(result)<< "load successful";

	// compute ray parameters
	int layerIndex = testTerra->iOuterDiscontinuity + 1;
	double rayParamP = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerPVel[layerIndex];
	double rayParamS = testTerra->dLayerRadii[layerIndex]
			/ testTerra->dLayerSVel[layerIndex];

	// set up radius
	double startRadius = TESTRADIUS - 500;
	double endRadius = TESTRADIUS + 500;

	// Check integrateRaySegment FUN_TEST
	double test = testTerra->rombergIntegration(FUN_TEST, startRadius,
												endRadius, rayParamP);
	double expectedTest = ROMBERGTEST;
	ASSERT_NEAR(test, expectedTest, 0.0001);

	// Check integrateRaySegment FUN_P_TIME
	double timeP = testTerra->rombergIntegration(FUN_P_TIME, startRadius,
													endRadius, rayParamP);
	double expectedTimeP = ROMBERGTIMEP;
	ASSERT_NEAR(timeP, expectedTimeP, 0.0001);

	// Check integrateRaySegment FUN_P_DELTA
	double distanceP = testTerra->rombergIntegration(FUN_P_DELTA, startRadius,
														endRadius, rayParamP);
	double expectedDistanceP = ROMBERGDISTANCEP;
	ASSERT_NEAR(distanceP, expectedDistanceP, 0.0001);

	// Check integrateRaySegment FUN_P_TAU
	double tauP = testTerra->rombergIntegration(FUN_P_TAU, startRadius,
												endRadius, rayParamP);
	double expectedTauP = ROMBERGTAUP;
	ASSERT_NEAR(tauP, expectedTauP, 0.0001);

	// Check integrateRaySegment FUN_S_TIME
	double timeS = testTerra->rombergIntegration(FUN_S_TIME, startRadius,
													endRadius, rayParamS);
	double expectedTimeS = ROMBERGTIMES;
	ASSERT_NEAR(timeS, expectedTimeS, 0.0001);

	// Check integrateRaySegment FUN_S_DELTA
	double distanceS = testTerra->rombergIntegration(FUN_S_DELTA, startRadius,
														endRadius, rayParamS);
	double expectedDistanceS = ROMBERGDISTANCES;
	ASSERT_NEAR(distanceS, expectedDistanceS, 0.0001);

	// Check integrateRaySegment FUN_S_TAU
	double tauS = testTerra->rombergIntegration(FUN_S_TAU, startRadius,
												endRadius, rayParamS);
	double expectedTauS = ROMBERGTAUS;
	ASSERT_NEAR(tauS, expectedTauS, 0.0001);
}
