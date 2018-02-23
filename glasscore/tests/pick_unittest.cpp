#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "SiteList.h"
#include "Site.h"
#include "Hypo.h"
#include "Pick.h"
#include "Logit.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define PICKJSON "{\"ID\":\"20682837\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\",\"Beam\":{\"BackAzimuth\":2.65,\"Slowness\":1.44}}"  // NOLINT

#define SCNL "LRM.EHZ.MB"
#define SITE "LRM"
#define COMP "EHZ"
#define NET "MB"
#define LOC ""
#define PICKTIME 3628281703.599
#define BACKAZIMUTH 2.65
#define SLOWNESS 1.44
#define PICKID 1
#define PICKIDSTRING "20682837"

// NOTE: Need to consider testing nucleate function, but that would need a
// much more involved set of real nodes and data, not these dummy nodes.
// Maybe consider performing this test at a higher level?

// check site data for validity
void checkdata(glasscore::CPick * pickobject, const std::string &testinfo) {
	// check scnl
	std::string sitescnl = pickobject->getSite()->getScnl();
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str());

	// check site
	std::string sitesite = pickobject->getSite()->getSite();
	std::string expectedsite = std::string(SITE);
	ASSERT_STREQ(sitesite.c_str(), expectedsite.c_str());

	// check comp
	std::string sitecomp = pickobject->getSite()->getComp();
	std::string expectedcomp = std::string(COMP);
	ASSERT_STREQ(sitecomp.c_str(), expectedcomp.c_str());

	// check net
	std::string sitenet = pickobject->getSite()->getNet();
	std::string expectednet = std::string(NET);
	ASSERT_STREQ(sitenet.c_str(), expectednet.c_str());

	// check loc
	std::string siteloc = pickobject->getSite()->getLoc();
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

	// check id
	int pickid = pickobject->getIdPick();
	double expectedpickid = PICKID;
	ASSERT_EQ(pickid, expectedpickid);

	// check string id
	std::string pickstringid = pickobject->getPid();
	std::string expectedstringid = std::string(PICKIDSTRING);
	ASSERT_STREQ(pickstringid.c_str(), expectedstringid.c_str());
}

// test to see if the pick can be constructed
TEST(PickTest, Construction) {
	glassutil::CLogit::disable();

	// construct a pick
	glasscore::CPick * testPick = new glasscore::CPick();

	// assert default values
	ASSERT_EQ(0, testPick->getTPick())<< "time is zero";
	ASSERT_EQ(-1, testPick->getBackAzimuth())<< "backazimuth is -1";
	ASSERT_EQ(-1, testPick->getSlowness())<< "slowness is -1";
	ASSERT_EQ(0, testPick->getIdPick())<< "id is zero";
	ASSERT_STREQ("", testPick->getAss().c_str());
	ASSERT_STREQ("", testPick->getPhs().c_str());
	ASSERT_STREQ("", testPick->getPid().c_str());

	// pointers
	ASSERT_TRUE(testPick->getSite() == NULL)<< "pSite null";
	ASSERT_TRUE(testPick->getHypo() == NULL)<< "pHypo null";
	ASSERT_TRUE(testPick->getJPick() == NULL)<< "jPick null";

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON, NULL));

	// now init
	testPick->initialize(sharedTestSite, PICKTIME, PICKID,
							std::string(PICKIDSTRING), BACKAZIMUTH, SLOWNESS);

	// check results
	checkdata(testPick, "initialize check");

	delete (testPick);
}

// tests to see if the pick can be constructed from JSON
TEST(PickTest, JSONConstruction) {
	glassutil::CLogit::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// add site to site list
	testSiteList->addSite(siteJSON);

	// construct a pick using a JSON object
	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICKJSON))));

	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, PICKID,
														testSiteList);

	// check results
	checkdata(testPick, "json construction check");
}

// tests pick hypo operations
TEST(PickTest, HypoOperations) {
	glassutil::CLogit::disable();

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
					new glasscore::CSite(siteJSON, NULL));

	// create pick
	glasscore::CPick * testPick = new glasscore::CPick(
			sharedTestSite, PICKTIME,
			PICKID,
			std::string(PICKIDSTRING), BACKAZIMUTH, SLOWNESS);

	// create a hypo
	traveltime::CTravelTime* nullTrav = NULL;
	glasscore::CHypo * testHypo = new glasscore::CHypo(0.0, 0.0, 0.0, 0.0, "1",
														"test", 0.0, 0.0, 0,
														nullTrav, nullTrav,
														NULL);

	// create new shared pointer to the hypo
	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	// add hypo to pick
	testPick->addHypo(sharedHypo);

	// check hypo
	ASSERT_TRUE(testPick->getHypo() != NULL)<< "pHypo  not null";

	// remove hypo from pick
	testPick->remHypo(sharedHypo);

	// check hypo
	ASSERT_TRUE(testPick->getHypo() == NULL)<< "pHypo null";
}
