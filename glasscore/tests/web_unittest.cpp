#include <gtest/gtest.h>

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <fstream>

#include <logger.h>

#include "Node.h"
#include "Web.h"
#include "Site.h"
#include "SiteList.h"

#define TESTPATH "testdata"

#define STATIONFILENAME "teststationlist.json"

#define GLOBALFILENAME "testglobal.d"
#define BADGLOBALFILENAME1 "badglobal1.d"
#define BADGLOBALFILENAME2 "badglobal2.d"
#define BADGLOBALFILENAME3 "badglobal3.d"

#define GRIDFILENAME "testgrid.d"
#define BADGRIDFILENAME1 "badgrid1.d"
#define BADGRIDFILENAME2 "badgrid2.d"
#define BADGRIDFILENAME3 "badgrid3.d"

#define GRIDEXPLICITFILENAME "testexplicitgrid.d"
#define BADEXPLICITGRIDFILENAME1 "badexplicitgrid1.d"
#define BADEXPLICITGRIDFILENAME2 "badexplicitgrid2.d"

#define NAME "TestWeb"
#define THRESH 1.4
#define NUMDETECT 5
#define NUMNUCLEATE 4
#define RESOLUTION 100.0
#define AZIGAP 360.0
#define UPDATE true
#define NOUPDATE false
#define SAVE true
#define NOSAVE false
#define NUMTHREADS 1
#define NOTHREADS 0

#define GLOBALNAME "TestGlobal"
#define GLOBALTHRESH 2.5
#define GLOBALNUMDETECT 10
#define GLOBALNUMNUCLEATE 5
#define GLOBALRESOLUTION 250.0
#define GLOBALNUMZ 2
#define GLOBALNUMNODES 19410
#define GLOBALNUMNETEXLUDE 13

#define GRIDNAME "TestGrid"
#define GRIDTHRESH 0.5
#define GRIDNUMDETECT 10
#define GRIDNUMNUCLEATE 6
#define GRIDRESOLUTION 25.0
#define GRIDNUMNODES 2601

#define GRIDEXPLICITNAME "TestExplicitGrid"
#define GRIDEXPLICITTHRESH 0.5
#define GRIDEXPLICITNUMDETECT 14
#define GRIDEXPLICITNUMNUCLEATE 5
#define GRIDEXPLICITRESOLUTION 100.0
#define GRIDEXPLICITNUMNODES 12

#define PHASE1 "P"
#define PHASE2 "S"

#define ADDSITE "{\"Elevation\":302.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.656729,\"Longitude\":-97.609276,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"BCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT
#define REMOVESITE "{\"Elevation\":378.000000,\"Enable\":false,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.356842,\"Longitude\":-97.656074,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"CCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT

// tests to see if the web can be constructed
TEST(WebTest, Construction) {
	glass3::util::Logger::disable();

	// default constructor
	glasscore::CWeb aWeb(NOTHREADS, 10, 10);

	// construct a web
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	glasscore::CWeb * testWeb = new glasscore::CWeb(std::string(NAME),
	THRESH,
													NUMDETECT,
													NUMNUCLEATE,
													RESOLUTION,
													UPDATE,
													NOSAVE, nullTrav, nullTrav);

	// name
	ASSERT_STREQ(std::string(NAME).c_str(), testWeb->getName().c_str())<<
	"Web getName() Matches";

	// threshold
	ASSERT_EQ(THRESH, testWeb->getNucleationStackThreshold())<< "Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(NUMDETECT, testWeb->getNumStationsPerNode())<< "Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(NUMNUCLEATE, testWeb->getNucleationDataThreshold())<< "Web getNucleate() Check";

	// resolution
	ASSERT_EQ(RESOLUTION, testWeb->getNodeResolution())<< "Web resolution Check";

	// getUpdate()
	ASSERT_EQ(UPDATE, testWeb->getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(NOSAVE, testWeb->getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, (int)testWeb->size())<< "node list empty";
	ASSERT_EQ(false, testWeb->getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(expectedSize, (int)testWeb->getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(expectedSize, (int)testWeb->getSitesFilterSize())<<
	"site filter list empty";

	// construct a web
	glasscore::CWeb * testWeb2 = new glasscore::CWeb(std::string(NAME),
	THRESH,
														NUMDETECT,
														NUMNUCLEATE,
														RESOLUTION,
														NOUPDATE,
														SAVE, nullTrav,
														nullTrav);

	// name
	ASSERT_STREQ(std::string(NAME).c_str(), testWeb2->getName().c_str())<<
	"Web getName() Matches";

	// threshold
	ASSERT_EQ(THRESH, testWeb2->getNucleationStackThreshold())<< "Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(NUMDETECT, testWeb2->getNumStationsPerNode())<< "Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(NUMNUCLEATE, testWeb2->getNucleationDataThreshold())<< "Web getNucleate() Check";

	// resolution
	ASSERT_EQ(RESOLUTION, testWeb2->getNodeResolution())<< "Web resolution Check";

	// getUpdate()
	ASSERT_EQ(NOUPDATE, testWeb2->getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(SAVE, testWeb2->getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	ASSERT_EQ(expectedSize, (int)testWeb2->size())<< "node list empty";
	ASSERT_EQ(false, testWeb->getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(expectedSize, (int)testWeb2->getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(expectedSize, (int)testWeb->getSitesFilterSize())<<
	"site filter list empty";

	delete (testWeb);
	delete (testWeb2);
}

// tests to see if the web can be initialized
TEST(WebTest, Initialize) {
	glass3::util::Logger::disable();

	printf("[ startup  ]\n");

	// default constructor
	glasscore::CWeb * testWeb = new glasscore::CWeb(NUMTHREADS, 10, 10);
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	printf("[ construct]\n");

	testWeb->initialize(std::string(NAME),
	THRESH,
						NUMDETECT,
						NUMNUCLEATE,
						RESOLUTION,
						UPDATE,
						SAVE, nullTrav, nullTrav, AZIGAP);

	printf("[ init     ]\n");

	// name
	ASSERT_STREQ(std::string(NAME).c_str(), testWeb->getName().c_str())<<
	"Web getName() Matches";

	// threshold
	ASSERT_EQ(THRESH, testWeb->getNucleationStackThreshold())<< "Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(NUMDETECT, testWeb->getNumStationsPerNode())<< "Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(NUMNUCLEATE, testWeb->getNucleationDataThreshold())<< "Web getNucleate() Check";

	// resolution
	ASSERT_EQ(RESOLUTION, testWeb->getNodeResolution())<< "Web resolution Check";

	// getUpdate()
	ASSERT_EQ(UPDATE, testWeb->getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(SAVE, testWeb->getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, (int)testWeb->size())<< "node list empty";
	ASSERT_EQ(false, testWeb->getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(expectedSize, (int)testWeb->getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(expectedSize, (int)testWeb->getSitesFilterSize())<<
	"site filter list empty";

	printf("[ shutdown ]\n");
}

// test constructing a global grid
TEST(WebTest, GlobalTest) {
	glass3::util::Logger::disable();

	std::string phasename1 = std::string(PHASE1);
	std::string phasename2 = std::string(PHASE2);

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// global config
	std::ifstream globalFile;
	globalFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(GLOBALFILENAME),
			std::ios::in);
	std::string globalLine = "";
	std::getline(globalFile, globalLine);
	globalFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> globalConfig = std::make_shared<json::Object>(
			json::Deserialize(globalLine));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a web
	glasscore::CWeb testGlobalWeb(NUMTHREADS);
	testGlobalWeb.setSiteList(testSiteList);
	testGlobalWeb.receiveExternalMessage(globalConfig);

	// name
	ASSERT_STREQ(std::string(GLOBALNAME).c_str(),
			testGlobalWeb.getName().c_str())<<
	"Web getName() Matches";

	// threshold
	ASSERT_EQ(GLOBALTHRESH, testGlobalWeb.getNucleationStackThreshold())<<
	"Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(GLOBALNUMDETECT, testGlobalWeb.getNumStationsPerNode())<<
	"Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(GLOBALNUMNUCLEATE, testGlobalWeb.getNucleationDataThreshold())<<
	"Web getNucleate() Check";

	// getResolution()
	ASSERT_EQ(GLOBALRESOLUTION, testGlobalWeb.getNodeResolution())<<
	"Web getResolution() Check";

	// getUpdate()
	ASSERT_EQ(UPDATE, testGlobalWeb.getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(SAVE, testGlobalWeb.getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	ASSERT_EQ(GLOBALNUMNODES, (int)testGlobalWeb.size())<< "node list";
	ASSERT_EQ(false, testGlobalWeb.getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(GLOBALNUMNETEXLUDE, (int)testGlobalWeb.getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(0, (int)testGlobalWeb.getSitesFilterSize())<<
	"site filter list empty";

	// pointers
	ASSERT_TRUE(NULL != testGlobalWeb.getNucleationTravelTime1())<< "getTrv1() not null";
	ASSERT_TRUE(NULL != testGlobalWeb.getNucleationTravelTime2())<< "getTrv2() not null";

	// phase name
	ASSERT_STREQ(testGlobalWeb.getNucleationTravelTime1()->sPhase.c_str(),
					phasename1.c_str());

	// phase name
	ASSERT_STREQ(testGlobalWeb.getNucleationTravelTime2()->sPhase.c_str(),
					phasename2.c_str());

	// cleanup
	delete (testSiteList);
}

// test creating a regional/local grid
// NOTE: Need to check that grid boundries are as expected.
TEST(WebTest, GridTest) {
	glass3::util::Logger::disable();

	std::string phasename1 = std::string(PHASE1);
	std::string phasename2 = std::string(PHASE2);

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// grid config
	std::ifstream gridFile;
	gridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(GRIDFILENAME),
			std::ios::in);
	std::string gridLine = "";
	std::getline(gridFile, gridLine);
	gridFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> gridConfig = std::make_shared<json::Object>(
			json::Deserialize(gridLine));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a web
	glasscore::CWeb testGridWeb(NUMTHREADS);
	testGridWeb.setSiteList(testSiteList);
	testGridWeb.receiveExternalMessage(gridConfig);

	// name
	ASSERT_STREQ(std::string(GRIDNAME).c_str(), testGridWeb.getName().c_str())<<
	"Web getName() Matches";

	// threshold
	ASSERT_EQ(GRIDTHRESH, testGridWeb.getNucleationStackThreshold())<< "Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(GRIDNUMDETECT, testGridWeb.getNumStationsPerNode())<< "Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(GRIDNUMNUCLEATE, testGridWeb.getNucleationDataThreshold())<<
	"Web getNucleate() Check";

	// getResolution()
	ASSERT_EQ(GRIDRESOLUTION, testGridWeb.getNodeResolution())<<
	"Web getResolution() Check";

	// getUpdate()
	ASSERT_EQ(UPDATE, testGridWeb.getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(SAVE, testGridWeb.getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	ASSERT_EQ(GRIDNUMNODES, (int)testGridWeb.size())<< "node list";
	ASSERT_EQ(false, testGridWeb.getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(0, (int)testGridWeb.getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(0, (int)testGridWeb.getSitesFilterSize())<<
	"site filter list empty";

	// pointers
	ASSERT_TRUE(NULL != testGridWeb.getNucleationTravelTime1())<< "getTrv1() not null";
	ASSERT_TRUE(NULL != testGridWeb.getNucleationTravelTime2())<< "getTrv2() not null";

	// phase name
	ASSERT_STREQ(testGridWeb.getNucleationTravelTime1()->sPhase.c_str(),
					phasename1.c_str());

	// phase name
	ASSERT_STREQ(testGridWeb.getNucleationTravelTime2()->sPhase.c_str(),
					phasename2.c_str());

	// cleanup
	delete (testSiteList);
}

// test creating an explcit grid
TEST(WebTest, GridExplicitTest) {
	glass3::util::Logger::disable();

	std::string phasename1 = std::string(PHASE1);

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// grid config
	std::ifstream gridFile;
	gridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(GRIDEXPLICITFILENAME),
			std::ios::in);
	std::string gridLine = "";
	std::getline(gridFile, gridLine);
	gridFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> gridConfig = std::make_shared<json::Object>(
			json::Deserialize(gridLine));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a web
	glasscore::CWeb testGridWeb(NUMTHREADS);
	testGridWeb.setSiteList(testSiteList);
	testGridWeb.receiveExternalMessage(gridConfig);

	// name
	ASSERT_STREQ(std::string(GRIDEXPLICITNAME).c_str(),
			testGridWeb.getName().c_str())<< "Web getName() Matches";

	// threshold
	ASSERT_EQ(GRIDEXPLICITTHRESH,
			testGridWeb.getNucleationStackThreshold())<< "Web getThresh() Check";

	// getDetect()
	ASSERT_EQ(GRIDEXPLICITNUMDETECT,
			testGridWeb.getNumStationsPerNode())<< "Web getDetect() Check";

	// getNucleate()
	ASSERT_EQ(GRIDEXPLICITNUMNUCLEATE,
			testGridWeb.getNucleationDataThreshold())<< "Web getNucleate() Check";

	// getResolution()
	ASSERT_EQ(GRIDEXPLICITRESOLUTION,
			testGridWeb.getNodeResolution())<< "Web getResolution() Check";

	// getUpdate()
	ASSERT_EQ(UPDATE, testGridWeb.getUpdate())<< "Web getUpdate() Check";

	// getSaveGrid()
	ASSERT_EQ(SAVE, testGridWeb.getSaveGrid())<< "Web getSaveGrid() Check";

	// lists
	ASSERT_EQ(GRIDEXPLICITNUMNODES, (int)testGridWeb.size())<< "node list";
	ASSERT_EQ(false, testGridWeb.getUseOnlyTeleseismicStations())<<
	"bUseOnlyTeleseismicStations false";
	ASSERT_EQ(0, (int)testGridWeb.getNetworksFilterSize())<<
	"net filter list empty";
	ASSERT_EQ(0, (int)testGridWeb.getSitesFilterSize())<<
	"site filter list empty";

	// pointers
	ASSERT_TRUE(NULL != testGridWeb.getNucleationTravelTime1())<< "getTrv1() not null";
	ASSERT_TRUE(NULL == testGridWeb.getNucleationTravelTime2())<< "getTrv2() null";

	// phase name
	ASSERT_STREQ(testGridWeb.getNucleationTravelTime1()->sPhase.c_str(),
					phasename1.c_str());

	// cleanup
	delete (testSiteList);
}

// test adding a station to a grid
TEST(WebTest, AddTest) {
	glass3::util::Logger::disable();

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// grid config
	std::ifstream gridFile;
	gridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(GRIDFILENAME),
			std::ios::in);
	std::string gridLine = "";
	std::getline(gridFile, gridLine);
	gridFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> gridConfig = std::make_shared<json::Object>(
			json::Deserialize(gridLine));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a web
	glasscore::CWeb testGridWeb(NUMTHREADS);
	testGridWeb.setSiteList(testSiteList);
	testGridWeb.receiveExternalMessage(gridConfig);

	// create site to add
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(ADDSITE))));
	glasscore::CSite * addSite = new glasscore::CSite(siteJSON);
	std::shared_ptr<glasscore::CSite> sharedAddSite(addSite);

	// add to site list
	testSiteList->addSite(sharedAddSite);

	// check to see if this site is in grid
	ASSERT_FALSE(testGridWeb.hasSite(sharedAddSite))<< "site not in grid";

	// add to grid
	testGridWeb.addSite(sharedAddSite);

	// check to see if this site is in grid
	ASSERT_TRUE(testGridWeb.hasSite(sharedAddSite))<< "site added";

	// cleanup
	delete (testSiteList);
}

// test removing a station from a grid
TEST(WebTest, RemoveTest) {
	glass3::util::Logger::disable();

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// grid config
	std::ifstream gridFile;
	gridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(GRIDFILENAME),
			std::ios::in);
	std::string gridLine = "";
	std::getline(gridFile, gridLine);
	gridFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> gridConfig = std::make_shared<json::Object>(
			json::Deserialize(gridLine));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a web
	glasscore::CWeb testGridWeb(NUMTHREADS);
	testGridWeb.setSiteList(testSiteList);
	testGridWeb.receiveExternalMessage(gridConfig);

	// create site to remove
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(REMOVESITE))));
	glasscore::CSite * removeSite = new glasscore::CSite(siteJSON);
	std::shared_ptr<glasscore::CSite> sharedRemoveSite(removeSite);

	// update in site list
	testSiteList->addSite(sharedRemoveSite);

	// check to see if this site is in grid
	ASSERT_TRUE(testGridWeb.hasSite(sharedRemoveSite))<< "site in grid";

	// remove from grid
	testGridWeb.removeSite(sharedRemoveSite);

	// check to see if this site is in grid
	ASSERT_FALSE(testGridWeb.hasSite(sharedRemoveSite))<< "site removed";

	// cleanup
	delete (testSiteList);
}

// test various failure cases for web
TEST(WebTest, FailTests) {
	glass3::util::Logger::disable();

	printf("[ startup  ]\n");

	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	glasscore::CWeb aWeb(std::string(NAME),
	THRESH,
							NUMDETECT,
							NUMNUCLEATE,
							RESOLUTION,
							NOUPDATE,
							SAVE, nullTrav, nullTrav, NOTHREADS, 10, 10);

	printf("[ construct]\n");

	// Nulls
	ASSERT_FALSE(aWeb.receiveExternalMessage(NULL))<< "Null dispatch false";
	ASSERT_FALSE(aWeb.generateGlobalGrid(NULL))<< "Null global false";
	ASSERT_FALSE(aWeb.generateLocalGrid(NULL))<< "Null grid false";
	ASSERT_FALSE(aWeb.generateExplicitGrid(NULL))<< "Null grid_explicit false";

	printf("[ nulls    ]\n");

	// grid fails
	std::ifstream badGridFile;
	std::string badGridLine = "";

	// global
	// bad tt
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(BADGLOBALFILENAME1),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig))<< "bad global1 false";

	// no resolution
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(BADGLOBALFILENAME2),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig2 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig2))<< "bad global2 false";

	// no depths
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(BADGLOBALFILENAME3),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig3 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig3))<< "bad global4 false";

	printf("[ global   ]\n");

	// grid
	// bad tt
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(BADGRIDFILENAME1),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig4 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig4))<< "bad grid1 false";

	// no resolution
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(BADGRIDFILENAME2),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig5 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig5))<< "bad grid2 false";

	// no depths
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(BADGRIDFILENAME3),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig6 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig6))<< "bad grid3 false";

	printf("[ local    ]\n");

	// explicit
	// bad tt
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(BADEXPLICITGRIDFILENAME1),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig7 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig7))<< "bad grid1 false";

	// no resolution
	badGridFile.open(
			"./" + std::string(TESTPATH) + "/"
					+ std::string(BADEXPLICITGRIDFILENAME2),
			std::ios::in);
	std::getline(badGridFile, badGridLine);
	badGridFile.close();

	std::shared_ptr<json::Object> badGridConfig8 =
			std::make_shared<json::Object>(json::Deserialize(badGridLine));
	ASSERT_FALSE(aWeb.generateGlobalGrid(badGridConfig8))<< "bad grid2 false";

	printf("[ explicit ]\n");
}
