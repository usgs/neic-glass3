#include <gtest/gtest.h>

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <fstream>

#include <logger.h>

#include "Web.h"
#include "WebList.h"
#include "Site.h"
#include "SiteList.h"

#define TESTPATH "testdata"
#define STATIONFILENAME "teststationlist.json"
#define GRIDFILENAME "testgrid.d"

#define REMWEB "{\"Cmd\":\"RemoveWeb\",\"Name\":\"TestGrid\"}"

#define ADDSITE "{\"Elevation\":302.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.656729,\"Longitude\":-97.609276,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"BCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT
#define REMOVESITE "{\"Elevation\":378.000000,\"Enable\":false,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"station-lookup-app\"},\"Latitude\":35.356842,\"Longitude\":-97.656074,\"Quality\":1.000000,\"Site\":{\"Channel\":\"HHZ\",\"Location\":\"--\",\"Network\":\"OK\",\"Station\":\"CCOK\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT

// tests to see if the weblist can be constructed
TEST(WebListTest, Construction) {
	glass3::util::Logger::disable();

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();

	// lists
	ASSERT_EQ(0, (int)testWebList->size())<< "web list empty";
}

// tests adding a web to the web list
TEST(WebListTest, AddWeb) {
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

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->setSiteList(testSiteList);

	// web list
	ASSERT_EQ(0, (int)testWebList->size())<< "web list empty";

	// add a web
	testWebList->receiveExternalMessage(gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->size())<< "web list added";
}

// tests removing a web from the web list
TEST(WebListTest, RemWeb) {
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

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->setSiteList(testSiteList);

	// web list
	ASSERT_EQ(0, (int)testWebList->size())<< "web list empty";

	// add a web
	testWebList->receiveExternalMessage(gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->size())<< "web list added";

	std::shared_ptr<json::Object> remGridConfig =
			std::make_shared<json::Object>(
					json::Deserialize(std::string(REMWEB)));

	// remove a web
	testWebList->receiveExternalMessage(remGridConfig);

	// web list
	ASSERT_EQ(0, (int)testWebList->size())<< "web list removed";
}

// test various site operations (add, remove) with the web list
TEST(WebListTest, SiteOperations) {
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

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();
	testWebList->setSiteList(testSiteList);

	// web list
	ASSERT_EQ(0, (int)testWebList->size())<< "web list empty";

	// add a web
	testWebList->receiveExternalMessage(gridConfig);

	// web list
	ASSERT_EQ(1, (int)testWebList->size())<< "web list added";

	// create site to add
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(ADDSITE))));
	glasscore::CSite * addSite = new glasscore::CSite(siteJSON);
	std::shared_ptr<glasscore::CSite> sharedAddSite(addSite);

	// add to site list
	testSiteList->addSite(sharedAddSite);
	testWebList->addSite(sharedAddSite);

	// give time for site to add
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// check to see if this site is in web list
	ASSERT_TRUE(testWebList->hasSite(sharedAddSite))<< "site added";

	// create site to remove
	std::shared_ptr<json::Object> siteJSON2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(REMOVESITE))));
	glasscore::CSite * removeSite = new glasscore::CSite(siteJSON2);
	std::shared_ptr<glasscore::CSite> sharedRemoveSite(removeSite);

	// update in site list
	testSiteList->addSite(sharedRemoveSite);

	// check to see if this site is in grid
	ASSERT_TRUE(testWebList->hasSite(sharedRemoveSite))<< "site in weblist";

	// remove
	testWebList->removeSite(sharedRemoveSite);

	// give time for site to remove
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// check to see if this site is in grid
	ASSERT_FALSE(testWebList->hasSite(sharedRemoveSite))<< "site removed";
}

// Tests various falure cases for weblist
TEST(WebListTest, FailTests) {
	glass3::util::Logger::disable();

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();

	ASSERT_FALSE(testWebList->receiveExternalMessage(NULL))<< "Null dispatch false";
	ASSERT_FALSE(testWebList->addWeb(NULL))<< "Null addWeb false";
	ASSERT_FALSE(testWebList->removeWeb(NULL))<< "Null removeWeb false";
	testWebList->removeSite(NULL);
	testWebList->addSite(NULL);
}
