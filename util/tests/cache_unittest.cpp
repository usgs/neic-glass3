#include <gtest/gtest.h>
#include <json.h>
#include <cache.h>
#include <string>

#define TESTDATA1 "{\"HighPass\":1.000000,\"LowPass\":1.000000,\"cacheid\":\"test1\"}" // NOLINT
#define TESTDATA1ID "test1"
#define TESTDATA2 "{\"HighPass\":2.000000,\"LowPass\":2.000000,\"cacheid\":\"test2\"}" // NOLINT
#define TESTDATA2ID "test2"
#define TESTDATA3 "{\"HighPass\":3.000000,\"LowPass\":3.000000,\"cacheid\":\"test3\"}" // NOLINT
#define TESTDATA3ID "test3"
#define CACHECONFIG "{\"Cmd\":\"Cache\",\"DiskFile\":\"./testdata/cachetest.txt\"}" // NOLINT
#define CACHEFILE "./testdata/cachetest.txt"

// tests to see if the queue is functional
TEST(CacheTest, CombinedTest) {
	// create a queue
	util::Cache * TestCache = new util::Cache();

	// assert an empty cache was created
	ASSERT_TRUE(TestCache->isEmpty())<< "empty cache constructed";
	ASSERT_EQ(TestCache->size(), 0)<< "cache size check";

	// assert that we're not setup
	ASSERT_FALSE(TestCache->m_bIsSetup)<< "cache not setup";

	// configure cache
	// generate configuration
	std::string configstring = std::string(CACHECONFIG);
	json::Object configuration = json::Deserialize(configstring);

	// setup cache
	bool returnflag = TestCache->setup(&configuration);

	// assert config was successful
	ASSERT_TRUE(returnflag)<< "cache config successful";

	// assert that we're setup
	ASSERT_TRUE(TestCache->m_bIsSetup)<< "cache set up";

	// confirm config data successful
	std::string cachefilename = std::string(CACHEFILE);
	std::string diskfile = TestCache->getSDiskCacheFile();
	ASSERT_STREQ(cachefilename.c_str(), diskfile.c_str())<<
			"cache diskfile config";

	// create input data
	std::string inputstring1 = std::string(TESTDATA1);
	json::Object* inputdata1 = new json::Object(
			json::Deserialize(inputstring1));
	std::string inputid1 = std::string(TESTDATA1ID);

	std::string inputstring2 = std::string(TESTDATA2);
	json::Object* inputdata2 = new json::Object(
			json::Deserialize(inputstring2));
	std::string inputid2 = std::string(TESTDATA2ID);

	std::string inputstring3 = std::string(TESTDATA3);
	json::Object* inputdata3 = new json::Object(
			json::Deserialize(inputstring3));
	std::string inputid3 = std::string(TESTDATA3ID);

	// add data to cache
	TestCache->addToCache(inputdata1, inputid1);
	TestCache->addToCache(inputdata2, inputid2);
	TestCache->addToCache(inputdata3, inputid3);

	// assert cache hase data
	ASSERT_FALSE(TestCache->isEmpty())<< "cache not empty";
	ASSERT_EQ(TestCache->size(), 3)<< "cache size check";

	// assert the items in the cache
	ASSERT_TRUE(TestCache->isInCache(inputid1))<< "item 1 in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid2))<< "item 2 in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid3))<< "item 3 in cache";

	// get data from cache
	json::Object* outputobject1 = TestCache->getFromCache(inputid1);
	json::Object* outputobject2 = TestCache->getFromCache(inputid2);
	json::Object* outputobject3 = TestCache->getFromCache(inputid3);

	// assert that we got something
	ASSERT_TRUE(outputobject1 != NULL)<< "non-null cache data 1 (getfromcache)";
	ASSERT_TRUE(outputobject2 != NULL)<< "non-null cache data 2 (getfromcache)";
	ASSERT_TRUE(outputobject3 != NULL)<< "non-null cache data 3 (getfromcache)";

	std::string outputstring1 = json::Serialize(*outputobject1);
	std::string outputstring2 = json::Serialize(*outputobject2);
	std::string outputstring3 = json::Serialize(*outputobject3);

	// assert that we got the objects we expect
	ASSERT_STREQ(inputstring1.c_str(), outputstring1.c_str())<<
			"cache data 1 matches (getfromcache)";
	ASSERT_STREQ(inputstring2.c_str(), outputstring2.c_str())<<
			"cache data 2 matches (getfromcache)";
	ASSERT_STREQ(inputstring3.c_str(), outputstring3.c_str())<<
			"cache data 3 matches (getfromcache)";

	// get data from cache
	outputobject1 = TestCache->getNextFromCache(true);
	outputobject2 = TestCache->getNextFromCache();
	outputobject3 = TestCache->getNextFromCache();

	// assert that we got something
	ASSERT_TRUE(outputobject1 != NULL)<<
			"non-null cache data 1 (getnextfromcache)";
	ASSERT_TRUE(outputobject2 != NULL)<<
			"non-null cache data 2 (getnextfromcache)";
	ASSERT_TRUE(outputobject3 != NULL)<<
			"non-null cache data 3 (getnextfromcache)";

	outputstring1 = json::Serialize(*outputobject1);
	outputstring2 = json::Serialize(*outputobject2);
	outputstring3 = json::Serialize(*outputobject3);

	// assert that we got the objectw we expect
	ASSERT_STREQ(inputstring1.c_str(), outputstring1.c_str())<<
			"cache data 1 matches (getnextfromcache)";
	ASSERT_STREQ(inputstring2.c_str(), outputstring2.c_str())<<
			"cache data 2 matches (getnextfromcache)";
	ASSERT_STREQ(inputstring3.c_str(), outputstring3.c_str())<<
			"cache data 3 matches (getnextfromcache)";

	// remove data from cache
	ASSERT_TRUE(TestCache->removeFromCache(inputid2))<< "remove from cache";

	// check if removed
	ASSERT_FALSE(TestCache->isInCache(inputid2))<< "item 2 is not in cache";

	// check cache size
	ASSERT_EQ(TestCache->size(), 2)<< "cache size check";

	// check if cache modified
	ASSERT_TRUE(TestCache->getBCacheModified())<< "cache modified";

	// write cache to disk
	ASSERT_TRUE(TestCache->writeCacheToDisk())<< "write cache to disk";

	// check if cache still modified after write
	ASSERT_FALSE(TestCache->getBCacheModified())<< "cache is not modified";

	// clear cache
	TestCache->clearCache();

	// assert cleared cache empty
	ASSERT_TRUE(TestCache->isEmpty())<< "cleared cache is empty";
	ASSERT_EQ(TestCache->size(), 0)<< "cache size check";

	// check if removed
	ASSERT_FALSE(TestCache->isInCache(inputid1))<< "item 1 is not in cache";
	ASSERT_FALSE(TestCache->isInCache(inputid3))<< "item 3 is not in cache";

	// load cache from disk
	ASSERT_TRUE(TestCache->loadCacheFromDisk())<< "load cache from disk";

	// assert loaded cache not empty
	ASSERT_FALSE(TestCache->isEmpty())<< "loaded cache is notempty";

	// check if present
	ASSERT_TRUE(TestCache->isInCache(inputid1))<< "item 1 is in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid3))<< "item 3 is in cache";

	// cleanup
	delete (TestCache);

	// cleanup cache file
	std::remove(cachefilename.c_str());
}
