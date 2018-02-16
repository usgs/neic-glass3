#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Pick.h"
#include "PickList.h"
#include "Site.h"
#include "SiteList.h"
#include "Logit.h"

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
	glassutil::CLogit::disable();

	// construct a picklist
	glasscore::CPickList * testPickList = new glasscore::CPickList();

	// assert default values
	ASSERT_EQ(-1, testPickList->getNPickTotal())<< "nPickTotal is 0";
	ASSERT_EQ(0, testPickList->getNPick())<< "nPick is 0";

	// lists
	ASSERT_EQ(0, testPickList->getVPickSize())<< "getVPickSize() is 0";

	// pointers
	ASSERT_EQ(NULL, testPickList->getGlass())<< "pGlass null";
	ASSERT_EQ(NULL, testPickList->getSiteList())<< "pSiteList null";

	// cleanup
	delete (testPickList);
}

// test various pick operations
TEST(PickListTest, PickOperations) {
	glassutil::CLogit::disable();

	// create json objects from the strings
	json::Object siteJSON = json::Deserialize(std::string(SITEJSON));
	json::Object site2JSON = json::Deserialize(std::string(SITE2JSON));
	json::Object site3JSON = json::Deserialize(std::string(SITE3JSON));

	json::Object pickJSON = json::Deserialize(std::string(PICKJSON));
	json::Object pick2JSON = json::Deserialize(std::string(PICK2JSON));
	json::Object pick3JSON = json::Deserialize(std::string(PICK3JSON));
	json::Object pick4JSON = json::Deserialize(std::string(PICK4JSON));
	json::Object pick5JSON = json::Deserialize(std::string(PICK5JSON));
	json::Object pick6JSON = json::Deserialize(std::string(PICK6JSON));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSite(&siteJSON);
	testSiteList->addSite(&site2JSON);
	testSiteList->addSite(&site3JSON);

	// construct a picklist
	glasscore::CPickList * testPickList = new glasscore::CPickList();
	testPickList->setSiteList(testSiteList);
	testPickList->setNPickMax(MAXNPICK);

	// test indexpick when empty
	ASSERT_EQ(-2, testPickList->indexPick(0))<< "test indexpick when empty";

	// test adding picks by addPick and dispatch
	testPickList->addPick(&pickJSON);
	testPickList->dispatch(&pick3JSON);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testPickList->getNPick())<< "Added Picks";

	// test getting a pick (first pick, id 1)
	std::shared_ptr<glasscore::CPick> testPick = testPickList->getPick(1);
	// check testpick
	ASSERT_TRUE(testPick != NULL)<< "testPick not null";
	// check scnl
	std::string sitescnl = testPick->getSite()->getScnl();
	std::string expectedscnl = std::string(SCNL);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str())<<
			"testPick has right scnl";

	// test indexpick
	ASSERT_EQ(-1, testPickList->indexPick(TPICK))<<
			"test indexpick with time before";
	ASSERT_EQ(1, testPickList->indexPick(TPICK2))<<
			"test indexpick with time after";
	ASSERT_EQ(0, testPickList->indexPick(TPICK3))<<
			"test indexpick with time within";

	// add more picks
	testPickList->addPick(&pick2JSON);
	testPickList->addPick(&pick4JSON);
	testPickList->addPick(&pick5JSON);
	testPickList->addPick(&pick6JSON);

	// check to make sure the size isn't any larger than our max
	expectedSize = MAXNPICK;
	ASSERT_EQ(expectedSize, testPickList->getVPickSize())<<
			"testPickList not larger than max";

	// get first pick, which is now id 2
	std::shared_ptr<glasscore::CPick> test2Pick = testPickList->getPick(2);

	// check scnl
	sitescnl = test2Pick->getSite()->getScnl();
	expectedscnl = std::string(SCNL2);
	ASSERT_STREQ(sitescnl.c_str(), expectedscnl.c_str())<<
			"test2Pick has right scnl";

	// test clearing picks
	testPickList->clearPicks();
	expectedSize = 0;
	ASSERT_EQ(expectedSize, testPickList->getNPick())<< "Cleared Picks";

	// cleanup
	delete (testPickList);
	delete (testSiteList);
}
