#include <logger.h>
#include <jsonparser.h>
#include <gtest/gtest.h>

#include <string>
#include <memory>

#define TESTDETECTIONSTRING "{\"Type\":\"Detection\",\"ID\":\"12GFH48776857\",\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:32:24.017Z\"},\"DetectionType\":\"New\",\"EventType\":\"earthquake\",\"Bayes\":2.65,\"MinimumDistance\":2.14,\"RMS\":3.8,\"Gap\":33.67,\"Data\":[{\"Type\":\"Pick\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2015-12-28T21:32:24.017Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Onset\":\"questionable\",\"Picker\":\"manual\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65}],\"Amplitude\":{\"Amplitude\":21.5,\"Period\":2.65,\"SNR\":3.8},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}},{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\",\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}]}" // NOLINT
#define TESTCORRELATIONSTRING "{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\",\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}" // NOLINT
#define TESTPICKSTRING "{\"Type\":\"Pick\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2015-12-28T21:32:24.017Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Onset\":\"questionable\",\"Picker\":\"manual\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65},{\"HighPass\":2.10,\"LowPass\":3.58}],\"Amplitude\":{\"Amplitude\":21.5,\"Period\":2.65,\"SNR\":3.8},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}" // NOLINT
#define TESTSTATIONSTRING "{\"Elevation\":150.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"glasstest\"},\"Latitude\":32.888901,\"Longitude\":-117.105103,\"Quality\":1.000000,\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"TA\",\"Station\":\"109C\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT
#define TESTOTHERSTATIONSTRING "{\"Elevation\":151.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"NC\",\"Author\":\"NC-Glass\"},\"Latitude\":32.888901,\"Longitude\":-117.105103,\"Quality\":1.000000,\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"N4\",\"Station\":\"109E\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT

#define TESTFAILDETECTIONSTRING "{\"Type\":\"Detection\",\"ID\":\"12GFH48776857\",\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"DetectionType\":\"New\",\"EventType\":\"earthquake\",\"Bayes\":2.65,\"MinimumDistance\":2.14,\"RMS\":3.8,\"Gap\":33.67,\"Data\":[{\"Type\":\"Pick\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2015-12-28T21:32:24.017Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Onset\":\"questionable\",\"Picker\":\"manual\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65}],\"Amplitude\":{\"Amplitude\":21.5,\"Period\":2.65,\"SNR\":3.8},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}},{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\",\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}]}" // NOLINT
#define TESTFAILCORRELATIONSTRING "{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\",\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}" // NOLINT
#define TESTFAILPICKSTRING "{\"Type\":\"Pick\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Time\":\"2015-12-28T21:32:24.017Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Onset\":\"questionable\",\"Picker\":\"manual\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65},{\"HighPass\":2.10,\"LowPass\":3.58}],\"Amplitude\":{\"Amplitude\":21.5,\"Period\":2.65,\"SNR\":3.8},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}}" // NOLINT
#define TESTFAILSTATIONSTRING "{\"Elevation\":150.000000,\"Enable\":true,\"InformationRequestor\":{\"AgencyID\":\"US\",\"Author\":\"glasstest\"},\"Quality\":1.000000,\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"TA\",\"Station\":\"109C\"},\"Type\":\"StationInfo\",\"UseForTeleseismic\":true}" // NOLINT

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

class JSONParser : public ::testing::Test {
 protected:
	virtual void SetUp() {
		agencyid = std::string(TESTAGENCYID);
		author = std::string(TESTAUTHOR);

		Parser = new glass3::parse::JSONParser(agencyid, author);

		// glass3::util::log_init("jsonparsertest", spdlog::level::debug, ".", true);
	}

	virtual void TearDown() {
		// cleanup
		delete (Parser);
	}

	std::string agencyid;
	std::string author;
	glass3::parse::JSONParser * Parser;
};

// tests to see jsonparser constructs correctly
TEST_F(JSONParser, Construction) {
	// assert that agencyid is ok
	ASSERT_STREQ(Parser->getDefaultAgencyId().c_str(), agencyid.c_str())<<
	"AgencyID check";

	// assert that author is ok
	ASSERT_STREQ(Parser->getDefaultAuthor().c_str(), author.c_str())
	<< "Author check";
}

// test detections
TEST_F(JSONParser, DetectionParsing) {
	std::string detectionstring = std::string(TESTDETECTIONSTRING);

	// parse the detection
	std::shared_ptr<json::Object> DetectionObject = Parser->parse(
			detectionstring);

	// check the detection
	ASSERT_FALSE(DetectionObject == NULL)<< "Parsed detection not null.";
}

// test correlations
TEST_F(JSONParser, CorrelationParsing) {
	std::string correlationstring = std::string(TESTCORRELATIONSTRING);

	// parse the corrleation
	std::shared_ptr<json::Object> CorrelationObject = Parser->parse(
			correlationstring);

	// check the corrleation
	ASSERT_FALSE(CorrelationObject == NULL)<< "Parsed correlation not null.";
}

// test picks
TEST_F(JSONParser, PickParsing) {
	std::string pickstring = std::string(TESTPICKSTRING);

	// parse the pick
	std::shared_ptr<json::Object> PickObject = Parser->parse(pickstring);

	// check the pick
	ASSERT_FALSE(PickObject == NULL)<< "Parsed pick not null.";
}

// test station
TEST_F(JSONParser, StationParsing) {
	std::string stationstring = std::string(TESTSTATIONSTRING);
	std::string otherstationstring = std::string(TESTOTHERSTATIONSTRING);

	// parse the station
	std::shared_ptr<json::Object> StationObject = Parser->parse(stationstring);

	// check the station
	ASSERT_FALSE(StationObject == NULL)<< "Parsed station not null.";

	// parse the other station
	std::shared_ptr<json::Object> OtherStationObject = Parser->parse(
			otherstationstring);

	// check the other station
	ASSERT_TRUE(OtherStationObject == NULL)<< "Parsed other station null.";
}

// test failure
TEST_F(JSONParser, FailTests) {
	std::string detectionfailstring = std::string(TESTFAILDETECTIONSTRING);
	std::string correlationfailstring = std::string(TESTFAILCORRELATIONSTRING);
	std::string pickfailstring = std::string(TESTFAILPICKSTRING);
	std::string stationfailstring = std::string(TESTFAILSTATIONSTRING);

	// detection failure
	std::shared_ptr<json::Object> FailObject = Parser->parse(
			detectionfailstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed detection fail object null.";

	// correlation failure
	FailObject = Parser->parse(correlationfailstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed correlation fail object null.";

	// pick failure
	FailObject = Parser->parse(pickfailstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed pick fail object null.";

	// station
	FailObject = Parser->parse(stationfailstring);

	// parse the bad data
	ASSERT_TRUE(FailObject == NULL)<< "Parsed station fail object null.";

	// parse empty string
	FailObject = Parser->parse("");

	// parse the empty string
	ASSERT_TRUE(FailObject == NULL)<< "Parsed empty string is null.";

	// parse garbage string
	FailObject = Parser->parse("djaksl;asjfoawov");

	// parse the empty string
	ASSERT_TRUE(FailObject == NULL)<< "Parsed garbage string is null.";
}
