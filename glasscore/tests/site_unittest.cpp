#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <cmath>

#include <logger.h>
#include <geo.h>

#include "Site.h"
#include "Pick.h"
#include "Node.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

#define SCNL "LRM.EHZ.MB"
#define SITE "LRM"
#define COMP "EHZ"
#define NET "MB"
#define LOC ""
#define LATITUDE 45.822170
#define LONGITUDE -112.451000
#define ELEVATION 2326.000000
#define QUALITY 1.0
#define GEOCENTRIC_LATITUDE 45.628982
#define GEOCENTRID_ELEVATION 6373.326
#define SITE2DISTANCE 109.66700963282658
#define SITE2DELTA 0.017241728546897175
#define USE true
#define USEFORTELE true

// NOTE: Need to consider testing nucleate function, but that would need a
// much more involved set of real nodes and data, not these dummy nodes.
// Maybe consider performing this test at a higher level?

// check site data for validity
void checkdata(glasscore::CSite * siteobject, const std::string &testinfo) {
	// check scnl
	std::string sitescnl = siteobject->getSCNL();
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str());

	// check site
	std::string sitesite = siteobject->getSite();
	std::string expectedsite = std::string(SITE);
	ASSERT_STREQ(sitesite.c_str(), expectedsite.c_str());

	// check comp
	std::string sitecomp = siteobject->getComponent();
	std::string expectedcomp = std::string(COMP);
	ASSERT_STREQ(sitecomp.c_str(), expectedcomp.c_str());

	// check net
	std::string sitenet = siteobject->getNetwork();
	std::string expectednet = std::string(NET);
	ASSERT_STREQ(sitenet.c_str(), expectednet.c_str());

	// check loc
	std::string siteloc = siteobject->getLocation();
	std::string expectedloc = std::string(LOC);
	ASSERT_STREQ(siteloc.c_str(), expectedloc.c_str());

	// check latitude
	double sitelatitude = siteobject->getGeo().m_dGeocentricLatitude;
	// NOTE: expected latitude is in geocentric coordinates
	double expectedlatitude = GEOCENTRIC_LATITUDE;
	ASSERT_NEAR(sitelatitude, expectedlatitude, 0.000001);

	// check longitude
	double sitelongitude = siteobject->getGeo().m_dGeocentricLongitude;
	// NOTE: expected longitude is the same in geocentric and geographic
	// coordinates
	double expectedlongitude = LONGITUDE;
	ASSERT_NEAR(sitelongitude, expectedlongitude, 0.000001);

	// check elevation
	double siteelevation = siteobject->getGeo().m_dGeocentricRadius;
	// NOTE: expected elevation is in geocentric coordinates
	double expectedelevation = GEOCENTRID_ELEVATION;
	ASSERT_NEAR(siteelevation, expectedelevation, 0.000001);

	// check use
	bool siteuse = siteobject->getUse();
	bool expecteduse = USE;
	ASSERT_EQ(siteuse, expecteduse);

	// check useforTele
	bool siteusefortele = siteobject->getUseForTeleseismic();
	bool expectedusefortele = USEFORTELE;
	ASSERT_EQ(siteusefortele, expectedusefortele);

	// check qual
	double sitequal = siteobject->getQuality();
	double expectedqual = QUALITY;
	ASSERT_NEAR(sitequal, expectedqual, 0.000001);
}

// tests to see if the site can be constructed
TEST(SiteTest, Construction) {
	glass3::util::Logger::disable();

	// construct a site
	glasscore::CSite * testSite = new glasscore::CSite();

	// assert default values
	// scnl
	ASSERT_STREQ("", testSite->getSCNL().c_str())<< "sScnl Empty";
	ASSERT_STREQ("", testSite->getSite().c_str())<< "sSite Empty";
	ASSERT_STREQ("", testSite->getComponent().c_str())<< "sComp Empty";
	ASSERT_STREQ("", testSite->getNetwork().c_str())<< "sNet Empty";
	ASSERT_STREQ("", testSite->getLocation().c_str())<< "sLoc Empty";

	ASSERT_EQ(true, testSite->getUse())<< "bUse false";
	ASSERT_EQ(true, testSite->getUseForTeleseismic())<< "bUseForTele false";
	ASSERT_EQ(1, testSite->getQuality())<< "dQual one";

	// geographic
	ASSERT_EQ(0, testSite->getGeo().m_dGeocentricLatitude)<<
			"geo.m_dGeocentricLatitude 0";
	ASSERT_EQ(0, testSite->getGeo().m_dGeocentricLongitude)<<
			"geo.m_dGeocentricLongitude 0";
	ASSERT_EQ(0, testSite->getGeo().m_dGeocentricRadius)<<
			"geo.m_dGeocentricRadius 0";

	// lists
	ASSERT_EQ(0, testSite->getNodeLinksCount())<< "vNode.size() 0";

	// now init
	testSite->initialize(std::string(SITE), std::string(COMP), std::string(NET),
							std::string(LOC), LATITUDE, LONGITUDE,
							ELEVATION,
							QUALITY,
							USE,
							USEFORTELE);

	// check results
	checkdata(testSite, "initialize check");

	// cleanup
	delete (testSite);
}

// tests to see if the site can be constructed from JSON
TEST(SiteTest, JSONConstruction) {
	glass3::util::Logger::disable();

	// create a json object from the string
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// construct a site using a JSON object
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);

	// check results
	checkdata(testSite, "json construction check");

	// cleanup
	delete (testSite);
}

// tests to see if the distance functions are working
TEST(SiteTest, Distance) {
	glass3::util::Logger::disable();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> site2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE2JSON))));

	// construct sites using JSON objects
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);
	glasscore::CSite * testSite2 = new glasscore::CSite(site2JSON);

	// create new shared pointer to the site
	std::shared_ptr<glasscore::CSite> sharedTestSite2(testSite2);

	// test getDistance
	double expectedDistance = SITE2DISTANCE;
	ASSERT_DOUBLE_EQ(expectedDistance, testSite->getDistance(sharedTestSite2));

	// test getDelta
	double expectedDelta = SITE2DELTA;
	ASSERT_DOUBLE_EQ(expectedDelta, testSite->getDelta(&testSite2->getGeo()));
}

// tests to see if picks can be added to and removed from the site
TEST(SiteTest, PickOperations) {
	glass3::util::Logger::disable();

	// create a json object from the string
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> site2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE2JSON))));

	// construct a site using a JSON object
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON));
	std::shared_ptr<glasscore::CSite> sharedTestSite2(
			new glasscore::CSite(site2JSON));

	// create pick objects
	glasscore::CPick * testPick = new glasscore::CPick(sharedTestSite, 10.0,
														"1", -1, -1);
	glasscore::CPick * testPick2 = new glasscore::CPick(sharedTestSite2, 11.1,
														"2", -1, -1);
	glasscore::CPick * testPick3 = new glasscore::CPick(sharedTestSite, 12.2,
														"3", -1, -1);
	glasscore::CPick * testPick4 = new glasscore::CPick(sharedTestSite, 13.3,
														"4", -1, -1);
	glasscore::CPick * testPick5 = new glasscore::CPick(sharedTestSite, 14.4,
														"5", -1, -1);
	glasscore::CPick * testPick6 = new glasscore::CPick(sharedTestSite, 15.5,
														"6", -1, -1);
	glasscore::CPick * testPick7 = new glasscore::CPick(sharedTestSite, 16.6,
														"7", -1, -1);
	glasscore::CPick * testPick8 = new glasscore::CPick(sharedTestSite, 17.7,
														"8", -1, -1);
	glasscore::CPick * testPick9 = new glasscore::CPick(sharedTestSite, 18.8,
														"9", -1, -1);
	glasscore::CPick * testPick10 = new glasscore::CPick(sharedTestSite, 19.9,
															"10", -1, -1);

	// create new shared pointers to the picks
	std::shared_ptr<glasscore::CPick> sharedTestPick(testPick);
	std::shared_ptr<glasscore::CPick> sharedTestPick2(testPick2);
	std::shared_ptr<glasscore::CPick> sharedTestPick3(testPick3);
	std::shared_ptr<glasscore::CPick> sharedTestPick4(testPick4);
	std::shared_ptr<glasscore::CPick> sharedTestPick5(testPick5);
	std::shared_ptr<glasscore::CPick> sharedTestPick6(testPick6);
	std::shared_ptr<glasscore::CPick> sharedTestPick7(testPick7);
	std::shared_ptr<glasscore::CPick> sharedTestPick8(testPick8);
	std::shared_ptr<glasscore::CPick> sharedTestPick9(testPick9);
	std::shared_ptr<glasscore::CPick> sharedTestPick10(testPick10);

	// test adding pick to site
	testSite->addPick(sharedTestPick);
	int expectedSize = 1;
	ASSERT_EQ(expectedSize, testSite->getPickCount())<< "Added Pick";

	// test adding pick from different station
	testSite->addPick(sharedTestPick2);
	ASSERT_EQ(expectedSize, testSite->getPickCount())<<
	"Added pick from different station";

	// add more sites
	testSite->addPick(sharedTestPick3);
	testSite->addPick(sharedTestPick4);
	testSite->addPick(sharedTestPick5);
	testSite->addPick(sharedTestPick6);
	testSite->addPick(sharedTestPick7);
	testSite->addPick(sharedTestPick8);
	testSite->addPick(sharedTestPick9);
	testSite->addPick(sharedTestPick10);
	expectedSize = 9;
	ASSERT_EQ(expectedSize, testSite->getPickCount())<< "Added more Picks";

	double min = 12.5;
	double max = 17.0;

	// get the picks
	std::vector<std::shared_ptr<glasscore::CPick>> testPicks = testSite
			->getPicks(min, max);
	expectedSize = 4;
	ASSERT_EQ(expectedSize, testPicks.size())<< "test pick vector size";
	ASSERT_STREQ("4", testPicks[0]->getID().c_str())<< "start pick vector";
	ASSERT_STREQ("7", testPicks[3]->getID().c_str())<< "end pick vector";

	// test removing pick
	testSite->removePick(sharedTestPick);
	expectedSize = 8;
	ASSERT_EQ(expectedSize, testSite->getPickCount())<< "Removed pick";
}

// tests to see if nodes can be added to and removed from the site
TEST(SiteTest, NodeOperations) {
	glass3::util::Logger::disable();

	// create a json object from the string
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// construct a site using a JSON object
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);

	// create node objects
	glasscore::CNode * testNode = new glasscore::CNode("test", 0.0, 0.0, 10,
														100);
	glasscore::CNode * testNode2 = new glasscore::CNode("test2", 0.897, 0.897,
														10, 100);

	// create new shared pointers to the nodes
	std::shared_ptr<glasscore::CNode> sharedNode(testNode);
	std::shared_ptr<glasscore::CNode> sharedNode2(testNode2);

	// test adding nodes to site
	testSite->addNode(sharedNode, 5.0, 10.0);
	testSite->addNode(sharedNode2, 6.0, 12.0);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testSite->getNodeLinksCount())<< "Added Nodes";

	// test removing nodes from site
	testSite->removeNode(testNode->getID());
	expectedSize = 1;
	ASSERT_EQ(expectedSize, testSite->getNodeLinksCount())<< "Removed Node";
}
