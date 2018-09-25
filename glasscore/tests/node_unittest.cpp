#include <gtest/gtest.h>
#include <memory>
#include <string>

#include <logger.h>

#include "Node.h"
#include "Site.h"

// test data
#define NAME "testNode"
#define LATITUDE 45.90
#define LONGITUDE -112.79
#define DEPTH 10
#define RESOLUTION 25
#define NODEID "testNode.45.900000.-112.790000.10.000000"

#define SITEJSON "{\"Cmd\":\"Site\",\"Elv\":2326.000000,\"Lat\":45.822170,\"Lon\":-112.451000,\"Site\":\"LRM.EHZ.MB.--\",\"Use\":true}"  // NOLINT
#define TRAVELTIME 122
#define DISTANCE_FOR_TT 8.5

// NOTE: Need to consider testing nucleate, but that would need a much more
// involved set of real data, and possibly a glass refactor to better support
// unit testing

// tests to see if the node can be constructed
TEST(NodeTest, Construction) {
	glass3::util::Logger::disable();

	// construct a node
	glasscore::CNode * testNode = new glasscore::CNode(std::string(NAME),
	LATITUDE,
														LONGITUDE, DEPTH,
														RESOLUTION);

	// name
	ASSERT_STREQ(std::string(NAME).c_str(), testNode->getName().c_str())<<
	"Node Name Matches";

	// latitude
	ASSERT_EQ(LATITUDE, testNode->getLatitude())<< "Node Latitude Check";

	// longitude
	ASSERT_EQ(LONGITUDE, testNode->getLongitude())<< "Node Longitude Check";

	// depth
	ASSERT_EQ(DEPTH, testNode->getDepth())<< "Node Depth Check";

	// resolution
	ASSERT_EQ(RESOLUTION, testNode->getResolution())<< "Node Resolution Check";

	// id
	ASSERT_STREQ(std::string(NODEID).c_str(), testNode->getID().c_str())<<
	"Node ID Matches";

	// web name
	ASSERT_TRUE(testNode->getWeb() == NULL)<< "Web null";

	// enabled
	ASSERT_TRUE(testNode->getEnabled())<< "enabled";

	// site list
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, testNode->getSiteLinksCount())<< "sitelist empty";

	// geo
	ASSERT_EQ(LONGITUDE, testNode->getGeo().m_dGeocentricLongitude)<< "geo";
}

// tests to see if sites can be added to the node
TEST(NodeTest, SiteOperations) {
	glass3::util::Logger::disable();

	// construct a node
	glasscore::CNode * testNode = new glasscore::CNode(std::string(NAME),
	LATITUDE,
														LONGITUDE, DEPTH,
														RESOLUTION);

	// create json object from the string
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// construct sites using JSON objects
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);

	// create new shared pointer to the node
	std::shared_ptr<glasscore::CNode> sharedTestNode(testNode);

	// create new shared pointer to the site
	std::shared_ptr<glasscore::CSite> sharedTestSite(testSite);

	// add the site to the node
	ASSERT_TRUE(
			testNode->linkSite(sharedTestSite, sharedTestNode, DISTANCE_FOR_TT, TRAVELTIME)); // NOLINT

	// check to see if the site was added
	int expectedSize = 1;
	ASSERT_EQ(expectedSize, testNode->getSiteLinksCount())<< "node has one site";

	// test clearing the node site links
	testNode->clearSiteLinks();

	// check to see if the site list was cleared
	expectedSize = 0;
	ASSERT_EQ(expectedSize, testNode->getSiteLinksCount())<< "sitelist cleared";
}

// test various failure cases
TEST(NodeTest, FailTests) {
	glass3::util::Logger::disable();

	// null pointers
	std::shared_ptr<glasscore::CNode> nullNode;
	std::shared_ptr<glasscore::CSite> nullSite;

	// construct a node
	glasscore::CNode * testNode = new glasscore::CNode(std::string(NAME),
	LATITUDE,
														LONGITUDE, DEPTH,
														RESOLUTION);

	// create json object from the string
	std::shared_ptr<json::Object> siteJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(SITEJSON))));

	// construct sites using JSON objects
	glasscore::CSite * testSite = new glasscore::CSite(siteJSON);

	// make shared
	std::shared_ptr<glasscore::CNode> sharedTestNode(testNode);
	std::shared_ptr<glasscore::CSite> sharedTestSite(testSite);

	// link fails
	ASSERT_FALSE(
			testNode->linkSite(nullSite, sharedTestNode, DISTANCE_FOR_TT, TRAVELTIME));  // NOLINT
	ASSERT_FALSE(
			testNode->linkSite(sharedTestSite, nullNode, DISTANCE_FOR_TT, TRAVELTIME));  // NOLINT

	// unlink fails
	ASSERT_FALSE(testNode->unlinkSite(nullSite));

	// nucleate
	ASSERT_TRUE(testNode->nucleate(-1) == NULL);
}
