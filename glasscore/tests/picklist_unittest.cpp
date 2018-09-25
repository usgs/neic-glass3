#include <gtest/gtest.h>
#include <memory>
#include <string>

#include <logger.h>

#include "Pick.h"
#include "PickList.h"
#include "Site.h"
#include "SiteList.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

#define PICKJSON "{\"ID\":\"20682831\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK2JSON "{\"ID\":\"20682832\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:02:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK3JSON "{\"ID\":\"20682833\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:03:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK4JSON "{\"ID\":\"20682834\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:04:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK5JSON "{\"ID\":\"20682835\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:05:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT
#define PICK6JSON "{\"ID\":\"20682836\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"228041013\",\"Author\":\"228041013\"},\"Time\":\"2014-12-23T00:00:43.599Z\",\"Type\":\"Pick\"}"  // NOLINT

#define SCNL "LRM.EHZ.MB"
#define SCNL2 "BOZ.BHZ.US.00"

#define TPICK 3628281643.59000
#define TPICK2 3628281943.590000
#define TPICK3 3628281763.590000

#define MAXNPICK 5

// NOTE: Need to consider testing scavenge, and rouges functions,
// but that would need a much more involved set of real nodes and data,
// not this simple setup.
// Maybe consider performing this test at a higher level?

// test to see if the picklist can be constructed
TEST(PickListTest, Construction) {
	glass3::util::Logger::disable();

	// construct a picklist
	glasscore::CPickList * testPickList = new glasscore::CPickList();

	// assert default values
	ASSERT_EQ(0, testPickList->getCountOfTotalPicksProcessed())<< "nPickTotal is 0";

	// lists
	ASSERT_EQ(0, testPickList->length())<< "getVPickSize() is 0";

	// pointers
	ASSERT_EQ(NULL, testPickList->getSiteList())<< "pSiteList null";
}

// test various pick operations
TEST(PickListTest, PickOperations) {
	glass3::util::Logger::disable();

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
	std::shared_ptr<json::Object> pick4JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICK4JSON))));
	std::shared_ptr<json::Object> pick5JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICK5JSON))));
	std::shared_ptr<json::Object> pick6JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(PICK6JSON))));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSiteFromJSON(siteJSON);
	testSiteList->addSiteFromJSON(site2JSON);
	testSiteList->addSiteFromJSON(site3JSON);

	// construct a picklist
	glasscore::CPickList * testPickList = new glasscore::CPickList();
	testPickList->setSiteList(testSiteList);

	glasscore::CGlass::setMaxNumPicks(-1);
	testPickList->setMaxAllowablePickCount(MAXNPICK);

	// test adding picks by addPick and dispatch
	testPickList->addPick(pickJSON);
	testPickList->receiveExternalMessage(pick3JSON);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testPickList->getCountOfTotalPicksProcessed())<< "Added Picks";

	// add more picks
	testPickList->addPick(pick2JSON);
	testPickList->addPick(pick4JSON);
	testPickList->addPick(pick5JSON);
	testPickList->addPick(pick6JSON);

	// check to make sure the size isn't any larger than our max
	expectedSize = MAXNPICK;
	ASSERT_EQ(expectedSize, testPickList->length())<<
	"testPickList not larger than max";

	// test clearing picks
	testPickList->clear();
	expectedSize = 0;
	ASSERT_EQ(expectedSize, testPickList->getCountOfTotalPicksProcessed())<< "Cleared Picks";
}
