#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <logger.h>

#include "Site.h"
#include "SiteList.h"
#include "Pick.h"
#include "Correlation.h"
#include "Hypo.h"
#include "Web.h"
#include "Trigger.h"
#include "Glass.h"

#define TESTPATH "testdata"
#define STATIONFILENAME "teststationlist.json"
#define HYPOFILENAME "testhypo1.json"
#define INITFILENAME "initialize.d"

#define LATITUDE -21.849968
#define LONGITUDE 170.034750
#define DEPTH 10.000000
#define TIME 3648585210.926340
#define ID "F6D594930C00134FA1C00B44403F4678"
#define WEB "Tonga_2"
#define BAYES 0.000000
#define THRESH 0.500000
#define CUT 6

#define TRIGGER_LATITUDE -21.849968
#define TRIGGER_LONGITUDE 170.034750
#define TRIGGER_DEPTH 10.000000
#define TRIGGER_TIME 3648585210.926340
#define TRIGGER_SUM 3.5
#define TRIGGER_COUNT 1

#define TRIGGER_SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}" // NOLINT

#define TRIGGER_PICKJSON "{\"ID\":\"20682831\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT

#define TRIGGER_WEB_NAME "TestWeb"
#define TRIGGER_WEB_THRESH 1.4
#define TRIGGER_WEB_NUMDETECT 5
#define TRIGGER_WEB_NUMNUCLEATE 4
#define TRIGGER_WEB_RESOLUTION 100.0
#define TRIGGER_WEB_UPDATE true
#define TRIGGER_WEB_SAVE false

#define CORRELATION_LATITUDE 40.3344
#define CORRELATION_LONGITUDE -121.44
#define CORRELATION_DEPTH 32.44
#define CORRELATION_TIME 3660327044.039

#define ANNEAL_LATITUDE 42.011810141430075
#define ANNEAL_LONGITUDE -119.36313949833216
#define ANNEAL_DEPTH 3.7997271318948496
#define ANNEAL_TIME 3648515731.6680002
#define ANNEAL_BAYES 31.352108346757745

#define LOCALIZE_LATITUDE 42.011810141430075
#define LOCALIZE_LONGITUDE -119.36313949833216
#define LOCALIZE_DEPTH 11.755308705815905
#define LOCALIZE_TIME 3648515732.3961039
#define LOCALIZE_BAYES 31.389001001003624

#define LOCALIZE_RES_LATITUDE 42.011810141430075
#define LOCALIZE_RES_LONGITUDE -119.36313949833216
#define LOCALIZE_RES_DEPTH 11.755308705815905
#define LOCALIZE_RES_TIME 3648515732.3961039
#define LOCALIZE_RES_BAYES 31.389001001003624

#define PRUNESIZE 0
#define RESOLVESIZE 36

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}" // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}" // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

#define PICKJSON "{\"ID\":\"20682831\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK2JSON "{\"ID\":\"20682832\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:02:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK3JSON "{\"ID\":\"20682833\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:03:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define MAXNPICK 3

#define CORRELATIONJSON "{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT

// NOTE: Need to consider testing associate, affinity, anneal,
//  weights, and evaluate functions,
// but that would need a much more involved set of real data, and possibly
// a glass refactor to better support unit testing
// hypo, quake, and event output functions should also be tested.

// check site data for validity
void checkdata(glasscore::CHypo *hypoobject, const std::string &testinfo) {
	// check lat
	double latitude = hypoobject->getLatitude();
	double expectedLatitude = LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 0.0001);

	// check lon
	double longitude = hypoobject->getLongitude();
	double expectedLongitude = LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 0.0001);

	// check depth
	double depth = hypoobject->getDepth();
	double expectedDepth = DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 0.0001);

	// check time
	double time = hypoobject->getTOrigin();
	double expectedTime = TIME;
	ASSERT_NEAR(time, expectedTime, 0.0001);

	// check id
	std::string id = hypoobject->getID();
	std::string expectedId = ID;
	ASSERT_STREQ(id.c_str(), expectedId.c_str());

	// check web
	std::string web = hypoobject->getWebName();
	std::string expectedWeb = std::string(WEB);
	ASSERT_STREQ(web.c_str(), expectedWeb.c_str());

	// check bayes
	double bayes = hypoobject->getBayesValue();
	double expectedBayes = BAYES;
	ASSERT_NEAR(bayes, expectedBayes, 0.0001);

	// check bayes
	double thresh = hypoobject->getNucleationStackThreshold();
	double expectedThresh = THRESH;
	ASSERT_NEAR(thresh, expectedThresh, 0.0001);

	// check cut
	int cut = hypoobject->getNucleationDataThreshold();
	int expectedCut = CUT;
	ASSERT_EQ(cut, expectedCut);
}

// test to see if the hypo can be constructed
TEST(HypoTest, Construction) {
	glass3::util::Logger::disable();

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo();

	// assert default values
	ASSERT_NEAR(0, testHypo->getLatitude(), 0.0001)<< "dLat is zero";
	ASSERT_NEAR(0, testHypo->getLongitude(), 0.0001)<< "dLon is zero";
	ASSERT_NEAR(0, testHypo->getDepth(), 0.0001)<< "dZ is zero";
	ASSERT_NEAR(0, testHypo->getTOrigin(), 0.0001)<< "tOrg is zero";
	ASSERT_STREQ("", testHypo->getID().c_str())<< "sPid is empty";
	ASSERT_STREQ("", testHypo->getWebName().c_str())<< "sWeb is empty";
	ASSERT_NEAR(0, testHypo->getBayesValue(), 0.0001)<< "dBayes is zero";
	ASSERT_NEAR(0, testHypo->getNucleationStackThreshold(), 0.0001)<<
	"dThresh is zero";
	ASSERT_NEAR(0, testHypo->getAssociationDistanceCutoff(), 0.0001)<<
	"nCut is zero";
	ASSERT_EQ(0, testHypo->getProcessCount())<< "iCycle is zero";
	ASSERT_NEAR(0, testHypo->getMedianDistance(), 0.0001)<< "dMed is zero";
	ASSERT_NEAR(0, testHypo->getMinDistance(), 0.0001)<< "dMin is zero";
	ASSERT_NEAR(0, testHypo->getGap(), 0.0001)<< "dGap is zero";
	ASSERT_NEAR(0, testHypo->getDistanceSD(), 0.0001)<< "dSig is zero";
	ASSERT_FALSE(testHypo->getFixed())<< "bFixed is false";
	ASSERT_FALSE(testHypo->getEventGenerated())<< "bEvent is false";

	ASSERT_EQ(0, testHypo->getPickDataSize())<< "vPick size is zero";

	// now init
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	testHypo->initialize(LATITUDE, LONGITUDE, DEPTH, TIME, std::string(ID),
							std::string(WEB), BAYES, THRESH, CUT, nullTrav,
							nullTrav, nullTTT);

	// check results
	checkdata(testHypo, "initialize check");
}

// test to see if the hypo can be constructed from a correlation
TEST(HypoTest, CorrelationConstruction) {
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

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a correlation using a JSON object
	std::shared_ptr<json::Object> correlationJSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATIONJSON))));
	glasscore::CCorrelation * testCorrelation = new glasscore::CCorrelation(
			correlationJSON, testSiteList);
	std::shared_ptr<glasscore::CCorrelation> sharedCorrelation(testCorrelation);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			sharedCorrelation, testGlass->getDefaultNucleationTravelTime(),
			nullTrav, testGlass->getAssociationTravelTimes());

	// check lat
	double latitude = testHypo->getLatitude();
	double expectedLatitude = CORRELATION_LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 0.0001);

	// check lon
	double longitude = testHypo->getLongitude();
	double expectedLongitude = CORRELATION_LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 0.0001);

	// check depth
	double depth = testHypo->getDepth();
	double expectedDepth = CORRELATION_DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 0.0001);

	// check time
	double time = testHypo->getTOrigin();
	double expectedTime = CORRELATION_TIME;
	ASSERT_NEAR(time, expectedTime, 0.0001);

}

// test to see if the hypo can be constructed from a correlation
TEST(HypoTest, TriggerConstruction) {
	glass3::util::Logger::disable();

	// construct a web
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	glasscore::CWeb * testWeb = new glasscore::CWeb(
			std::string(TRIGGER_WEB_NAME),
			TRIGGER_WEB_THRESH,
			TRIGGER_WEB_NUMDETECT,
			TRIGGER_WEB_NUMNUCLEATE,
			TRIGGER_WEB_RESOLUTION,
			TRIGGER_WEB_UPDATE,
			TRIGGER_WEB_SAVE, nullTrav, nullTrav);

	// create  shared pointer to the site
	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(TRIGGER_SITEJSON))));
	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(TRIGGER_PICKJSON))));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSiteFromJSON(siteJSON);

	// create pick
	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, testSiteList);

	std::shared_ptr<glasscore::CPick> sharedPick(testPick);

	// create pick vector
	std::vector<std::shared_ptr<glasscore::CPick>> picks;
	picks.push_back(sharedPick);

	glasscore::CTrigger * testTrigger = new glasscore::CTrigger(
			TRIGGER_LATITUDE,
			TRIGGER_LONGITUDE,
			TRIGGER_DEPTH,
			TRIGGER_TIME,
			TRIGGER_WEB_RESOLUTION,
			TRIGGER_SUM,
			TRIGGER_COUNT, picks, testWeb);
	std::shared_ptr<glasscore::CTrigger> sharedTrigger(testTrigger);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(sharedTrigger, nullTTT);

	// check lat
	double latitude = testHypo->getLatitude();
	double expectedLatitude = TRIGGER_LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 0.0001);

	// check lon
	double longitude = testHypo->getLongitude();
	double expectedLongitude = TRIGGER_LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 0.0001);

	// check depth
	double depth = testHypo->getDepth();
	double expectedDepth = TRIGGER_DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 0.0001);

	// check time
	double time = testHypo->getTOrigin();
	double expectedTime = TRIGGER_TIME;
	ASSERT_NEAR(time, expectedTime, 0.0001);

}

// test to see if the hypopick operations work
TEST(HypoTest, PickOperations) {
	glass3::util::Logger::disable();

	// construct a hypo
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	glasscore::CHypo * testHypo = new glasscore::CHypo(LATITUDE, LONGITUDE,
	DEPTH,
														TIME, std::string(ID),
														std::string(WEB), BAYES,
														THRESH,
														CUT, nullTrav, nullTrav,
														nullTTT);

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> site2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE2JSON))));
	std::shared_ptr<json::Object> site3JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE3JSON))));

	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICKJSON))));
	std::shared_ptr<json::Object> pick2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICK2JSON))));
	std::shared_ptr<json::Object> pick3JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICK3JSON))));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSiteFromJSON(siteJSON);
	testSiteList->addSiteFromJSON(site2JSON);
	testSiteList->addSiteFromJSON(site3JSON);

	// create picks
	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, testSiteList);
	glasscore::CPick * testPick2 = new glasscore::CPick(pick2JSON,
														testSiteList);
	glasscore::CPick * testPick3 = new glasscore::CPick(pick3JSON,
														testSiteList);

	// create new shared pointers to the picks
	std::shared_ptr<glasscore::CPick> sharedPick(testPick);
	std::shared_ptr<glasscore::CPick> sharedPick2(testPick2);
	std::shared_ptr<glasscore::CPick> sharedPick3(testPick3);

	// add picks to hypo
	testHypo->addPickReference(sharedPick);
	testHypo->addPickReference(sharedPick2);
	testHypo->addPickReference(sharedPick3);

	// check to make sure the size isn't any larger than our max
	int expectedSize = MAXNPICK;
	ASSERT_EQ(expectedSize, testHypo->getPickDataSize())<<
	"hypo vPick not larger than max";

	// remove picks from hypo
	testHypo->removePickReference(sharedPick);
	testHypo->removePickReference(sharedPick2);

	// check to see that only one pick remains
	expectedSize = 1;
	ASSERT_EQ(expectedSize, testHypo->getPickDataSize())<< "hypo has only one pick";
}

// test to see if the localize operation works
TEST(HypoTest, Anneal) {
	// glass3::util::log_init("localizetest", "debug", ".", true);
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

	// hypo
	std::ifstream hypoFile;
	hypoFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(HYPOFILENAME),
			std::ios::in);
	std::string hypoLine = "";
	std::getline(hypoFile, hypoLine);
	hypoFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> hypoMessage = std::make_shared<json::Object>(
			json::Deserialize(hypoLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			hypoMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	// relocate it
	testHypo->anneal();

	// check lat
	double latitude = testHypo->getLatitude();
	double expectedLatitude = ANNEAL_LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 1.0);

	// check lon
	double longitude = testHypo->getLongitude();
	double expectedLongitude = ANNEAL_LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 1.0);

	// check depth
	double depth = testHypo->getDepth();
	double expectedDepth = ANNEAL_DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 50.0);

	// check time
	double time = testHypo->getTOrigin();
	double expectedTime = ANNEAL_TIME;
	ASSERT_NEAR(time, expectedTime, 1.0);

	// check bayes
	double bayes = testHypo->getBayesValue();
	double expectedBayes = ANNEAL_BAYES;
	ASSERT_NEAR(bayes, expectedBayes, 1.0);
}

// test to see if the localize operation works
TEST(HypoTest, Localize) {
	// glass3::util::log_init("localizetest", "debug", ".", true);
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

	// hypo
	std::ifstream hypoFile;
	hypoFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(HYPOFILENAME),
			std::ios::in);
	std::string hypoLine = "";
	std::getline(hypoFile, hypoLine);
	hypoFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> hypoMessage = std::make_shared<json::Object>(
			json::Deserialize(hypoLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			hypoMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	// relocate it
	testHypo->localize();

	// check lat
	double latitude = testHypo->getLatitude();
	double expectedLatitude = LOCALIZE_LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 1.0);

	// check lon
	double longitude = testHypo->getLongitude();
	double expectedLongitude = LOCALIZE_LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 1.0);

	// check depth
	double depth = testHypo->getDepth();
	double expectedDepth = LOCALIZE_DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 1.0);

	// check time
	double time = testHypo->getTOrigin();
	double expectedTime = LOCALIZE_TIME;
	ASSERT_NEAR(time, expectedTime, 1.0);

	// check bayes
	double bayes = testHypo->getBayesValue();
	double expectedBayes = LOCALIZE_BAYES;
	ASSERT_NEAR(bayes, expectedBayes, 1.0);

	// switch to residual locator
	testGlass->setMinimizeTTLocator(true);

	// relocate it
	testHypo->localize();

	// check lat
	latitude = testHypo->getLatitude();
	expectedLatitude = LOCALIZE_RES_LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 1.0);

	// check lon
	longitude = testHypo->getLongitude();
	expectedLongitude = LOCALIZE_RES_LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 1.0);

	// check depth
	depth = testHypo->getDepth();
	expectedDepth = LOCALIZE_RES_DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 50.0);

	// check time
	time = testHypo->getTOrigin();
	expectedTime = LOCALIZE_RES_TIME;
	ASSERT_NEAR(time, expectedTime, 1.0);

	// check bayes
	bayes = testHypo->getBayesValue();
	expectedBayes = LOCALIZE_RES_BAYES;
	ASSERT_NEAR(bayes, expectedBayes, 1.0);
}

// test to see if the localize operation works
TEST(HypoTest, Prune) {
	// glass3::util::log_init("localizetest", "debug", ".", true);
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

	// hypo
	std::ifstream hypoFile;
	hypoFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(HYPOFILENAME),
			std::ios::in);
	std::string hypoLine = "";
	std::getline(hypoFile, hypoLine);
	hypoFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> hypoMessage = std::make_shared<json::Object>(
			json::Deserialize(hypoLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			hypoMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	// prune it
	testHypo->pruneData();

	ASSERT_EQ(PRUNESIZE, testHypo->getPickDataSize())<< "vPick size";
}

// test to see if the localize operation works
TEST(HypoTest, Resolve) {
	// glass3::util::log_init("localizetest", "debug", ".", true);
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

	// hypo
	std::ifstream hypoFile;
	hypoFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(HYPOFILENAME),
			std::ios::in);
	std::string hypoLine = "";
	std::getline(hypoFile, hypoLine);
	hypoFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> hypoMessage = std::make_shared<json::Object>(
			json::Deserialize(hypoLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			hypoMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	// prune it
	testHypo->resolveData(sharedHypo);

	ASSERT_EQ(RESOLVESIZE, testHypo->getPickDataSize())<< "vPick size";
}

// test to see if the localize operation works
TEST(HypoTest, Messaging) {
	// glass3::util::log_init("localizetest", "debug", ".", true);
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

	// hypo
	std::ifstream hypoFile;
	hypoFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(HYPOFILENAME),
			std::ios::in);
	std::string hypoLine = "";
	std::getline(hypoFile, hypoLine);
	hypoFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> sharedHypoMessage = std::make_shared<
			json::Object>(json::Deserialize(hypoLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo(
			sharedHypoMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	std::shared_ptr<json::Object> hypoMessage =
			sharedHypo->generateHypoMessage();
	ASSERT_TRUE(hypoMessage != NULL)<< "hypo message";

	std::shared_ptr<json::Object> eventMessage = sharedHypo
			->generateEventMessage();
	ASSERT_TRUE(eventMessage != NULL)<< "event message";

	std::shared_ptr<json::Object> cancelMessage = sharedHypo
			->generateCancelMessage();
	ASSERT_TRUE(cancelMessage != NULL)<< "cancel message";

	std::shared_ptr<json::Object> expireMessage = sharedHypo
			->generateExpireMessage();
	ASSERT_TRUE(expireMessage != NULL)<< "expire message";
}

