#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Site.h"
#include "SiteList.h"
#include "Hypo.h"
#include "HypoList.h"
#include "Pick.h"
#include "Logit.h"

#define TESTHYPOID "3"

#define TSTART 3648585220.50
#define TEND 3648585240.00

#define TORG 3648585200.59000
#define TORG2 3648585220.590000
#define TORG3 3648585209.590000

#define MAXNHYPO 5

// NOTE: Need to consider testing associate, resolve, darwin, and evolve
// functions, but that would need a much more involved set of real data,
// not this simple setup.
// Maybe consider performing this test at a higher level?

// test to see if the hypolist can be constructed
TEST(HypoListTest, Construction) {
	glassutil::CLogit::disable();

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// assert default values
	ASSERT_EQ(-1, testHypoList->nHypoTotal)<< "nPickTotal is 0";
	ASSERT_EQ(0, testHypoList->nHypo)<< "nHypo is 0";

	// lists
	ASSERT_EQ(0, testHypoList->vHypo.size())<< "vHypo.size() is 0";
	ASSERT_EQ(0, testHypoList->mHypo.size())<< "mHypo.size() is 0";
	ASSERT_EQ(0, testHypoList->qFifo.size())<< "qFifo.size() is 0";

	// pointers
	ASSERT_EQ(NULL, testHypoList->pGlass)<< "pGlass null";

	// cleanup
	delete (testHypoList);
}

// test various hypo operations
TEST(HypoListTest, HypoOperations) {
	glassutil::CLogit::disable();

	traveltime::CTTT * nullTTT = NULL;

	// create hypo objects
	traveltime::CTravelTime* nullTrav = NULL;
	std::shared_ptr<glasscore::CHypo> hypo1 =
			std::make_shared<glasscore::CHypo>(-21.84, 170.03, 10.0,
												3648585210.926340, "1", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo2 =
			std::make_shared<glasscore::CHypo>(22.84, 70.03, 12.0,
												3648585208.926340, "2", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo3 =
			std::make_shared<glasscore::CHypo>(41.84, -120.03, 8.0,
												3648585222.926340, "3", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo4 =
			std::make_shared<glasscore::CHypo>(-1.84, 10.03, 100.0,
												3648585250.926340, "4", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo5 =
			std::make_shared<glasscore::CHypo>(1.84, -170.03, 67.0,
												3648585233.926340, "5", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo6 =
			std::make_shared<glasscore::CHypo>(46.84, 135.03, 42.0,
												3648585211.926340, "6", "Test",
												0.0, 0.5, 6, nullTrav,
												nullTrav, nullTTT);

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	testHypoList->nHypoMax = MAXNHYPO;

	// test indexpick when empty
	ASSERT_EQ(-2, testHypoList->indexHypo(0))<< "test indexHypo when empty";

	// test adding hypos by addHypo
	testHypoList->addHypo(hypo1, false);
	testHypoList->addHypo(hypo2, false);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testHypoList->nHypo)<< "Added Hypos";

	// test indexHypo
	ASSERT_EQ(-1, testHypoList->indexHypo(TORG))<<
	"test indexHypo with time before";
	ASSERT_EQ(1, testHypoList->indexHypo(TORG2))<<
	"test indexHypo with time after";
	ASSERT_EQ(0, testHypoList->indexHypo(TORG3))<<
	"test indexHypo with time within";

	// add more hypos
	testHypoList->addHypo(hypo3);
	testHypoList->addHypo(hypo4);
	testHypoList->addHypo(hypo5);
	testHypoList->addHypo(hypo6);

	// check to make sure the size isn't any larger than our max
	expectedSize = MAXNHYPO;
	ASSERT_EQ(expectedSize, (int)testHypoList->vHypo.size())<<
	"testHypoList not larger than max";

	// test getting a hypo
	std::shared_ptr<glasscore::CHypo> testHypo = testHypoList->findHypo(TSTART,
	TEND);

	// check testHypo
	ASSERT_TRUE(testHypo != NULL)<< "testHypo not null";

	// check id
	std::string hypoId = testHypo->getPid();
	std::string expectedId = std::string(TESTHYPOID);
	ASSERT_STREQ(hypoId.c_str(), expectedId.c_str())<< "testHypo has right id";

	// test removing hypos by remhypo
	testHypoList->remHypo(hypo6);

	// check to make sure the size is now one less
	expectedSize = MAXNHYPO - 1;
	ASSERT_EQ(expectedSize, (int)testHypoList->vHypo.size())<<
	"testHypoList is one smaller";

	// test clearing hypos
	testHypoList->clearHypos();
	expectedSize = 0;
	ASSERT_EQ(expectedSize, (int)testHypoList->nHypo)<< "Cleared Hypos";

	// cleanup
	delete (testHypoList);
}
