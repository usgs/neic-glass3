#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "SiteList.h"
#include "Site.h"
#include "Pick.h"
#include "Logit.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE4JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":false,\"Quality\":1.0,\"UseForTeleseismic\":false}"  // NOLINT

// test to see if the sitelist can be constructed
TEST(SiteListTest, Construction) {
	glassutil::CLogit::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// assert default values
	// lists
	ASSERT_EQ(0, testSiteList->getVSiteSize())<< "vSite is empty";

	// pointers
	ASSERT_EQ(NULL, testSiteList->getGlass())<< "pGlass null";

	// cleanup
	delete (testSiteList);
}

// test various site operations
TEST(SiteListTest, SiteOperations) {
	glassutil::CLogit::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));
	json::Object site2JSON = json::Deserialize(std::string(SITE2JSON));
	json::Object site3JSON = json::Deserialize(std::string(SITE3JSON));
	json::Object site4JSON = json::Deserialize(std::string(SITE4JSON));

	// construct sites using JSON objects
	glasscore::CSite * testSite = new glasscore::CSite(&siteJSON, NULL);
	glasscore::CSite * testSite2 = new glasscore::CSite(&site2JSON, NULL);
	glasscore::CSite * testSite3 = new glasscore::CSite(&site3JSON, NULL);
	glasscore::CSite * testSite4 = new glasscore::CSite(&site4JSON, NULL);

	// create new shared pointer to the sites
	std::shared_ptr<glasscore::CSite> sharedTestSite(testSite);
	std::shared_ptr<glasscore::CSite> sharedTestSite2(testSite2);
	std::shared_ptr<glasscore::CSite> sharedTestSite3(testSite3);
	std::shared_ptr<glasscore::CSite> sharedTestSite4(testSite4);

	// test adding sites to site list via all three methods
	// shared_ptr, json, and dispatch (also json)
	testSiteList->addSite(sharedTestSite);
	testSiteList->addSite(&site2JSON);
	testSiteList->dispatch(&site3JSON);
	int expectedSize = 3;
	ASSERT_EQ(expectedSize, testSiteList->getSiteCount())<< "Added Sites";

	// test updating a site, and get site
	testSiteList->addSite(sharedTestSite4);
	std::shared_ptr<glasscore::CSite> updatedSite = testSiteList->getSite(
			sharedTestSite4->getScnl());
	ASSERT_FALSE(updatedSite->getUse())<< "Updated site";

	// cleanup
	delete (testSiteList);
}
