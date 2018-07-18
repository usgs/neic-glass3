#include <gtest/gtest.h>
#include <json.h>
#include <cache.h>
#include <string>
#include <memory>

#define TESTDATA1 "{\"HighPass\":1.000000,\"LowPass\":1.000000,\"cacheid\":\"test1\"}" // NOLINT
#define TESTDATA1ID "test1"
#define TESTDATA2 "{\"HighPass\":2.000000,\"LowPass\":2.000000,\"cacheid\":\"test2\"}" // NOLINT
#define TESTDATA2ID "test2"
#define TESTDATA3 "{\"HighPass\":3.000000,\"LowPass\":3.000000,\"cacheid\":\"test3\"}" // NOLINT
#define TESTDATA3ID "test3"
#define CACHEFILE "./testdata/cachetest.txt"

// tests to see if the cache is functional
TEST(CacheTest, Construction) {
	glass3::util::Cache TestCache;

	// assert an empty cache was created
	ASSERT_TRUE(TestCache.isEmpty())<< "empty cache constructed";
	ASSERT_EQ(TestCache.size(), 0)<< "cache size check";
}

// tests to see if the cache is functional
TEST(CacheTest, CombinedTest) {
	// create cache
	glass3::util::Cache * TestCache = new glass3::util::Cache();

	// create input data
	std::string inputstring1 = std::string(TESTDATA1);
	std::shared_ptr<json::Object> inputdata1 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(inputstring1)));
	std::string inputid1 = std::string(TESTDATA1ID);

	std::string inputstring2 = std::string(TESTDATA2);
	std::shared_ptr<json::Object> inputdata2 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(inputstring2)));
	std::string inputid2 = std::string(TESTDATA2ID);

	std::string inputstring3 = std::string(TESTDATA3);
	std::shared_ptr<json::Object> inputdata3 = std::make_shared<json::Object>(
			json::Object(json::Deserialize(inputstring3)));
	std::string inputid3 = std::string(TESTDATA3ID);

	// add data to cache
	ASSERT_FALSE(TestCache->addToCache(NULL, inputid1))<< "null add test";
	ASSERT_FALSE(TestCache->addToCache(inputdata1, ""))<< "null id add test";
	ASSERT_TRUE(TestCache->addToCache(inputdata1, inputid1))<< "add item 1";
	ASSERT_TRUE(TestCache->addToCache(inputdata2, inputid2))<< "add item 2";
	ASSERT_TRUE(TestCache->addToCache(inputdata3, inputid3))<< "add item 3";

	// assert cache has data
	ASSERT_FALSE(TestCache->isEmpty())<< "cache not empty";
	ASSERT_EQ(TestCache->size(), 3)<< "cache size check";

	// assert the items in the cache
	ASSERT_FALSE(TestCache->isInCache(""))<< "null id in cache";
	ASSERT_FALSE(TestCache->isInCache("5"))<< "bad id in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid1))<< "item 1 in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid2))<< "item 2 in cache";
	ASSERT_TRUE(TestCache->isInCache(inputid3))<< "item 3 in cache";

	// get data from cache
	ASSERT_TRUE(NULL == TestCache->getFromCache(""));
	std::shared_ptr<json::Object> outputobject1 = TestCache->getFromCache(
			inputid1);
	std::shared_ptr<json::Object> outputobject2 = TestCache->getFromCache(
			inputid2);
	std::shared_ptr<json::Object> outputobject3 = TestCache->getFromCache(
			inputid3);

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

	// assert that we got the object we expect
	ASSERT_STREQ(inputstring1.c_str(), outputstring1.c_str())<<
	"cache data 1 matches (getnextfromcache)";
	ASSERT_STREQ(inputstring2.c_str(), outputstring2.c_str())<<
	"cache data 2 matches (getnextfromcache)";
	ASSERT_STREQ(inputstring3.c_str(), outputstring3.c_str())<<
	"cache data 3 matches (getnextfromcache)";

	// remove data from cache
	ASSERT_FALSE(TestCache->removeFromCache(""))<< "no id remove from cache";
	ASSERT_FALSE(TestCache->removeFromCache("5"))<< "bad id remove from cache";
	ASSERT_TRUE(TestCache->removeFromCache(inputid2))<< "remove from cache";

	// check if removed
	ASSERT_FALSE(TestCache->isInCache(inputid2))<< "item 2 is not in cache";

	// check cache size
	ASSERT_EQ(TestCache->size(), 2)<< "cache size check";

	// clear cache
	TestCache->clear();

	// assert cleared cache empty
	ASSERT_TRUE(TestCache->isEmpty())<< "cleared cache is empty";
	ASSERT_EQ(TestCache->size(), 0)<< "cache size check";

	// check if removed
	ASSERT_FALSE(TestCache->isInCache(inputid1))<< "item 1 is not in cache";
	ASSERT_FALSE(TestCache->isInCache(inputid3))<< "item 3 is not in cache";

	// cleanup
	delete (TestCache);
}
