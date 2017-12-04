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
void checkdata(glasscore::CHypo *hypoobject, std::string testinfo) {
	// check lat
	double latitude = hypoobject->dLat;
	double expectedLatitude = LATITUDE;
	ASSERT_NEAR(latitude, expectedLatitude, 0.0001);

	// check lon
	double longitude = hypoobject->dLon;
	double expectedLongitude = LONGITUDE;
	ASSERT_NEAR(longitude, expectedLongitude, 0.0001);

	// check depth
	double depth = hypoobject->dZ;
	double expectedDepth = DEPTH;
	ASSERT_NEAR(depth, expectedDepth, 0.0001);

	// check time
	double time = hypoobject->tOrg;
	double expectedTime = TIME;
	ASSERT_NEAR(time, expectedTime, 0.0001);

	// check id
	std::string id = hypoobject->sPid;
	std::string expectedId = ID;
	ASSERT_STREQ(id.c_str(), expectedId.c_str());

	// check web
	std::string web = hypoobject->sWeb;
	std::string expectedWeb = std::string(WEB);
	ASSERT_STREQ(web.c_str(), expectedWeb.c_str());

	// check bayes
	double bayes = hypoobject->dBayes;
	double expectedBayes = BAYES;
	ASSERT_NEAR(bayes, expectedBayes, 0.0001);

	// check bayes
	double thresh = hypoobject->dThresh;
	double expectedThresh = THRESH;
	ASSERT_NEAR(thresh, expectedThresh, 0.0001);

	// check cut
	int cut = hypoobject->nCut;
	int expectedCut = CUT;
	ASSERT_EQ(cut, expectedCut);
}

// test to see if the hypo can be constructed
TEST(HypoTest, Construction) {
	glassutil::CLogit::disable();

	// construct a hypo
	glasscore::CHypo * testHypo = new glasscore::CHypo();

	// assert default values
	ASSERT_EQ(0, testHypo->dLat)<< "dLat is zero";
	ASSERT_EQ(0, testHypo->dLon)<< "dLon is zero";
	ASSERT_EQ(0, testHypo->dZ)<< "dZ is zero";
	ASSERT_EQ(0, testHypo->tOrg)<< "tOrg is zero";
	ASSERT_STREQ("", testHypo->sPid.c_str())<< "sPid is empty";
	ASSERT_STREQ("", testHypo->sWeb.c_str())<< "sWeb is empty";
	ASSERT_EQ(0, testHypo->dBayes)<< "dBayes is zero";
	ASSERT_EQ(0, testHypo->dThresh)<< "dThresh is zero";
	ASSERT_EQ(0, testHypo->nCut)<< "nCut is zero";
	ASSERT_EQ(0, testHypo->iCycle)<< "iCycle is zero";
	ASSERT_EQ(0, testHypo->nWts)<< "nWts is zero";
	ASSERT_EQ(0, testHypo->dMed)<< "dMed is zero";
	ASSERT_EQ(0, testHypo->dMin)<< "dMin is zero";
	ASSERT_EQ(0, testHypo->dGap)<< "dGap is zero";
	ASSERT_EQ(0, testHypo->dSig)<< "dSig is zero";
	ASSERT_EQ(0, testHypo->dKrt)<< "dKrt is zero";
	ASSERT_FALSE(testHypo->bFixed)<< "bFixed is false";
	ASSERT_FALSE(testHypo->bRefine)<< "bRefine is false";
	ASSERT_FALSE(testHypo->bQuake)<< "bQuake is false";
	ASSERT_FALSE(testHypo->bEvent)<< "bEvent is false";

	ASSERT_EQ(0, testHypo->vPick.size())<< "vPick size is zero";
	ASSERT_EQ(0, testHypo->vWts.size())<< "vWts size is zero";

	// pointers
	ASSERT_TRUE(testHypo->pGlass == NULL)<< "pGlass null";

	// now init
	traveltime::CTravelTime* nullTrav = NULL;
	testHypo->initialize(LATITUDE, LONGITUDE, DEPTH, TIME, std::string(ID),
							std::string(WEB), BAYES, THRESH, CUT, nullTrav,
							nullTrav, NULL);

	// check results
	checkdata(testHypo, "initialize check");
}

// test to see if the hypopick operations work
TEST(HypoTest, PickOperations) {
	glassutil::CLogit::disable();

	// construct a hypo
	traveltime::CTravelTime* nullTrav = NULL;
	glasscore::CHypo * testHypo = new glasscore::CHypo(LATITUDE, LONGITUDE,
	DEPTH,
														TIME, std::string(ID),
														std::string(WEB), BAYES,
														THRESH,
														CUT, nullTrav,
														nullTrav, NULL);

	// create json objects from the strings
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));
	json::Object site2JSON = json::Deserialize(std::string(SITE2JSON));
	json::Object site3JSON = json::Deserialize(std::string(SITE3JSON));

	json::Object pickJSON = json::Deserialize(std::string(PICKJSON));
	json::Object pick2JSON = json::Deserialize(std::string(PICK2JSON));
	json::Object pick3JSON = json::Deserialize(std::string(PICK3JSON));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSite(&siteJSON);
	testSiteList->addSite(&site2JSON);
	testSiteList->addSite(&site3JSON);

	// create picks
	glasscore::CPick * testPick = new glasscore::CPick(&pickJSON, 1,
														testSiteList);
	glasscore::CPick * testPick2 = new glasscore::CPick(&pick2JSON, 2,
														testSiteList);
	glasscore::CPick * testPick3 = new glasscore::CPick(&pick3JSON, 3,
														testSiteList);

	// create new shared pointers to the picks
	std::shared_ptr<glasscore::CPick> sharedPick(testPick);
	std::shared_ptr<glasscore::CPick> sharedPick2(testPick2);
	std::shared_ptr<glasscore::CPick> sharedPick3(testPick3);

	// add picks to hypo
	testHypo->addPick(sharedPick);
	testHypo->addPick(sharedPick2);
	testHypo->addPick(sharedPick3);

	// check to make sure the size isn't any larger than our max
	int expectedSize = MAXNPICK;
	ASSERT_EQ(expectedSize, testHypo->vPick.size())<<
	"hypo vPick not larger than max";

	// remove picks from hypo
	testHypo->remPick(sharedPick);
	testHypo->remPick(sharedPick2);

	// check to see that only one pick remains
	expectedSize = 1;
	ASSERT_EQ(expectedSize, testHypo->vPick.size())<< "hypo has only one pick";
}
