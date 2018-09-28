#include <gtest/gtest.h>
#include <memory>
#include <string>

#include <logger.h>

#include "SiteList.h"
#include "Site.h"
#include "Pick.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE4JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":false,\"Quality\":1.0,\"UseForTeleseismic\":false}"  // NOLINT

#define BADSITE "{\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define BADSITE2 "{\"Type\":\"FEH\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define BADSITE3 "{\"Type\":\"StationInfo\",\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"--\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

// test to see if the sitelist can be constructed
TEST(SiteListTest, Construction) {
	glass3::util::Logger::disable();

	printf("[ startup  ]\n");

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	printf("[ construct]\n");

	// assert default values
	ASSERT_EQ(-1, testSiteList->getMaxHoursWithoutPicking());
	ASSERT_EQ(-1, testSiteList->getHoursBeforeLookingUp());
	ASSERT_EQ(-1, testSiteList->getMaxPicksPerHour());

	// lists
	ASSERT_EQ(0, testSiteList->size())<< "vSite is empty";

	printf("[ shutdown ]\n");
}

// test various site operations
TEST(SiteListTest, SiteOperations) {
	glass3::util::Logger::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> site2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE2JSON))));
	std::shared_ptr<json::Object> site3JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE3JSON))));
	std::shared_ptr<json::Object> site4JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE4JSON))));

	// construct sites using JSON objects
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);
	glasscore::CSite * testSite4 = new glasscore::CSite(site4JSON);

	// create new shared pointer to the sites
	std::shared_ptr<glasscore::CSite> sharedTestSite(testSite);
	std::shared_ptr<glasscore::CSite> sharedTestSite4(testSite4);

	// test adding sites to site list via all three methods
	// shared_ptr, json, and dispatch (also json)
	testSiteList->addSite(sharedTestSite);
	testSiteList->addSiteFromJSON(site2JSON);
	testSiteList->receiveExternalMessage(site3JSON);
	int expectedSize = 3;
	ASSERT_EQ(expectedSize, testSiteList->size())<< "Added Sites";

	// test updating a site, and get site
	testSiteList->addSite(sharedTestSite4);
	std::shared_ptr<glasscore::CSite> updatedSite = testSiteList->getSite(
			sharedTestSite4->getSCNL());
	ASSERT_FALSE(updatedSite->getUse())<< "Updated site";

	std::shared_ptr<json::Object> siteListMessage = testSiteList
			->generateSiteListMessage(false);
	ASSERT_TRUE(siteListMessage != NULL)<< "site list message";
}

// test various failure cases
TEST(SiteListTest, FailTests) {
	glass3::util::Logger::disable();

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> badSite = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(BADSITE))));
	std::shared_ptr<json::Object> badSite2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(BADSITE2))));
	std::shared_ptr<json::Object> badSite3 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(BADSITE3))));
	std::shared_ptr<json::Object> nullSiteJSON;
	std::shared_ptr<glasscore::CSite> nullSite;

	// add failures
	ASSERT_FALSE(testSiteList->receiveExternalMessage(nullSiteJSON));
	ASSERT_FALSE(testSiteList->receiveExternalMessage(nullSiteJSON));
	ASSERT_FALSE(testSiteList->addListOfSitesFromJSON(nullSiteJSON));
	ASSERT_FALSE(testSiteList->addListOfSitesFromJSON(siteJSON));
	ASSERT_FALSE(testSiteList->addSiteFromJSON(badSite));
	ASSERT_FALSE(testSiteList->addSiteFromJSON(badSite2));
	ASSERT_FALSE(testSiteList->addSiteFromJSON(badSite3));
	ASSERT_FALSE(testSiteList->addSite(nullSite));

	// get failures
	ASSERT_TRUE(testSiteList->getSite("") == NULL);
	ASSERT_TRUE(testSiteList->getSite("BOZ", "", "", "") == NULL);
	ASSERT_TRUE(testSiteList->getSite("", "", "US", "") == NULL);
}
