#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <limits>

#include <logger.h>

#include "SiteList.h"
#include "Site.h"
#include "Hypo.h"
#include "Pick.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define PICKJSON "{\"ID\":\"20682837\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\",\"Beam\":{\"BackAzimuth\":2.65,\"Slowness\":1.44}}"  // NOLINT
#define BADPICK "{\"ID\":\"20682837\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"FEH\",\"Beam\":{\"BackAzimuth\":2.65,\"Slowness\":1.44}}"  // NOLINT
#define BADPICK2 "{\"ID\":\"20682837\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Beam\":{\"BackAzimuth\":2.65,\"Slowness\":1.44}}"  // NOLINT

#define SCNL "LRM.EHZ.MB"
#define SITE "LRM"
#define COMP "EHZ"
#define NET "MB"
#define LOC ""
#define PICKTIME 3628281703.599
#define BACKAZIMUTH 2.65
#define SLOWNESS 1.44
#define PICKIDSTRING "20682837"

// NOTE: Need to consider testing nucleate function, but that would need a
// much more involved set of real nodes and data, not these dummy nodes.
// Maybe consider performing this test at a higher level?

// check site data for validity
void checkdata(glasscore::CPick * pickobject, const std::string &testinfo) {
	// check scnl
	std::string sitescnl = pickobject->getSite()->getSCNL();
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str());

	// check site
	std::string sitesite = pickobject->getSite()->getSite();
	std::string expectedsite = std::string(SITE);
	ASSERT_STREQ(sitesite.c_str(), expectedsite.c_str());

	// check comp
	std::string sitecomp = pickobject->getSite()->getComponent();
	std::string expectedcomp = std::string(COMP);
	ASSERT_STREQ(sitecomp.c_str(), expectedcomp.c_str());

	// check net
	std::string sitenet = pickobject->getSite()->getNetwork();
	std::string expectednet = std::string(NET);
	ASSERT_STREQ(sitenet.c_str(), expectednet.c_str());

	// check loc
	std::string siteloc = pickobject->getSite()->getLocation();
	std::string expectedloc = std::string(LOC);
	ASSERT_STREQ(siteloc.c_str(), expectedloc.c_str());

	// check time
	double picktime = pickobject->getTPick();
	double expectedtime = PICKTIME;
	ASSERT_NEAR(picktime, expectedtime, 0.0001);

	// check backazimuth
	double beambackazimuth = pickobject->getBackAzimuth();
	double expectedbackazimuth = BACKAZIMUTH;
	ASSERT_EQ(beambackazimuth, expectedbackazimuth);

	// check slowness
	double beamslowness = pickobject->getSlowness();
	double expectedslowness = SLOWNESS;
	ASSERT_EQ(beamslowness, expectedslowness);

	// check string id
	std::string pickstringid = pickobject->getID();
	std::string expectedstringid = std::string(PICKIDSTRING);
	ASSERT_STREQ(pickstringid.c_str(), expectedstringid.c_str());
}

// test to see if the pick can be constructed
TEST(PickTest, Construction) {
	glass3::util::Logger::disable();

	// construct a pick
	glasscore::CPick * testPick = new glasscore::CPick();

	// assert default values
	ASSERT_EQ(0, testPick->getTPick())<< "time is zero";

	/* Google test can't seem to handle nans
	 *
	 ASSERT_(std::numeric_limits<double>::quiet_NaN(),
	 testPick->getBackAzimuth())<< "backazimuth is -1";
	 ASSERT_EQ(std::numeric_limits<double>::quiet_NaN(),
	 testPick->getSlowness())<< "slowness is -1";
	 */

	ASSERT_STREQ("", testPick->getPhaseName().c_str());
	ASSERT_STREQ("", testPick->getID().c_str());

	// pointers
	ASSERT_TRUE(testPick->getSite() == NULL)<< "pSite null";
	ASSERT_TRUE(testPick->getHypoReference() == NULL)<< "pHypo null";
	ASSERT_TRUE(testPick->getJSONPick() == NULL)<< "jPick null";

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON));

	// now init
	testPick->initialize(sharedTestSite, PICKTIME, std::string(PICKIDSTRING),
	BACKAZIMUTH,
							SLOWNESS);

	// check results
	checkdata(testPick, "initialize check");

	delete (testPick);
}

// tests to see if the pick can be constructed from JSON
TEST(PickTest, JSONConstruction) {
	glass3::util::Logger::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// add site to site list
	testSiteList->addSiteFromJSON(siteJSON);

	// construct a pick using a JSON object
	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICKJSON))));

	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, testSiteList);

	// check results
	checkdata(testPick, "json construction check");
}

// tests pick hypo operations
TEST(PickTest, HypoOperations) {
	glass3::util::Logger::disable();

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON));

	// create pick
	glasscore::CPick * testPick = new glasscore::CPick(
			sharedTestSite, PICKTIME, std::string(PICKIDSTRING), BACKAZIMUTH,
			SLOWNESS);

	// create a hypo
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	glasscore::CHypo * testHypo = new glasscore::CHypo(0.0, 0.0, 0.0, 0.0, "1",
														"test", 0.0, 0.0, 0,
														nullTrav, nullTrav,
														nullTTT);

	// create new shared pointer to the hypo
	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	// add hypo to pick
	testPick->addHypoReference(sharedHypo);

	// check hypo
	ASSERT_TRUE(testPick->getHypoReference() != NULL)<< "pHypo  not null";

	// remove hypo from pick
	testPick->removeHypoReference(sharedHypo);

	// check hypo
	ASSERT_TRUE(testPick->getHypoReference() == NULL)<< "pHypo null";
}

// test various failure cases
TEST(PickTest, FailTests) {
	glass3::util::Logger::disable();

	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICKJSON))));
	std::shared_ptr<json::Object> badPick = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(BADPICK))));
	std::shared_ptr<json::Object> badPick2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(BADPICK2))));
	std::shared_ptr<json::Object> nullMessage;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// add site to site list
	testSiteList->addSiteFromJSON(siteJSON);

	// create failures
	glasscore::CPick testPick(pickJSON, NULL);
	ASSERT_STREQ("", testPick.getID().c_str());

	glasscore::CPick aNullPick(nullMessage, testSiteList);
	ASSERT_STREQ("", aNullPick.getID().c_str());

	glasscore::CPick aBadPick(badPick, NULL);
	ASSERT_STREQ("", aBadPick.getID().c_str());

	glasscore::CPick aBadPick2(badPick2, NULL);
	ASSERT_STREQ("", aBadPick2.getID().c_str());
}
