#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Correlation.h"
#include "CorrelationList.h"
#include "Site.h"
#include "SiteList.h"
#include "Logit.h"

#define SITEJSON "{\"Type\":\"StationInfo\",\"Elevation\":2326.000000,\"Latitude\":45.822170,\"Longitude\":-112.451000,\"Site\":{\"Station\":\"LRM\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE2JSON "{\"Type\":\"StationInfo\",\"Elevation\":1342.000000,\"Latitude\":46.711330,\"Longitude\":-111.831200,\"Site\":{\"Station\":\"HRY\",\"Channel\":\"EHZ\",\"Network\":\"MB\",\"Location\":\"\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT
#define SITE3JSON "{\"Type\":\"StationInfo\",\"Elevation\":1589.000000,\"Latitude\":45.596970,\"Longitude\":-111.629670,\"Site\":{\"Station\":\"BOZ\",\"Channel\":\"BHZ\",\"Network\":\"US\",\"Location\":\"00\"},\"Enable\":true,\"Quality\":1.0,\"UseForTeleseismic\":true}"  // NOLINT

#define CORRELATIONJSON "{\"ID\":\"20682831\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:01:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:01:55.599Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT
#define CORRELATION2JSON "{\"ID\":\"20682832\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:02:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:02:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT
#define CORRELATION3JSON "{\"ID\":\"20682833\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:03:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:03:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT
#define CORRELATION4JSON "{\"ID\":\"20682834\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"LRM\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:04:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:04:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT
#define CORRELATION5JSON "{\"ID\":\"20682835\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"EHZ\",\"Location\":\"\",\"Network\":\"MB\",\"Station\":\"HRY\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:05:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:05::44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT
#define CORRELATION6JSON "{\"ID\":\"20682836\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"00\",\"Network\":\"US\",\"Station\":\"BOZ\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2014-12-23T00:00:43.599Z\",\"Type\":\"Correlation\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2014-12-23T00:00:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\"}"  // NOLINT

#define SCNL "LRM.EHZ.MB"
#define SCNL2 "BOZ.BHZ.US.00"

#define TCORRELATION 3628281643.590000
#define TCORRELATION2 3628281943.590000
#define TCORRELATION3 3628281763.590000

#define MAXNCORRELATION 5

// NOTE: Need to consider testing scavenge, and rouges functions,
// but that would need a much more involved set of real nodes and data,
// not this simple setup.
// Maybe consider performing this test at a higher level?

// test to see if the correlationlist can be constructed
TEST(CorrelationListTest, Construction) {
	glassutil::CLogit::disable();

	// construct a correlationlist
	glasscore::CCorrelationList * testCorrelationList =
			new glasscore::CCorrelationList();

	// assert default values
	ASSERT_EQ(0, testCorrelationList->getCorrelationTotal())<<
	"nCorrelationTotal is 0";

	// lists
	ASSERT_EQ(0, testCorrelationList->size())<<
	"vCorrelation.size() is 0";

	// pointers
	ASSERT_EQ(NULL, testCorrelationList->getGlass())<< "pGlass null";
	ASSERT_EQ(NULL, testCorrelationList->getSiteList())<< "pSiteList null";

	// cleanup
	delete (testCorrelationList);
}

// test various correlation operations
TEST(CorrelationListTest, CorrelationOperations) {
	glassutil::CLogit::disable();

	// create json objects from the strings
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));
	std::shared_ptr<json::Object> site2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE2JSON))));
	std::shared_ptr<json::Object> site3JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITE3JSON))));

	std::shared_ptr<json::Object> correlationJSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATIONJSON))));
	std::shared_ptr<json::Object> correlation2JSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATION2JSON))));
	std::shared_ptr<json::Object> correlation3JSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATION3JSON))));
	std::shared_ptr<json::Object> correlation4JSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATION4JSON))));
	std::shared_ptr<json::Object> correlation5JSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATION5JSON))));
	std::shared_ptr<json::Object> correlation6JSON = std::make_shared<
			json::Object>(
			json::Object(json::Deserialize(std::string(CORRELATION6JSON))));

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();

	// add sites to site list
	testSiteList->addSite(siteJSON);
	testSiteList->addSite(site2JSON);
	testSiteList->addSite(site3JSON);

	// construct a correlationlist
	glasscore::CCorrelationList * testCorrelationList =
			new glasscore::CCorrelationList();
	testCorrelationList->setSiteList(testSiteList);
	testCorrelationList->setCorrelationMax(MAXNCORRELATION);

	// test adding correlations by addCorrelation and dispatch
	testCorrelationList->addCorrelationFromJSON(correlationJSON);
	testCorrelationList->dispatch(correlation3JSON);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testCorrelationList->getCorrelationTotal())<<
	"Added Correlations";

	// add more correlations
	testCorrelationList->addCorrelationFromJSON(correlation2JSON);
	testCorrelationList->addCorrelationFromJSON(correlation4JSON);
	testCorrelationList->addCorrelationFromJSON(correlation5JSON);
	testCorrelationList->addCorrelationFromJSON(correlation6JSON);

	// check to make sure the size isn't any larger than our max
	expectedSize = MAXNCORRELATION;
	ASSERT_EQ(expectedSize, testCorrelationList->size())<<
	"testCorrelationList not larger than max";

	// test clearing correlations
	testCorrelationList->clear();
	expectedSize = 0;
	ASSERT_EQ(expectedSize, testCorrelationList->getCorrelationTotal())<<
	"Cleared Correlations";

	// cleanup
	delete (testCorrelationList);
	delete (testSiteList);
}
