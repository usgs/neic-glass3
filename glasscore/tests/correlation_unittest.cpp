#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "SiteList.h"
#include "Site.h"
#include "Hypo.h"
#include "Correlation.h"
#include "Logit.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":6377.377373,\"Latitude\":40.4314,\"Longitude\":-117.221,\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define CORRELATIONJSON "{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT

#define SCNL "BMN.HHZ.LB.01"
#define SITE "BMN"
#define COMP "HHZ"
#define NET "LB"
#define LOC "01"
#define CORRELATIONTIME 3660327144.0170002
#define CORRELATIONID 1
#define CORRELATIONIDSTRING "12GFH48776857"
#define LAT 40.3344
#define LON -121.44
#define Z 32.44
#define ORIGINTIME 3660327044.039
#define CORRELATION 2.65
#define PHASE "P"

// check site data for validity
void checkdata(glasscore::CCorrelation * corrleationobject,
				std::string testinfo) {
	// check scnl
	std::string sitescnl = corrleationobject->pSite->sScnl;
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str());

	// check site
	std::string sitesite = corrleationobject->pSite->sSite;
	std::string expectedsite = std::string(SITE);
	ASSERT_STREQ(sitesite.c_str(), expectedsite.c_str());

	// check comp
	std::string sitecomp = corrleationobject->pSite->sComp;
	std::string expectedcomp = std::string(COMP);
	ASSERT_STREQ(sitecomp.c_str(), expectedcomp.c_str());

	// check net
	std::string sitenet = corrleationobject->pSite->sNet;
	std::string expectednet = std::string(NET);
	ASSERT_STREQ(sitenet.c_str(), expectednet.c_str());

	// check loc
	std::string siteloc = corrleationobject->pSite->sLoc;
	std::string expectedloc = std::string(LOC);
	ASSERT_STREQ(siteloc.c_str(), expectedloc.c_str());

	// check time
	double correlationtime = corrleationobject->tCorrelation;
	double expectedcorrelationtime = CORRELATIONTIME;
	ASSERT_NEAR(correlationtime, expectedcorrelationtime, 0.0001);

	// check id
	int id = corrleationobject->idCorrelation;
	int expectedid = CORRELATIONID;
	ASSERT_EQ(id, expectedid);

	// check string id
	std::string stringid = corrleationobject->sPid;
	std::string expectedstringid = std::string(CORRELATIONIDSTRING);
	ASSERT_STREQ(stringid.c_str(), expectedstringid.c_str());

	// check origin time
	double origintime = corrleationobject->tOrg;
	double expectedorigintime = ORIGINTIME;
	ASSERT_NEAR(origintime, expectedorigintime, 0.0001);

	// check lat
	double lat = corrleationobject->dLat;
	double expectedlat = LAT;
	ASSERT_NEAR(lat, expectedlat, 0.0001);

	// check lon
	double lon = corrleationobject->dLon;
	double expectedlon = LON;
	ASSERT_NEAR(lon, expectedlon, 0.0001);

	// check rad
	double z = corrleationobject->dZ;
	double expectedz = Z;
	ASSERT_NEAR(z, expectedz, 0.0001);

	// check correlation
	double correlation = corrleationobject->dCorrelation;
	double expectedcorrelation = CORRELATION;
	ASSERT_NEAR(correlation, expectedcorrelation, 0.0001);

	// check phase
	std::string stringphase = corrleationobject->sPhs;
	std::string expectedstringphase = std::string(PHASE);
	ASSERT_STREQ(stringphase.c_str(), expectedstringphase.c_str());
}

// test to see if the correlation can be constructed
TEST(CorrelationTest, Construction) {
	glassutil::CLogit::disable();

	// construct a correlation
	glasscore::CCorrelation * testCorrelation = new glasscore::CCorrelation();

	// assert default values
	ASSERT_EQ(0, testCorrelation->tCorrelation)<< "time is zero";
	ASSERT_EQ(0, testCorrelation->idCorrelation)<< "id is zero";
	ASSERT_EQ(0, testCorrelation->tOrg)<< "origin time is zero";
	ASSERT_EQ(0, testCorrelation->dLat)<< "lat is zero";
	ASSERT_EQ(0, testCorrelation->dLon)<< "lon is zero";
	ASSERT_EQ(0, testCorrelation->dZ)<< "z is zero";
	ASSERT_EQ(0, testCorrelation->dCorrelation)<< "correlation is zero";

	ASSERT_STREQ("", testCorrelation->sAss.c_str());
	ASSERT_STREQ("", testCorrelation->sPhs.c_str());
	ASSERT_STREQ("", testCorrelation->sPid.c_str());

	// pointers
	ASSERT_TRUE(testCorrelation->pSite == NULL)<< "pSite null";
	ASSERT_TRUE(testCorrelation->pHypo == NULL)<< "pHypo null";
	ASSERT_TRUE(testCorrelation->jCorrelation == NULL)<< "jCorrelation null";

	// create  shared pointer to the site
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(&siteJSON, NULL));

	// now init
	testCorrelation->initialize(sharedTestSite, CORRELATIONTIME, CORRELATIONID,
								std::string(CORRELATIONIDSTRING),
								std::string(PHASE), ORIGINTIME,
								LAT,
								LON, Z, CORRELATION);

	// check results
	checkdata(testCorrelation, "initialize check");
}

// tests to see if the correlation can be constructed from JSON
TEST(CorrelationTest, JSONConstruction) {
	glassutil::CLogit::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));

	// add site to site list
	testSiteList->addSite(&siteJSON);

	// construct a correlation using a JSON object
	json::Object correlationJSON = json::Deserialize(
			std::string(CORRELATIONJSON));
	glasscore::CCorrelation * testCorrelation = new glasscore::CCorrelation(
			&correlationJSON, CORRELATIONID, testSiteList);

	// check results
	checkdata(testCorrelation, "json construction check");
}

// tests correlation hypo operations
TEST(CorrelationTest, HypoOperations) {
	glassutil::CLogit::disable();

	// create  shared pointer to the site
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));
	std::shared_ptr<glasscore::CSite> sharedTestSite(
			new glasscore::CSite(&siteJSON, NULL));

	// create correlation
	glasscore::CCorrelation *testCorrelation = new glasscore::CCorrelation(
			sharedTestSite, CORRELATIONTIME, CORRELATIONID,
			std::string(CORRELATIONIDSTRING), std::string(PHASE), ORIGINTIME,
			LAT,
			LON, Z, CORRELATION);

	// create a hypo
	traveltime::CTravelTime* nullTrav = NULL;
	glasscore::CHypo * testHypo = new glasscore::CHypo(0.0, 0.0, 0.0, 0.0, "1",
														"test", 0.0, 0.0, 0,
														nullTrav, nullTrav,
														NULL);

	// create new shared pointer to the hypo
	std::shared_ptr<glasscore::CHypo> sharedHypo(testHypo);

	// add hypo to correlation
	testCorrelation->addHypo(sharedHypo);

	// check hypo
	ASSERT_TRUE(testCorrelation->pHypo != NULL)<< "pHypo  not null";

	// remove hypo from correlation
	testCorrelation->remHypo(sharedHypo);

	// check hypo
	ASSERT_TRUE(testCorrelation->pHypo == NULL)<< "pHypo null";
}
