#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <logger.h>

#include "Glass.h"

#define TESTPATH "testdata"
#define INITFILENAME "initialize.d"

// default values
#define DEFAULT_MAXNUMPICKS -1
#define DEFAULT_MAXPICKSPERSITE -1
#define DEFAULT_MAXNUMCORR -1
#define DEFAULT_MAXNUMHYPOS -1
#define DEFAULT_GRAPHICSOUT false
#define DEFAULT_GRAPHICSOUTFOLDER "./"
#define DEFAULT_GRAPHICSOUTSTEPSIZE 1.0
#define DEFAULT_GRAPHICSOUTNUMSTEPS 100
#define DEFAULT_TESTTT false
#define DEFAULT_MINMIZELOC false
#define DEFAULT_TESTLOC false
#define DEFAULT_NUCTDATA 7
#define DEFAULT_STAPERNODE 20
#define DEFAULT_NUCTHRESH 2.5
#define DEFAULT_ASSOCSD 3.0
#define DEFAULT_PRUNESD 3.0
#define DEFAULT_AFFEXP 2.5
#define DEFAULT_DISTCUTFACT 4
#define DEFAULT_DISTCUTPERC 0.4
#define DEFAULT_DISTCUTMIN 30
#define DEFAULT_PROCLIMIT 25
#define DEFAULT_PICKDUP 2.5
#define DEFAULT_CORRMATCHTIME 2.5
#define DEFAULT_CORRMATCHDIST 0.5
#define DEFAULT_CORCANCAGE 900
#define DEFAULT_BEAMMATCHAZM 22.5
#define DEFAULT_BEAMMATCHDIST 5
#define DEFAULT_REPORTDATA 0
#define DEFAULT_REPORTTHRESH 2.5

// init values
#define MAXNUMPICKS 10000
#define MAXPICKSPERSITE 30
#define MAXNUMCORR 1000
#define MAXNUMHYPOS 250
#define GRAPHICSOUT false
#define GRAPHICSOUTFOLDER "./"
#define GRAPHICSOUTSTEPSIZE 1.0
#define GRAPHICSOUTNUMSTEPS 100
#define TESTTT false
#define MINMIZELOC false
#define TESTLOC false
#define NUCTDATA 10
#define STAPERNODE 20
#define NUCTHRESH 0.5
#define ASSOCSD 3.0
#define PRUNESD 3.0
#define AFFEXP 2.5
#define DISTCUTFACT 5
#define DISTCUTPERC 0.8
#define DISTCUTMIN 30
#define PROCLIMIT 25
#define PICKDUP 2.5
#define CORRMATCHTIME 2.5
#define CORRMATCHDIST 0.5
#define CORCANCAGE 900
#define BEAMMATCHAZM 22.5
#define BEAMMATCHDIST 5
#define REPORTDATA 5
#define REPORTTHRESH 0.5

// test to see if the hypo can be constructed
TEST(GlassTest, Construction) {
	glass3::util::Logger::disable();

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();

	// assert default values
	ASSERT_EQ(DEFAULT_BEAMMATCHAZM, testGlass->getBeamMatchingAzimuthWindow())<<
	"getBeamMatchingAzimuthWindow";
	ASSERT_EQ(DEFAULT_BEAMMATCHDIST, testGlass->getBeamMatchingDistanceWindow())<<
	"getBeamMatchingDistanceWindow";
	ASSERT_EQ(DEFAULT_CORCANCAGE, testGlass->getCorrelationCancelAge())<<
	"getCorrelationCancelAge";
	ASSERT_EQ(DEFAULT_CORRMATCHTIME, testGlass->getCorrelationMatchingTimeWindow())<<
	"getCorrelationMatchingTimeWindow";
	ASSERT_EQ(DEFAULT_CORRMATCHDIST, testGlass->getCorrelationMatchingDistanceWindow())<<
	"getCorrelationMatchingDistanceWindow";
	ASSERT_EQ(DEFAULT_DISTCUTFACT, testGlass->getDistanceCutoffFactor())<<
	"getDistanceCutoffFactor";
	ASSERT_EQ(DEFAULT_DISTCUTMIN, testGlass->getMinDistanceCutoff())<<
	"getMinDistanceCutoff";
	ASSERT_EQ(DEFAULT_DISTCUTPERC, testGlass->getDistanceCutoffRatio())<<
	"getDistanceCutoffPercentage";
	ASSERT_EQ(DEFAULT_REPORTTHRESH, testGlass->getReportingStackThreshold())<<
	"getReportingStackThreshold";
	ASSERT_EQ(DEFAULT_NUCTHRESH, testGlass->getNucleationStackThreshold())<<
	"getNucleationStackThreshold";
	ASSERT_EQ(DEFAULT_AFFEXP, testGlass->getPickAffinityExpFactor())<<
	"getPickAffinityExpFactor";
	ASSERT_EQ(DEFAULT_GRAPHICSOUT, testGlass->getGraphicsOut())<<
	"getGraphicsOut";
	ASSERT_STREQ(std::string(DEFAULT_GRAPHICSOUTFOLDER).c_str(),
			testGlass->getGraphicsOutFolder().c_str())<<
	"getGraphicsOutFolder";
	ASSERT_EQ(DEFAULT_GRAPHICSOUTSTEPSIZE, testGlass->getGraphicsStepKm())<<
	"getGraphicsStepKm";
	ASSERT_EQ(DEFAULT_GRAPHICSOUTNUMSTEPS, testGlass->getGraphicsSteps())<<
	"getGraphicsSteps";
	ASSERT_EQ(DEFAULT_PROCLIMIT, testGlass->getProcessLimit())<<
	"getProcessLimit";
	ASSERT_EQ(DEFAULT_MINMIZELOC, testGlass->getMinimizeTTLocator())<<
	"getMinimizeTTLocator";
	ASSERT_EQ(DEFAULT_MAXNUMCORR, testGlass->getMaxNumCorrelations())<<
	"getMaxNumCorrelations";
	ASSERT_EQ(DEFAULT_STAPERNODE, testGlass->getNumStationsPerNode())<<
	"getNumStationsPerNode";
	ASSERT_EQ(DEFAULT_MAXNUMHYPOS, testGlass->getMaxNumHypos())<<
	"getMaxNumHypos";
	ASSERT_EQ(DEFAULT_NUCTDATA, testGlass->getNucleationDataThreshold())<<
	"getNucleationDataThreshold";
	ASSERT_EQ(DEFAULT_MAXNUMPICKS, testGlass->getMaxNumPicks())<<
	"getMaxNumPicks";
	ASSERT_EQ(DEFAULT_REPORTDATA, testGlass->getReportingDataThreshold())<<
	"getReportingDataThreshold";
	ASSERT_EQ(DEFAULT_MAXPICKSPERSITE, testGlass->getMaxNumPicksPerSite())<<
	"getMaxNumPicksPerSite";
	ASSERT_EQ(DEFAULT_PICKDUP, testGlass->getPickDuplicateTimeWindow())<<
	"getPickDuplicateTimeWindow";
	ASSERT_EQ(DEFAULT_ASSOCSD, testGlass->getAssociationSDCutoff())<<
	"getAssociationSDCutoff";
	ASSERT_EQ(DEFAULT_PRUNESD, testGlass->getPruningSDCutoff())<<
	"getPruningSDCutoff";
	ASSERT_EQ(DEFAULT_TESTLOC, testGlass->getTestLocator())<<
	"getTestLocator";
	ASSERT_EQ(DEFAULT_TESTTT, testGlass->getTestTravelTimes())<<
	"getTestTravelTimes";

	// pointers
	ASSERT_TRUE(testGlass->getWebList() == NULL)<< "web list null";
	ASSERT_TRUE(testGlass->getSiteList() == NULL)<< "site list null";
	ASSERT_TRUE(testGlass->getHypoList() == NULL)<< "hypo list null";
	ASSERT_TRUE(testGlass->getPickList() == NULL)<< "pick list null";
	ASSERT_TRUE(testGlass->getCorrelationList() == NULL)<<
	"correlation list null";
	ASSERT_TRUE(testGlass->getDetectionProcessor() == NULL)<<
	"detection processor list null";
	ASSERT_TRUE(testGlass->getDefaultNucleationTravelTime() == NULL)<<
	"nucleation travel times null";
	ASSERT_TRUE(testGlass->getAssociationTravelTimes() == NULL)<<
	"association travel times null";
}

// test to see if the glass can be initialized
TEST(GlassTest, Init) {
	glass3::util::Logger::disable();

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));

	testGlass->receiveExternalMessage(initConfig);

	// assert values
	ASSERT_EQ(BEAMMATCHAZM, testGlass->getBeamMatchingAzimuthWindow())<<
	"getBeamMatchingAzimuthWindow";
	ASSERT_EQ(BEAMMATCHDIST, testGlass->getBeamMatchingDistanceWindow())<<
	"getBeamMatchingDistanceWindow";
	ASSERT_EQ(CORCANCAGE, testGlass->getCorrelationCancelAge())<<
	"getCorrelationCancelAge";
	ASSERT_EQ(CORRMATCHTIME, testGlass->getCorrelationMatchingTimeWindow())<<
	"getCorrelationMatchingTimeWindow";
	ASSERT_EQ(CORRMATCHDIST, testGlass->getCorrelationMatchingDistanceWindow())<<
	"getCorrelationMatchingDistanceWindow";
	ASSERT_EQ(DISTCUTFACT, testGlass->getDistanceCutoffFactor())<<
	"getDistanceCutoffFactor";
	ASSERT_EQ(DISTCUTMIN, testGlass->getMinDistanceCutoff())<<
	"getMinDistanceCutoff";
	ASSERT_EQ(DISTCUTPERC, testGlass->getDistanceCutoffRatio())<<
	"getDistanceCutoffPercentage";
	ASSERT_EQ(REPORTTHRESH, testGlass->getReportingStackThreshold())<<
	"getReportingStackThreshold";
	ASSERT_EQ(NUCTHRESH, testGlass->getNucleationStackThreshold())<<
	"getNucleationStackThreshold";
	ASSERT_EQ(AFFEXP, testGlass->getPickAffinityExpFactor())<<
	"getPickAffinityExpFactor";
	ASSERT_EQ(GRAPHICSOUT, testGlass->getGraphicsOut())<<
	"getGraphicsOut";
	ASSERT_STREQ(std::string(GRAPHICSOUTFOLDER).c_str(),
			testGlass->getGraphicsOutFolder().c_str())<<
	"getGraphicsOutFolder";
	ASSERT_EQ(GRAPHICSOUTSTEPSIZE, testGlass->getGraphicsStepKm())<<
	"getGraphicsStepKm";
	ASSERT_EQ(GRAPHICSOUTNUMSTEPS, testGlass->getGraphicsSteps())<<
	"getGraphicsSteps";
	ASSERT_EQ(PROCLIMIT, testGlass->getProcessLimit())<<
	"getProcessLimit";
	ASSERT_EQ(MINMIZELOC, testGlass->getMinimizeTTLocator())<<
	"getMinimizeTTLocator";
	ASSERT_EQ(MAXNUMCORR, testGlass->getMaxNumCorrelations())<<
	"getMaxNumCorrelations";
	ASSERT_EQ(STAPERNODE, testGlass->getNumStationsPerNode())<<
	"getNumStationsPerNode";
	ASSERT_EQ(MAXNUMHYPOS, testGlass->getMaxNumHypos())<<
	"getMaxNumHypos";
	ASSERT_EQ(NUCTDATA, testGlass->getNucleationDataThreshold())<<
	"getNucleationDataThreshold";
	ASSERT_EQ(MAXNUMPICKS, testGlass->getMaxNumPicks())<<
	"getMaxNumPicks";
	ASSERT_EQ(REPORTDATA, testGlass->getReportingDataThreshold())<<
	"getReportingDataThreshold";
	ASSERT_EQ(MAXPICKSPERSITE, testGlass->getMaxNumPicksPerSite())<<
	"getMaxNumPicksPerSite";
	ASSERT_EQ(PICKDUP, testGlass->getPickDuplicateTimeWindow())<<
	"getPickDuplicateTimeWindow";
	ASSERT_EQ(ASSOCSD, testGlass->getAssociationSDCutoff())<<
	"getAssociationSDCutoff";
	ASSERT_EQ(PRUNESD, testGlass->getPruningSDCutoff())<<
	"getPruningSDCutoff";
	ASSERT_EQ(TESTLOC, testGlass->getTestLocator())<<
	"getTestLocator";
	ASSERT_EQ(TESTTT, testGlass->getTestTravelTimes())<<
	"getTestTravelTimes";

	// pointers
	ASSERT_TRUE(testGlass->getWebList() != NULL)<< "web list";
	ASSERT_TRUE(testGlass->getSiteList() != NULL)<< "site list";
	ASSERT_TRUE(testGlass->getHypoList() != NULL)<< "hypo list";
	ASSERT_TRUE(testGlass->getPickList() != NULL)<< "pick list";
	ASSERT_TRUE(testGlass->getCorrelationList() != NULL)<<
	"correlation list";
	ASSERT_TRUE(testGlass->getDetectionProcessor() != NULL)<<
	"detection processor list";
	ASSERT_TRUE(testGlass->getDefaultNucleationTravelTime() != NULL)<<
	"nucleation travel times";
	ASSERT_TRUE(testGlass->getAssociationTravelTimes() != NULL)<<
	"association travel times";
}

