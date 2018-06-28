#include <gtest/gtest.h>
#include <baseclass.h>
#include <string>

#define TESTCONFIG "{\"Cmd\":\"Test\",\"DiskFile\":\"./testdata/cachetest.txt\"}" // NOLINT

// tests to see if the queue is functional
TEST(BaseClassTest, CombinedTest) {
	// create a queue
	util::BaseClass * TestBaseClass = new util::BaseClass();

	// assert that class not set up
	ASSERT_FALSE(TestBaseClass->getSetup())<< "baseclass not set up";

	// assert no configuration
	ASSERT_TRUE(TestBaseClass->getConfig() == NULL)<< "null config data";

	// generate configuration
	std::string configstring = std::string(TESTCONFIG);
	json::Object configuration = json::Deserialize(configstring);

	// configure baseclass
	ASSERT_TRUE(TestBaseClass->setup(&configuration))<< "setup baseclass";

	// assert that class is set up
	ASSERT_TRUE(TestBaseClass->getSetup())<< "baseclass is set up";

	// assert configuration
	ASSERT_TRUE(TestBaseClass->getConfig() != NULL)<< "non-null config data";

	// check configuration
	std::string baseclassconfig = json::Serialize(*TestBaseClass->getConfig());
	ASSERT_STREQ(configstring.c_str(), baseclassconfig.c_str());

	// clear config
	TestBaseClass->clear();

	// assert that class not set up
	ASSERT_FALSE(TestBaseClass->getSetup())
		<< "baseclass not set up after clear";

	// assert no configuration
	ASSERT_TRUE(TestBaseClass->getConfig() == NULL)
		<< "null config data after clear";

	// cleanup
	delete (TestBaseClass);
}
