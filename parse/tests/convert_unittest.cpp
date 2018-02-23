#include <logger.h>
#include <convert.h>
#include <json.h>
#include <gtest/gtest.h>
#include <detection-formats.h>

#include <fstream>
#include <string>
#include <memory>

#define HYPOSTRING "{\"Bayes\":2.087726,\"Cmd\":\"Hypo\",\"Data\":[{\"Type\":\"Correlation\",\"ID\":\"12GFH48776857\",\"Site\":{\"Station\":\"BMN\",\"Network\":\"LB\",\"Channel\":\"HHZ\",\"Location\":\"01\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"TestAuthor\"},\"Phase\":\"P\",\"Time\":\"2015-12-28T21:32:24.017Z\",\"Correlation\":2.65,\"Hypocenter\":{\"Latitude\":40.3344,\"Longitude\":-121.44,\"Depth\":32.44,\"Time\":\"2015-12-28T21:30:44.039Z\"},\"EventType\":\"earthquake\",\"Magnitude\":2.14,\"SNR\":3.8,\"ZScore\":33.67,\"DetectionThreshold\":1.5,\"ThresholdType\":\"minimum\",\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.442559,\"Azimuth\":0.418479,\"Residual\":-0.025393,\"Sigma\":0.086333}},{\"Amplitude\":{\"Amplitude\":0.000000,\"Period\":0.000000,\"SNR\":3.410000},\"AssociationInfo\":{\"Azimuth\":146.725914,\"Distance\":0.114828,\"Phase\":\"P\",\"Residual\":0.000904,\"Sigma\":1.000000},\"Filter\":[{\"HighPass\":1.050000,\"LowPass\":2.650000}],\"ID\":\"100725\",\"Phase\":\"P\",\"Picker\":\"raypicker\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"AK\",\"Station\":\"SSN\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T03:35:25.947Z\",\"Type\":\"Pick\"}],\"Depth\":24.717898,\"Gap\":110.554774,\"ID\":\"20311B8E10AF5649BDC52ED099CF173E\",\"IsUpdate\":false,\"Latitude\":61.559315,\"Longitude\":-150.877897,\"MinimumDistance\":0.110850,\"Source\":{\"AgencyID\":\"US\",\"Author\":\"glass\"},\"T\":\"20150814033521.219\",\"Time\":\"2015-08-14T03:35:21.219Z\",\"Type\":\"Hypo\"}" // NOLINT
#define BADHYPOSTRING1 "{\"Bayes\":2.087726,\"Cmd\":\"Hypo\",\"Data\":[{\"Amplitude\":{\"Amplitude\":0.000000,\"Period\":0.000000,\"SNR\":3.410000},\"AssociationInfo\":{\"Azimuth\":146.725914,\"Distance\":0.114828,\"Phase\":\"P\",\"Residual\":0.000904,\"Sigma\":1.000000},\"Filter\":[{\"HighPass\":1.050000,\"LowPass\":2.650000}],\"ID\":\"100725\",\"Phase\":\"P\",\"Picker\":\"raypicker\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"AK\",\"Station\":\"SSN\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T03:35:25.947Z\",\"Type\":\"Pick\"}],\"Depth\":24.717898,\"Gap\":110.554774,\"ID\":\"20311B8E10AF5649BDC52ED099CF173E\",\"IsUpdate\":false,\"Latitude\":61.559315,\"Longitude\":-150.877897,\"MinimumDistance\":0.110850,\"Source\":{\"AgencyID\":\"US\",\"Author\":\"glass\"},\"T\":\"20150814033521.219\",\"Time\":\"2015-08-14T03:35:21.219Z\"}" // NOLINT
#define BADHYPOSTRING2 "{\"Bayes\":2.087726,\"Cmd\":\"Hypo\",\"Data\":[{\"Amplitude\":{\"Amplitude\":0.000000,\"Period\":0.000000,\"SNR\":3.410000},\"AssociationInfo\":{\"Azimuth\":146.725914,\"Distance\":0.114828,\"Phase\":\"P\",\"Residual\":0.000904,\"Sigma\":1.000000},\"Filter\":[{\"HighPass\":1.050000,\"LowPass\":2.650000}],\"ID\":\"100725\",\"Phase\":\"P\",\"Picker\":\"raypicker\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"AK\",\"Station\":\"SSN\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T03:35:25.947Z\",\"Type\":\"Pick\"}],\"Depth\":24.717898,\"Gap\":110.554774,\"IsUpdate\":false,\"Latitude\":61.559315,\"Longitude\":-150.877897,\"MinimumDistance\":0.110850,\"Source\":{\"AgencyID\":\"US\",\"Author\":\"glass\"},\"T\":\"20150814033521.219\",\"Time\":\"2015-08-14T03:35:21.219Z\",\"Type\":\"Hypo\"}" // NOLINT

#define CANCELSTRING "{\"Pid\":\"20311B8E10AF5649BDC52ED099CF173E\",\"Type\":\"Cancel\"}" // NOLINT
#define BADCANCELSTRING1 "{\"Type\":\"Cancel\"}"
#define BADCANCELSTRING2 "{\"Pid\":\"20311B8E10AF5649BDC52ED099CF173E\"}"

#define TESTPATH "testdata"
#define SITELISTFILE "siteList.txt"
#define BADSITELISTSTRING1 "{\"Type\":\"WhoKnows\"}"
#define BADSITELISTSTRING2 "{\"Cmd\":\"NotRight\"}"

#define SITELOOKUPSTRING "{\"Comp\":\"BHZ\",\"Loc\":"",\"Net\":\"AU\",\"Site\":\"WR10\",\"Type\":\"SiteLookup\"}" // NOLINT
#define BADSITELOOKUPSTRING1 "{\"Comp\":\"BHZ\",\"Loc\":"",\"Net\":\"AU\",\"Site\":\"WR10\"}" // NOLINT

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

TEST(Convert, HypoTest) {
	// logger::log_init("converttest", spdlog::level::debug, ".", true);
	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	// failure cases
	ASSERT_STREQ(parse::hypoToJSONDetection(NULL, agencyid, author).c_str(),
					"");
	ASSERT_STREQ(
			parse::hypoToJSONDetection(std::make_shared<json::Object>(json::Object(json::Deserialize(BADHYPOSTRING1))), agencyid, author).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::hypoToJSONDetection(std::make_shared<json::Object>(json::Object(json::Deserialize(BADHYPOSTRING2))), agencyid, author).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::hypoToJSONDetection(std::make_shared<json::Object>(json::Object(json::Deserialize(CANCELSTRING))), agencyid, author).c_str(),  // NOLINT
			"");

	// Hypo
	std::string detectionoutput = parse::hypoToJSONDetection(
			std::make_shared<json::Object>(
					json::Object(json::Deserialize(HYPOSTRING))),
			agencyid, author);
	// build detection object
	rapidjson::Document detectiondocument;
	detectionformats::detection detectionobject(
			detectionformats::FromJSONString(detectionoutput,
												detectiondocument));
	// check valid code
	ASSERT_TRUE(detectionobject.isvalid())<< "Converted detection is valid";

	// check id
	std::string detectionid = detectionobject.id;
	std::string expectedid = "20311B8E10AF5649BDC52ED099CF173E";
	ASSERT_STREQ(detectionid.c_str(), expectedid.c_str());

	// check agencyid
	std::string sourceagencyid = detectionobject.source.agencyid;
	std::string expectedagencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(sourceagencyid.c_str(), expectedagencyid.c_str());

	// check author
	std::string sourceauthor = detectionobject.source.author;
	std::string expectedauthor = std::string(TESTAUTHOR);
	ASSERT_STREQ(sourceauthor.c_str(), expectedauthor.c_str());

	// check latitude
	double latitude = detectionobject.hypocenter.latitude;
	double expectedlatitude = 61.559315;
	ASSERT_EQ(latitude, expectedlatitude);

	// check longitude
	double longitude = detectionobject.hypocenter.longitude;
	double expectedlongitude = -150.877897;
	ASSERT_EQ(longitude, expectedlongitude);

	// check detectiontime
	double time = detectionobject.hypocenter.time;
	double expectedtime = detectionformats::ConvertISO8601ToEpochTime(
			"2015-08-14T03:35:21.219Z");
	ASSERT_NEAR(time, expectedtime, 0.0001);

	// check depth
	double depth = detectionobject.hypocenter.depth;
	double expecteddepth = 24.717898;
	ASSERT_EQ(depth, expecteddepth);

	// check detectiontype
	std::string detectiondetectiontype = detectionobject.detectiontype;
	std::string expecteddetectiontype = "New";
	ASSERT_STREQ(detectiondetectiontype.c_str(), expecteddetectiontype.c_str());

	// check bayes
	double detectionbayes = detectionobject.bayes;
	double expectedbayes = 2.087726;
	ASSERT_EQ(detectionbayes, expectedbayes);

	// check minimumdistance
	double detectionminimumdistance = detectionobject.minimumdistance;
	double expectedminimumdistancee = 0.11085;
	ASSERT_EQ(detectionminimumdistance, expectedminimumdistancee);

	// check gap
	double detectiongap = detectionobject.gap;
	double expectedgap = 110.554774;
	ASSERT_EQ(detectiongap, expectedgap);
}

TEST(Convert, CancelTest) {
	// logger::log_init("converttest", spdlog::level::debug, ".", true);
	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	// failure cases
	ASSERT_STREQ(parse::cancelToJSONRetract(NULL, agencyid, author).c_str(),
					"");
	ASSERT_STREQ(
			parse::cancelToJSONRetract(std::make_shared<json::Object>(json::Object(json::Deserialize(BADCANCELSTRING1))), agencyid, author).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::cancelToJSONRetract(std::make_shared<json::Object>(json::Object(json::Deserialize(BADCANCELSTRING2))), agencyid, author).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::cancelToJSONRetract(std::make_shared<json::Object>(json::Object(json::Deserialize(HYPOSTRING))), agencyid, author).c_str(),  // NOLINT
			"");

	// Cancel
	std::string retractoutput = parse::cancelToJSONRetract(
			std::make_shared<json::Object>(
					json::Object(json::Deserialize(CANCELSTRING))),
			agencyid, author);
	// build detection object
	rapidjson::Document retractdocument;
	detectionformats::retract retractobject(
			detectionformats::FromJSONString(retractoutput, retractdocument));
	// check valid code
	ASSERT_TRUE(retractobject.isvalid())<< "Converted retraction is valid";

	// check id
	std::string retractid = retractobject.id;
	std::string expectedid = "20311B8E10AF5649BDC52ED099CF173E";
	ASSERT_STREQ(retractid.c_str(), expectedid.c_str());

	// check agencyid
	std::string sourceagencyid = retractobject.source.agencyid;
	std::string expectedagencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(sourceagencyid.c_str(), expectedagencyid.c_str());

	// check author
	std::string sourceauthor = retractobject.source.author;
	std::string expectedauthor = std::string(TESTAUTHOR);
	ASSERT_STREQ(sourceauthor.c_str(), expectedauthor.c_str());
}

TEST(Convert, SiteListTest) {
	// logger::log_init("converttest", spdlog::level::debug, ".", true);

	// failure cases
	ASSERT_STREQ(parse::siteListToStationList(NULL).c_str(), "");
	ASSERT_STREQ(
			parse::siteListToStationList(std::make_shared<json::Object>(json::Object( json::Deserialize(BADSITELISTSTRING1)))).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::siteListToStationList(std::make_shared<json::Object>(json::Object( json::Deserialize(BADSITELISTSTRING2)))).c_str(),  // NOLINT
			"");

	std::string sitelistfile = "./" + std::string(TESTPATH) + "/"
			+ std::string(SITELISTFILE);

	// open the file
	std::ifstream inFile;
	inFile.open(sitelistfile, std::ios::in);

	// get the next line
	std::string sitelist;
	std::getline(inFile, sitelist);

	inFile.close();

	std::shared_ptr<json::Object> sitelistobject =
			std::make_shared<json::Object>(
					json::Object(json::Deserialize(sitelist)));
	int numsites = (*sitelistobject)["SiteList"].ToArray().size();

	std::string stationlist = parse::siteListToStationList(sitelistobject);

	std::shared_ptr<json::Object> stationlistobject = std::make_shared<
			json::Object>(json::Object(json::Deserialize(stationlist)));

	ASSERT_TRUE(stationlistobject->HasKey("Type"));
	ASSERT_STREQ((*stationlistobject)["Type"].ToString().c_str(),
					"StationInfoList");

	ASSERT_TRUE(stationlistobject->HasKey("StationList"));
	ASSERT_EQ((*stationlistobject)["StationList"].ToArray().size(), numsites);
}

TEST(Convert, SiteLookupTest) {
	// logger::log_init("converttest", spdlog::level::debug, ".", true);
	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	// failure cases
	ASSERT_STREQ(
			parse::siteLookupToStationInfoRequest(NULL, agencyid, author).c_str(),
			"");
	ASSERT_STREQ(
			parse::siteLookupToStationInfoRequest(std::make_shared<json::Object>( json::Object(json::Deserialize(BADSITELOOKUPSTRING1))), agencyid, author).c_str(),  // NOLINT
			"");
	ASSERT_STREQ(
			parse::siteLookupToStationInfoRequest(std::make_shared<json::Object>( json::Object( json::Deserialize(CANCELSTRING))), agencyid, author).c_str(),  // NOLINT
			"");

	// sitelookup
	std::string stationinforequestoutput =
			parse::siteLookupToStationInfoRequest(
					std::make_shared<json::Object>(
							json::Object(json::Deserialize(SITELOOKUPSTRING))),
					agencyid, author);
	// build detection object
	rapidjson::Document stationdocument;
	detectionformats::stationInfoRequest stationrequestobject(
			detectionformats::FromJSONString(stationinforequestoutput,
												stationdocument));
	// check valid code
	ASSERT_TRUE(stationrequestobject.isvalid())<< "Converted station info "
	"request is valid";

	// check station
	std::string sitestation = stationrequestobject.site.station;
	std::string expectedstation = "WR10";
	ASSERT_STREQ(sitestation.c_str(), expectedstation.c_str());

	// check channel
	std::string sitechannel = stationrequestobject.site.channel;
	std::string expectedchannel = "BHZ";
	ASSERT_STREQ(sitechannel.c_str(), expectedchannel.c_str());

	// check network
	std::string sitenetwork = stationrequestobject.site.network;
	std::string expectednetwork = "AU";
	ASSERT_STREQ(sitenetwork.c_str(), expectednetwork.c_str());

	// check location
	std::string sitelocation = stationrequestobject.site.location;
	std::string expectedlocation = "";
	ASSERT_STREQ(sitelocation.c_str(), expectedlocation.c_str());

	// check agencyid
	std::string sourceagencyid = stationrequestobject.source.agencyid;
	std::string expectedagencyid = std::string(TESTAGENCYID);
	ASSERT_STREQ(sourceagencyid.c_str(), expectedagencyid.c_str());

	// check author
	std::string sourceauthor = stationrequestobject.source.author;
	std::string expectedauthor = std::string(TESTAUTHOR);
	ASSERT_STREQ(sourceauthor.c_str(), expectedauthor.c_str());
}
