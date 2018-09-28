#include <gtest/gtest.h>
#include <memory>
#include <string>

#include <logger.h>

#include "SiteList.h"
#include "Site.h"
#include "Hypo.h"
#include "Correlation.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":6377.377373,\"Latitude\":40.4314,\"Longitude\":-117.221,\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define CORRELATIONJSON "{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT

#define SCNL "BMN.HHZ.LB.01"
#define SITE "BMN"
#define COMP "HHZ"
#define NET "LB"
#define LOC "01"
#define CORRELATIONTIME 3660327144.0170002
#define CORRELATIONIDSTRING "12GFH48776857"
#define LAT 40.3344
#define LON -121.44
#define Z 32.44
#define ORIGINTIME 3660327044.039
#define CORRELATION 2.65
#define PHASE "P"

// check site data for validity
void checkdata(glasscore::CCorrelation * corrleationobject,
				const std::string &testinfo) {
	// check scnl
	std::string sitescnl = corrleationobject->getSite()->getSCNL();
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str());

	// check site
	std::string sitesite = corrleationobject->getSite()->getSite();
	std::string expectedsite = std::string(SITE);
	ASSERT_STREQ(sitesite.c_str(), expectedsite.c_str());

	// check comp
	std::string sitecomp = corrleationobject->getSite()->getComponent();
	std::string expectedcomp = std::string(COMP);
	ASSERT_STREQ(sitecomp.c_str(), expectedcomp.c_str());

	// check net
	std::string sitenet = corrleationobject->getSite()->getNetwork();
	std::string expectednet = std::string(NET);
	ASSERT_STREQ(sitenet.c_str(), expectednet.c_str());

	// check loc
	std::string siteloc = corrleationobject->getSite()->getLocation();
	std::string expectedloc = std::string(LOC);
	ASSERT_STREQ(siteloc.c_str(), expectedloc.c_str());

	// check time
	double correlationtime = corrleationobject->getTCorrelation();
	double expectedcorrelationtime = CORRELATIONTIME;
	ASSERT_NEAR(correlationtime, expectedcorrelationtime, 0.0001);

	// check string id
	std::string stringid = corrleationobject->getID();
	std::string expectedstringid = std::string(CORRELATIONIDSTRING);
	ASSERT_STREQ(stringid.c_str(), expectedstringid.c_str());

	// check origin time
	double origintime = corrleationobject->getTOrigin();
	double expectedorigintime = ORIGINTIME;
	ASSERT_NEAR(origintime, expectedorigintime, 0.0001);

	// check lat
	double lat = corrleationobject->getLatitude();
	double expectedlat = LAT;
	ASSERT_NEAR(lat, expectedlat, 0.0001);

	// check lon
	double lon = corrleationobject->getLongitude();
	double expectedlon = LON;
	ASSERT_NEAR(lon, expectedlon, 0.0001);

	// check rad
	double z = corrleationobject->getDepth();
	double expectedz = Z;
	ASSERT_NEAR(z, expectedz, 0.0001);

	// check correlation
	double correlation = corrleationobject->getCorrelation();
	double expectedcorrelation = CORRELATION;
	ASSERT_NEAR(correlation, expectedcorrelation, 0.0001);

	// check phase
	std::string stringphase = corrleationobject->getPhaseName();
	std::string expectedstringphase = std::string(PHASE);
	ASSERT_STREQ(stringphase.c_str(), expectedstringphase.c_str());
}

// test to see if the correlation can be constructed
TEST(CorrelationTest, Construction) {
	glass3::util::Logger::disable();

	// construct a correlation
	glasscore::CCorrelation * testCorrelation = new glasscore::CCorrelation();

	// assert default values
	ASSERT_EQ(0, testCorrelation->getTCorrelation())<< "time is zero";
	ASSERT_EQ(0, testCorrelation->getTOrigin())<< "origin time is zero";
	ASSERT_EQ(0, testCorrelation->getLatitude())<< "lat is zero";
	ASSERT_EQ(0, testCorrelation->getLongitude())<< "lon is zero";
	ASSERT_EQ(0, testCorrelation->getDepth())<< "z is zero";
	ASSERT_EQ(0, testCorrelation->getCorrelation())<< "correlation is zero";

	ASSERT_STREQ("", testCorrelation->getPhaseName().c_str());
	ASSERT_STREQ("", testCorrelation->getID().c_str());

	// pointers
	ASSERT_TRUE(testCorrelation->getSite() == NULL)<< "pSite null";
	ASSERT_TRUE(testCorrelation->getHypoReference() == NULL)<< "pHypo null";
	ASSERT_TRUE(testCorrelation->getJSONCorrelation() == NULL)<< "jCorrelation null";

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON));

	// now init
	testCorrelation->initialize(sharedTestSite, CORRELATIONTIME,
								std::string(CORRELATIONIDSTRING),
								std::string(PHASE), ORIGINTIME,
								LAT,
								LON, Z, CORRELATION);

	// check results
	checkdata(testCorrelation, "initialize check");
}

// tests to see if the correlation can be constructed from JSON
TEST(CorrelationTest, JSONConstruction) {
	glass3::util::Logger::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// add site to site list
	testSiteList->addSiteFromJSON(siteJSON);

	// construct a correlation using a JSON object
	std::shared_ptr<json::Object> correlationJSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATIONJSON))));
	glasscore::CCorrelation * testCorrelation = new glasscore::CCorrelation(
			correlationJSON, testSiteList);

	// check results
	checkdata(testCorrelation, "json construction check");

	delete (testCorrelation);
}

// tests correlation hypo operations
TEST(CorrelationTest, HypoOperations) {
	glass3::util::Logger::disable();

	// create  shared pointer to the site
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(siteJSON));

	// create correlation
	glasscore::CCorrelation *testCorrelation = new glasscore::CCorrelation(
			sharedTestSite, CORRELATIONTIME, std::string(CORRELATIONIDSTRING),
			std::string(PHASE), ORIGINTIME,
			LAT,
			LON, Z, CORRELATION);

	// create a hypo
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	glasscore::CHypo * testHypo = new glasscore::CHypo(0.0, 0.0, 0.0, 0.0, "1",
														"test", 0.0, 0.0, 0,
														nullTrav, nullTrav,
														nullTTT);

	// create new shared pointer to the hypo
	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	// add hypo to correlation
	testCorrelation->addHypoReference(sharedHypo);

	// check hypo
	ASSERT_TRUE(testCorrelation->getHypoReference() != NULL)<< "pHypo  not null";

	// remove hypo from correlation
	testCorrelation->removeHypoReference(sharedHypo);

	// check hypo
	ASSERT_TRUE(testCorrelation->getHypoReference() == NULL)<< "pHypo null";
}
