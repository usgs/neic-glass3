#include <gtest/gtest.h>

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <fstream>

#include "Web.h"
#include "WebList.h"
#include "Site.h"
#include "SiteList.h"
#include "Logit.h"

#define TESTPATH "testdata"
#define STATIONFILENAME "teststationlist.json"
#define GRIDFILENAME "testgrid.d"

#define REMWEB "{\"Cmd\":\"RemoveWeb\",\"Name\":\"TestGrid\"}"

#define ADDSITE "{\"Elevation\":302.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.656729,\"Longitude\":-97.609276,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"BCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT
#define REMOVESITE "{\"Elevation\":378.000000,\"Enable\":false,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.356842,\"Longitude\":-97.656074,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"CCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT

// tests to see if the weblist can be constructed
TEST(WebListTest, Construction) {
	glassutil::CLogit::disable();

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();

	// lists
	ASSERT_EQ(0, (int)testWebList->vWeb.size())<< "web list empty";

	// pointers
	ASSERT_EQ(NULL, testWebList->pGlass)<< "pGlass null";

	ASSERT_TRUE(testWebList->statusCheck())<< "status check";

	delete (testWebList);
}

// tests adding a web to the web list
TEST(WebListTest, AddWeb) {
	glassutil::CLogit::disable();

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

	json::Object siteList = json::Deserialize(stationLine);
	json::Object gridConfig = json::Deserialize(gridLine);

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->dispatch(&siteList);

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->pSiteList = testSiteList;

	// web list
	ASSERT_EQ(0, (int)testWebList->vWeb.size())<< "web list empty";

	// add a web
	testWebList->dispatch(&gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->vWeb.size())<< "web list added";

	ASSERT_TRUE(testWebList->statusCheck())<< "status check";

	delete (testSiteList);
	delete (testWebList);
}

// tests removing a web from the web list
TEST(WebListTest, RemWeb) {
	glassutil::CLogit::disable();

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

	json::Object siteList = json::Deserialize(stationLine);
	json::Object gridConfig = json::Deserialize(gridLine);

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->dispatch(&siteList);

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->pSiteList = testSiteList;

	// web list
	ASSERT_EQ(0, (int)testWebList->vWeb.size())<< "web list empty";

	// add a web
	testWebList->dispatch(&gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->vWeb.size())<< "web list added";

	json::Object remGridConfig = json::Deserialize(std::string(REMWEB));

	// remove a web
	testWebList->dispatch(&remGridConfig);

	// web list
	ASSERT_EQ(0, (int)testWebList->vWeb.size())<< "web list removed";

	delete (testSiteList);
	delete (testWebList);
}

// test various site operations (add, remove) with the web list
TEST(WebListTest, SiteOperations) {
	glassutil::CLogit::disable();

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

	json::Object siteList = json::Deserialize(stationLine);
	json::Object gridConfig = json::Deserialize(gridLine);

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->dispatch(&siteList);

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->pSiteList = testSiteList;

	// web list
	ASSERT_EQ(0, (int)testWebList->vWeb.size())<< "web list empty";

	// add a web
	testWebList->dispatch(&gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->vWeb.size())<< "web list added";

	// create site to add
	json::Object siteJSON = json::Deserialize(std::string(ADDSITE));
	glasscore::CSite * addSite = new glasscore::CSite(&siteJSON, NULL);
	std::shared_ptr<glasscore::CSite> sharedAddSite(addSite);

	// add to site list
	testSiteList->addSite(sharedAddSite);
	testWebList->addSite(sharedAddSite);

	// give time for site to add
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// check to see if this site is in web list
	ASSERT_TRUE(testWebList->hasSite(sharedAddSite))<< "site added";

	// create site to remove
	json::Object siteJSON2 = json::Deserialize(std::string(REMOVESITE));
	glasscore::CSite * removeSite = new glasscore::CSite(&siteJSON2, NULL);
	std::shared_ptr<glasscore::CSite> sharedRemoveSite(removeSite);

	// update in site list
	testSiteList->addSite(sharedRemoveSite);

	// check to see if this site is in grid
	ASSERT_TRUE(testWebList->hasSite(sharedRemoveSite))<< "site in weblist";

	// remove
	testWebList->remSite(sharedRemoveSite);

	// give time for site to remove
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// check to see if this site is in grid
	ASSERT_FALSE(testWebList->hasSite(sharedRemoveSite))<< "site removed";

	ASSERT_TRUE(testWebList->statusCheck())<< "status check";

	delete (testSiteList);
	delete (testWebList);
}

// Tests various falure cases for weblist
TEST(WebListTest, FailTests) {
	glassutil::CLogit::disable();

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();

	ASSERT_FALSE(testWebList->dispatch(NULL))<< "Null dispatch false";
	ASSERT_FALSE(testWebList->addWeb(NULL))<< "Null addWeb false";
	ASSERT_FALSE(testWebList->removeWeb(NULL))<< "Null removeWeb false";
	testWebList->remSite(NULL);
	testWebList->addSite(NULL);

	delete (testWebList);
}
