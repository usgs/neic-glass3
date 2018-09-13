#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Node.h"
#include "Site.h"
#include "Logit.h"

// test data
#define NAME "testNode"
#define LATITUDE 45.90
#define LONGITUDE -112.79
#define DEPTH 10
#define RESOLUTION 25
#define NODEID "testNode.45.900000.-112.790000.10.000000"

#define SITEJSON "{\"Cmd\":\"Site\",\"Elv\":2326.000000,\"Lat\":45.822170,\"Lon\":-112.451000,\"Site\":\"LRM.EHZ.MB.--\",\"Use\":true}"  // NOLINT
#define TRAVELTIME 122

// NOTE: Need to consider testing nucleate, but that would need a much more
// involved set of real data, and possibly a glass refactor to better support
// unit testing

// tests to see if the node can be constructed
TEST(NodeTest, Construction) {
	glassutil::CLogit::disable();

	// construct a node
	glasscore::CNode * testNode = new glasscore::CNode(std::string(NAME),
	LATITUDE,
														LONGITUDE, DEPTH,
														RESOLUTION
														);

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

	// site list
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, testNode->count())<< "sitelist empty";
}

// tests to see if sites can be added to the node
TEST(NodeTest, SiteOperations) {
	glassutil::CLogit::disable();

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
	testNode->linkSite(sharedTestSite, sharedTestNode, TRAVELTIME);

	// check to see if the site was added
	int expectedSize = 1;
	ASSERT_EQ(expectedSize, testNode->count())<< "node has one site";

	// test clearing the node site links
	testNode->clearSiteLinks();

	// check to see if the site list was cleared
	expectedSize = 0;
	ASSERT_EQ(expectedSize, testNode->count())<< "sitelist cleared";
}
