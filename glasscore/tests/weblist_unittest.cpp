#include "Web.h"
#include "WebList.h"
#include "Site.h"
#include "Logit.h"

#include <gtest/gtest.h>

#define NAME "TestWeb"
#define PHASE "P"
#define THRESH 1.4
#define NUMDETECT 5
#define NUMNUCLEATE 4
#define NUMROWS 3
#define NUMCOLS 4

// NOTE: Need to consider testing the addWeb/remWeb functions but that would
// need a much more involved set of real data, and possibly a glass refactor to
// better support unit testing

// tests to see if the node can be constructed
TEST(WebListTest, Construction) {
	glassutil::CLogit::disable();

	// construct a WebList
	glasscore::CWebList * testWebList = new glasscore::CWebList();

	// lists
	int expectedSize = 0;
	ASSERT_EQ(expectedSize, (int)testWebList->vWeb.size())<< "web list empty";

	// pointers
	ASSERT_EQ(NULL, testWebList->pGlass)<< "pGlass null";
}
