#include <gtest/gtest.h>

#include <string>
#include <memory>
#include "Node.h"
#include "Web.h"
#include "Site.h"
#include "Logit.h"

#define NAME "TestWeb"
#define THRESH 1.4
#define NUMDETECT 5
#define NUMNUCLEATE 4
#define NUMROWS 3
#define NUMCOLS 4
#define UPDATE true
#define NOUPDATE false

// NOTE: Need to consider testing the grid generation functions, as well
// as addSite and remSite, but that would need a much more
// involved set of real data, and possibly a glass refactor to better support
// unit testing

// tests to see if the node can be constructed
TEST(WebTest, Construction) {
	glassutil::CLogit::disable();

	// default constructor
	glasscore::CWeb aWeb(NOUPDATE, 10, 10);

	// construct a web
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	glasscore::CWeb * testWeb = new glasscore::CWeb(std::string(NAME),
	THRESH,
													NUMDETECT,
													NUMNUCLEATE,
													NUMROWS,
													NUMCOLS,
													UPDATE, nullTrav, nullTrav);

	// name
	ASSERT_STREQ(std::string(NAME).c_str(), testWeb->sName.c_str())<<
	"Web sName Matches";

	// threshold
	ASSERT_EQ(THRESH, testWeb->dThresh)<< "Web dThresh Check";

	// nDetect
	ASSERT_EQ(NUMDETECT, testWeb->nDetect)<< "Web nDetect Check";

	// nNucleate
	ASSERT_EQ(NUMNUCLEATE, testWeb->nNucleate)<< "Web nNucleate Check";

	// nRow
	ASSERT_EQ(NUMROWS, testWeb->nRow)<< "Web nRow Check";

	// nCol
	ASSERT_EQ(NUMCOLS, testWeb->nCol)<< "Web nCol Check";

	// bUpdate
	ASSERT_EQ(UPDATE, testWeb->bUpdate)<< "Web bUpdate Check";

	// lists
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, (int)testWeb->vNode.size())<< "node list empty";
	ASSERT_EQ(expectedSize, (int)testWeb->vSitesFilter.size())<<
	"site filter list empty";
	ASSERT_EQ(expectedSize, (int)testWeb->vNetFilter.size())<<
	"net filter list empty";

	// pointers
	ASSERT_EQ(NULL, testWeb->pGlass)<< "pGlass null";

	delete (testWeb);
}

// tests to see if the node can be initialized
TEST(WebTest, Initialize) {
	glassutil::CLogit::disable();

	// default constructor
	glasscore::CWeb aWeb(UPDATE, 10, 10);
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	aWeb.initialize(std::string(NAME),
	THRESH,
					NUMDETECT,
					NUMNUCLEATE,
					NUMROWS,
					NUMCOLS,
					UPDATE, nullTrav, nullTrav);
}
