#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Site.h"
#include "SiteList.h"
#include "Pick.h"
#include "Hypo.h"
#include "Logit.h"

#define LATITUDE -21.849968
#define LONGITUDE 170.034750
#define DEPTH 10.000000
#define TIME 3648585210.926340
#define ID "F6D594930C00134FA1C00B44403F4678"
#define WEB "Tonga_2"
#define BAYES 0.000000
#define THRESH 0.500000
#define CUT 6

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}" // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}" // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

#define PICKJSON "{\"ID\":\"20682831\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK2JSON "{\"ID\":\"20682832\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:02:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK3JSON "{\"ID\":\"20682833\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:03:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define MAXNPICK 3

// NOTE: Need to consider testing associate, prune, affinity, anneal, localize,
// focus, iterate, weights, and evaluate functions,
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
	glassutil::CLogit::disable();

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo();

	// assert default values
	ASSERT_EQ(0, testHypo->getLatitude())<< "dLat is zero";
	ASSERT_EQ(0, testHypo->getLongitude())<< "dLon is zero";
	ASSERT_EQ(0, testHypo->getDepth())<< "dZ is zero";
	ASSERT_EQ(0, testHypo->getTOrigin())<< "tOrg is zero";
	ASSERT_STREQ("", testHypo->getID().c_str())<< "sPid is empty";
	ASSERT_STREQ("", testHypo->getWebName().c_str())<< "sWeb is empty";
	ASSERT_EQ(0, testHypo->getBayesValue())<< "dBayes is zero";
	ASSERT_EQ(0, testHypo->getNucleationStackThreshold())<< "dThresh is zero";
	ASSERT_EQ(0, testHypo->getAssociationDistanceCutoff())<< "nCut is zero";
	ASSERT_EQ(0, testHypo->getCycleCount())<< "iCycle is zero";
	ASSERT_EQ(0, testHypo->getMedianDistance())<< "dMed is zero";
	ASSERT_EQ(0, testHypo->getMinDistance())<< "dMin is zero";
	ASSERT_EQ(0, testHypo->getGap())<< "dGap is zero";
	ASSERT_EQ(0, testHypo->getDistanceSD())<< "dSig is zero";
	ASSERT_EQ(0, testHypo->getKurtosisValue())<< "dKrt is zero";
	ASSERT_FALSE(testHypo->getFixed())<< "bFixed is false";
	ASSERT_FALSE(testHypo->getEventSent())<< "bEvent is false";

	ASSERT_EQ(0, testHypo->getPickDataSize())<< "vPick size is zero";

	// pointers
	ASSERT_TRUE(testHypo->getGlass() == NULL)<< "pGlass null";

	// now init
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	testHypo->initialize(LATITUDE, LONGITUDE, DEPTH, TIME, std::string(ID),
							std::string(WEB), BAYES, THRESH, CUT, nullTrav,
							nullTrav, nullTTT);

	// check results
	checkdata(testHypo, "initialize check");
}

// test to see if the hypopick operations work
TEST(HypoTest, PickOperations) {
	glassutil::CLogit::disable();

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
	testSiteList->addSite(siteJSON);
	testSiteList->addSite(site2JSON);
	testSiteList->addSite(site3JSON);

	// create picks
	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, 1,
														testSiteList);
	glasscore::CPick * testPick2 = new glasscore::CPick(pick2JSON, 2,
														testSiteList);
	glasscore::CPick * testPick3 = new glasscore::CPick(pick3JSON, 3,
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
