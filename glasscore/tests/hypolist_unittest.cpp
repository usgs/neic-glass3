#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <logger.h>

#include "Site.h"
#include "SiteList.h"
#include "Hypo.h"
#include "HypoList.h"
#include "Pick.h"
#include "Glass.h"


#define TESTPATH "testdata"
#define MERGE1FILENAME "merge1.json"
#define MERGE2FILENAME "merge2.json"
#define NOMERGEFILENAME "nomerge.json"
#define STATIONFILENAME "teststationlist.json"
#define INITFILENAME "initialize.d"

#define TESTHYPOID "3"

#define TSTART 3648585220.50
#define TEND 3648585240.00

#define TORG 3648585200.59000
#define TORG2 3648585220.590000
#define TORG3 3648585209.590000

#define MAXNHYPO 5

#define ASSOCPICKJSON "{\"Type\":\"Pick\",\"ID\":\"96499\",\"Site\":{\"Station\":\"BERG\",\"Network\":\"AK\",\"Channel\":\"BHZ\",\"Location\":\"--\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T00:39:08.527Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Picker\":\"raypicker\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65}],\"Amplitude\":{\"Amplitude\":0.0,\"Period\":0.0,\"SNR\":3.81},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":4.522347499827323,\"Azimuth\":80.51195539508707,\"Residual\":-0.07935762576066452,\"Sigma\":0.9968561359397737}}"  // NOLINT
#define ASSOCPICK2JSON "{\"Type\":\"Pick\",\"ID\":\"96386\",\"Site\":{\"Station\":\"HOM\",\"Network\":\"AK\",\"Channel\":\"BHZ\",\"Location\":\"--\"},\"Source\":{\"AgencyID\":\"US\",\"Author\":\"228041013\"},\"Time\":\"2015-08-14T00:38:18.207Z\",\"Phase\":\"P\",\"Polarity\":\"up\",\"Picker\":\"raypicker\",\"Filter\":[{\"HighPass\":1.05,\"LowPass\":2.65}],\"Amplitude\":{\"Amplitude\":0.0,\"Period\":0.0,\"SNR\":5.0},\"AssociationInfo\":{\"Phase\":\"P\",\"Distance\":0.6123794730349522,\"Azimuth\":118.57193866082869,\"Residual\":-1.1292614242197293,\"Sigma\":0.5285511568133225}}"  // NOLINT

// test to see if the hypolist can be constructed
TEST(HypoListTest, Construction) {
	glass3::util::Logger::disable();

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// assert default values
	ASSERT_EQ(0, testHypoList->getCountOfTotalHyposProcessed())<< "nHypoTotal is 0";

	// lists
	ASSERT_EQ(0, testHypoList->length())<< "vHypo.size() is 0";
	ASSERT_EQ(0, testHypoList->getHypoProcessingQueueLength())<< "qFifo.size() is 0";
}

// test various hypo operations
TEST(HypoListTest, HypoOperations) {
	glass3::util::Logger::disable();

	// create hypo objects
	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;
	std::shared_ptr<glasscore::CHypo> hypo1 =
			std::make_shared<glasscore::CHypo>(-21.84, 170.03, 10.0,
												3648585210.926340, "1", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo2 =
			std::make_shared<glasscore::CHypo>(22.84, 70.03, 12.0,
												3648585208.926340, "2", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo3 =
			std::make_shared<glasscore::CHypo>(41.84, -120.03, 8.0,
												3648585222.926340, "3", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo4 =
			std::make_shared<glasscore::CHypo>(-1.84, 10.03, 100.0,
												3648585250.926340, "4", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo5 =
			std::make_shared<glasscore::CHypo>(1.84, -170.03, 67.0,
												3648585233.926340, "5", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);
	std::shared_ptr<glasscore::CHypo> hypo6 =
			std::make_shared<glasscore::CHypo>(46.84, 135.03, 42.0,
												3648585211.926340, "6", "Test",
												0.0, 0.5, 6, nullTrav, nullTrav,
												nullTTT);

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	testHypoList->setMaxAllowableHypoCount(MAXNHYPO);
	glasscore::CGlass::setMaxNumHypos(-1);

	// test adding hypos by addHypo
	testHypoList->addHypo(hypo1, false);
	testHypoList->addHypo(hypo2, false);
	int expectedSize = 2;
	ASSERT_EQ(expectedSize, testHypoList->getCountOfTotalHyposProcessed())<< "Added Hypos";

	// add more hypos
	testHypoList->addHypo(hypo3, false);
	testHypoList->addHypo(hypo4, false);
	testHypoList->addHypo(hypo5, false);
	testHypoList->addHypo(hypo6, false);

	// check to make sure the size isn't any larger than our max
	expectedSize = MAXNHYPO;
	ASSERT_EQ(expectedSize, (int)testHypoList->length())<<
	"testHypoList not larger than max";

	// test getting a hypo
	std::vector<std::weak_ptr<glasscore::CHypo>> testHypos = testHypoList
			->getHypos(TSTART, TEND);
	ASSERT_TRUE(testHypos.size() != 0)<< "testHypos not empty";

	std::shared_ptr<glasscore::CHypo> testHypo = testHypos[0].lock();

	// check testHypo
	ASSERT_TRUE(testHypo != NULL)<< "testHypo not null";

	// check id
	std::string hypoId = testHypo->getID();
	std::string expectedId = std::string(TESTHYPOID);
	ASSERT_STREQ(hypoId.c_str(), expectedId.c_str())<< "testHypo has right id";

	// test removing hypos by remhypo
	testHypoList->removeHypo(hypo6);

	// check to make sure the size is now one less
	expectedSize = MAXNHYPO - 1;
	ASSERT_EQ(expectedSize, (int)testHypoList->length())<<
	"testHypoList is one smaller";

	// test clearing hypos
	testHypoList->clear();
	expectedSize = 0;
	ASSERT_EQ(expectedSize,
			(int)testHypoList->getCountOfTotalHyposProcessed())<< "Cleared Hypos";
}

// test process
TEST(HypoListTest, ProcessTest) {
	//glass3::util::log_init("processtest", "debug", ".", true);
	glass3::util::Logger::disable();

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// merge
	std::ifstream mergeFile;
	mergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(MERGE1FILENAME),
			std::ios::in);
	std::string mergeLine = "";
	std::getline(mergeFile, mergeLine);
	mergeFile.close();

	// merge2
	std::ifstream merge2File;
	merge2File.open(
			"./" + std::string(TESTPATH) + "/" + std::string(MERGE2FILENAME),
			std::ios::in);
	std::string merge2Line = "";
	std::getline(merge2File, merge2Line);
	merge2File.close();

	// nomerge
	std::ifstream noMergeFile;
	noMergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(NOMERGEFILENAME),
			std::ios::in);
	std::string noMergeLine = "";
	std::getline(noMergeFile, noMergeLine);
	noMergeFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> mergeMessage = std::make_shared<json::Object>(
			json::Deserialize(mergeLine));
	std::shared_ptr<json::Object> merge2Message =
			std::make_shared<json::Object>(json::Deserialize(merge2Line));
	std::shared_ptr<json::Object> noMergeMessage =
			std::make_shared<json::Object>(json::Deserialize(noMergeLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct hypos
	glasscore::CHypo * mergeHypo = new glasscore::CHypo(
			mergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * merge2Hypo = new glasscore::CHypo(
			merge2Message, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * noMergeHypo = new glasscore::CHypo(
			noMergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	// make em shared
	std::shared_ptr<glasscore::CHypo> sharedMerge = std::shared_ptr<
			glasscore::CHypo>(mergeHypo);
	std::shared_ptr<glasscore::CHypo> sharedMerge2 = std::shared_ptr<
			glasscore::CHypo>(merge2Hypo);
	std::shared_ptr<glasscore::CHypo> sharedNoMerge = std::shared_ptr<
			glasscore::CHypo>(noMergeHypo);

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// add hypos
	testHypoList->addHypo(sharedMerge, false);
	testHypoList->addHypo(sharedMerge2, false);
	testHypoList->addHypo(sharedNoMerge, false);
	int expectedSize = 3;
	ASSERT_EQ(expectedSize, testHypoList->length())<< "Added Hypos";

	// process it
	testHypoList->processHypo(sharedMerge);

	// check
	expectedSize = 2;
	ASSERT_EQ(expectedSize, testHypoList->length())<< "processed Hypos";
}

// test process
TEST(HypoListTest, AssociateTest) {
	// glass3::util::log_init("assoctest", "debug", ".", true);
	glass3::util::Logger::disable();

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// merge
	std::ifstream mergeFile;
	mergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(MERGE1FILENAME),
			std::ios::in);
	std::string mergeLine = "";
	std::getline(mergeFile, mergeLine);
	mergeFile.close();

	// merge2
	std::ifstream merge2File;
	merge2File.open(
			"./" + std::string(TESTPATH) + "/" + std::string(MERGE2FILENAME),
			std::ios::in);
	std::string merge2Line = "";
	std::getline(merge2File, merge2Line);
	merge2File.close();

	// nomerge
	std::ifstream noMergeFile;
	noMergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(NOMERGEFILENAME),
			std::ios::in);
	std::string noMergeLine = "";
	std::getline(noMergeFile, noMergeLine);
	noMergeFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> mergeMessage = std::make_shared<json::Object>(
			json::Deserialize(mergeLine));
	std::shared_ptr<json::Object> merge2Message =
			std::make_shared<json::Object>(json::Deserialize(merge2Line));
	std::shared_ptr<json::Object> noMergeMessage =
			std::make_shared<json::Object>(json::Deserialize(noMergeLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(ASSOCPICKJSON))));
	std::shared_ptr<json::Object> pick2JSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(ASSOCPICK2JSON))));

	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct picks
	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, testSiteList);
	glasscore::CPick * testPick2 = new glasscore::CPick(pick2JSON,
														testSiteList);

	// construct hypos
	glasscore::CHypo * mergeHypo = new glasscore::CHypo(
			mergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * merge2Hypo = new glasscore::CHypo(
			merge2Message, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * noMergeHypo = new glasscore::CHypo(
			noMergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	// make em shared
	std::shared_ptr<glasscore::CHypo> sharedMerge = std::shared_ptr<
			glasscore::CHypo>(mergeHypo);
	std::shared_ptr<glasscore::CHypo> sharedMerge2 = std::shared_ptr<
			glasscore::CHypo>(merge2Hypo);
	std::shared_ptr<glasscore::CHypo> sharedNoMerge = std::shared_ptr<
			glasscore::CHypo>(noMergeHypo);
	std::shared_ptr<glasscore::CPick> sharedPick(testPick);
	std::shared_ptr<glasscore::CPick> sharedPick2(testPick2);

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// add hypos
	testHypoList->addHypo(sharedMerge, false);
	testHypoList->addHypo(sharedMerge2, false);
	testHypoList->addHypo(sharedNoMerge, false);

	ASSERT_FALSE(testHypoList->associateData(sharedPick));

	// update statistics
	sharedMerge->calculateStatistics();

	// test assoc 1
	ASSERT_TRUE(testHypoList->associateData(sharedPick));

	// update statistics
	sharedMerge2->calculateStatistics();

	// test assoc multiple
	ASSERT_TRUE(testHypoList->associateData(sharedPick2));
}

// test process
TEST(HypoListTest, WorkTest) {
	//glass3::util::log_init("assoctest", "debug", ".", true);
	glass3::util::Logger::disable();

	// load files
	// stationlist
	std::ifstream stationFile;
	stationFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(STATIONFILENAME),
			std::ios::in);
	std::string stationLine = "";
	std::getline(stationFile, stationLine);
	stationFile.close();

	// merge
	std::ifstream mergeFile;
	mergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(MERGE1FILENAME),
			std::ios::in);
	std::string mergeLine = "";
	std::getline(mergeFile, mergeLine);
	mergeFile.close();

	// nomerge
	std::ifstream noMergeFile;
	noMergeFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(NOMERGEFILENAME),
			std::ios::in);
	std::string noMergeLine = "";
	std::getline(noMergeFile, noMergeLine);
	noMergeFile.close();

	// load config file
	std::ifstream initFile;
	initFile.open(
			"./" + std::string(TESTPATH) + "/" + std::string(INITFILENAME),
			std::ios::in);
	std::string initLine = "";
	std::getline(initFile, initLine);
	initFile.close();

	std::shared_ptr<json::Object> siteList = std::make_shared<json::Object>(
			json::Deserialize(stationLine));
	std::shared_ptr<json::Object> mergeMessage = std::make_shared<json::Object>(
			json::Deserialize(mergeLine));
	std::shared_ptr<json::Object> noMergeMessage =
			std::make_shared<json::Object>(json::Deserialize(noMergeLine));
	std::shared_ptr<json::Object> initConfig = std::make_shared<json::Object>(
			json::Deserialize(initLine));
	std::shared_ptr<json::Object> pickJSON = std::make_shared<json::Object>(
			json::Object(json::Deserialize(std::string(ASSOCPICKJSON))));
	std::shared_ptr<traveltime::CTravelTime> nullTrav;

	// construct a sitelist
	glasscore::CSiteList * testSiteList = new glasscore::CSiteList();
	testSiteList->receiveExternalMessage(siteList);

	// construct a glass
	glasscore::CGlass * testGlass = new glasscore::CGlass();
	testGlass->receiveExternalMessage(initConfig);

	// construct pick
	glasscore::CPick * testPick = new glasscore::CPick(pickJSON, testSiteList);

	// construct hypos
	glasscore::CHypo * mergeHypo = new glasscore::CHypo(
			mergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * noMergeHypo = new glasscore::CHypo(
			noMergeMessage, testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0,
			testSiteList);

	glasscore::CHypo * cancelHypo = new glasscore::CHypo(
			46.84, 135.03, 42.0, 3648585211.926340, "cancel", "Test", 0,
			testGlass->getNucleationStackThreshold(),
			testGlass->getNucleationDataThreshold(),
			testGlass->getDefaultNucleationTravelTime(), nullTrav,
			testGlass->getAssociationTravelTimes(), 100, 360.0, 800.0);

	// make em shared
	std::shared_ptr<glasscore::CHypo> sharedMerge = std::shared_ptr<
			glasscore::CHypo>(mergeHypo);
	std::shared_ptr<glasscore::CHypo> sharedNoMerge = std::shared_ptr<
			glasscore::CHypo>(noMergeHypo);
	std::shared_ptr<glasscore::CHypo> sharedCancel = std::shared_ptr<
			glasscore::CHypo>(cancelHypo);
	std::shared_ptr<glasscore::CPick> sharedPick(testPick);

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// add hypos
	testHypoList->addHypo(sharedMerge, true);
	testHypoList->addHypo(sharedNoMerge, true);
	testHypoList->addHypo(sharedCancel, true);

	// give time for work
	std::this_thread::sleep_for(std::chrono::seconds(2));

	ASSERT_EQ(0, testHypoList->getHypoProcessingQueueLength());
}

// test various failure cases
TEST(HypoListTest, FailTests) {
	glass3::util::Logger::disable();

	// construct a hypolist
	glasscore::CHypoList * testHypoList = new glasscore::CHypoList();

	// nulls
	std::shared_ptr<glasscore::CHypo> nullHypo;
	std::shared_ptr<glasscore::CPick> nullPick;
	std::shared_ptr<glasscore::CCorrelation> nullCorrelation;
	std::shared_ptr<json::Object> nullMessage;

	// empty
	std::shared_ptr<glasscore::CHypo> emptyHypo = std::make_shared<
			glasscore::CHypo>();

	// add failure
	ASSERT_FALSE(testHypoList->addHypo(nullHypo));

	// remove failure
	testHypoList->removeHypo(nullHypo);
	testHypoList->removeHypo(emptyHypo);

	// associate failure
	ASSERT_FALSE(testHypoList->associateData(nullPick));
	ASSERT_FALSE(testHypoList->associateData(nullCorrelation));

	// queue failures
	ASSERT_FALSE(testHypoList->appendToHypoProcessingQueue(nullHypo));
	ASSERT_FALSE(testHypoList->appendToHypoProcessingQueue(emptyHypo));
	ASSERT_TRUE(testHypoList->getNextHypoFromProcessingQueue() == NULL);

	// process failure
	ASSERT_FALSE(testHypoList->processHypo(nullHypo));
	ASSERT_FALSE(testHypoList->processHypo(emptyHypo));

	// merge failure
	ASSERT_FALSE(testHypoList->findAndMergeMatchingHypos(nullHypo));
	ASSERT_FALSE(testHypoList->findAndMergeMatchingHypos(emptyHypo));

	// messaging failures
	ASSERT_FALSE(testHypoList->receiveExternalMessage(nullMessage));
	ASSERT_FALSE(testHypoList->requestHypo(nullMessage));
}
