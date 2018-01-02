#include <logger.h>
#include <convert.h>
#include <json.h>
#include <gtest/gtest.h>
#include <detection-formats.h>

#include <fstream>
#include <string>

#define HYPOSTRING "{\"Bayes\":2.087726,\"Cmd\":\"Hypo\",\"Data\":[{\"Amplitude\":{\"Amplitude\":0.000000,\"Period\":0.000000,\"SNR\":3.410000},\"AssociationInfo\":{\"Azimuth\":146.725914,\"Distance\":0.114828,\"Phase\":\"P\",\"Residual\":0.000904,\"Sigma\":1.000000},\"Filter\":[{\"HighPass\":1.050000,\"LowPass\":2.650000}],\"ID\":\"100725\",\"Phase\":\"P\",\"Picker\":\"raypicker\",\"Polarity\":\"up\",\"Site\":{\"Channel\":\"BHZ\",\"Location\":\"--\",\"Network\":\"AK\",\"Station\":\"SSN\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T03:35:25.947Z\",\"Type\":\"Pick\"}],\"Depth\":24.717898,\"Gap\":110.554774,\"ID\":\"20311B8E10AF5649BDC52ED099CF173E\",\"IsUpdate\":false,\"Latitude\":61.559315,\"Longitude\":-150.877897,\"MinimumDistance\":0.110850,\"Source\":{\"AgencyID\":\"US\",\"Author\":\"glass\"},\"T\":\"20150814033521.219\",\"Time\":\"2015-08-14T03:35:21.219Z\",\"Type\":\"Hypo\"}" // NOLINT
#define CANCELSTRING "{\"Pid\":\"20311B8E10AF5649BDC52ED099CF173E\",\"Type\":\"Cancel\"}" // NOLINT
#define TESTPATH "testdata"
#define SITELISTFILE "siteList.txt"
#define SITELOOKUPSTRING "{\"Comp\":\"BHZ\",\"Loc\":"",\"Net\":\"AU\",\"Site\":\"WR10\",\"Type\":\"SiteLookup\"}" // NOLINT

#define TESTAGENCYID "US"
#define TESTAUTHOR "glasstest"

TEST(Convert, HypoTest) {
	// logger::log_init("converttest", spdlog::level::debug, ".", true);
	std::string agencyid = std::string(TESTAGENCYID);
	std::string author = std::string(TESTAUTHOR);

	// Hypo
	std::string hypostring = std::string(HYPOSTRING);

	json::Object* hypoobject = new json::Object(json::Deserialize(hypostring));

	std::string detectionoutput = parse::hypoToJSONDetection(hypoobject,
																agencyid,
																author);
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

	// Cancel
	std::string cancelstring = std::string(CANCELSTRING);

	json::Object* cancleobject = new json::Object(
			json::Deserialize(cancelstring));

	std::string retractoutput = parse::cancelToJSONRetract(cancleobject,
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
	std::string sitelistfile = "./" + std::string(TESTPATH) + "/"
			+ std::string(SITELISTFILE);

	// open the file
	std::ifstream inFile;
	inFile.open(sitelistfile, std::ios::in);

	// get the next line
	std::string sitelist;
	std::getline(inFile, sitelist);

	inFile.close();

	json::Object* sitelistobject = new json::Object(
			json::Deserialize(sitelist));
	int numsites = (*sitelistobject)["SiteList"].ToArray().size();

	std::string stationlist = parse::siteListToStationList(sitelistobject);

	json::Object* stationlistobject = new json::Object(
			json::Deserialize(stationlist));

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

	// sitelookup
	std::string sitelookupstring = std::string(SITELOOKUPSTRING);

	json::Object* sitelookupobject = new json::Object(
			json::Deserialize(sitelookupstring));

	std::string stationinforequestoutput =
			parse::siteLookupToStationInfoRequest(sitelookupobject, agencyid,
													author);
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
