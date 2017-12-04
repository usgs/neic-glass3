#include "config.h"

#include <gtest/gtest.h>

#include <string>

#define CONFIGSTRING "{\"Cmd\":\"ConfigTest\",\"ConfigString\":\"aabbCCdegkdh.3343\", \"ConfigInteger\":4,\"ConfigDouble\":2.112,\"ConfigList\":[\"configitem1\", \"configitem2\"]}"
#define CONFIGFILENAME "configtest.d"
#define CONFIGFILEPATH "./testdata"


void checkdata(json::Object ConfigObject)
{
	// check to see if expected keys are present
	ASSERT_TRUE(ConfigObject.HasKey("Cmd")) << "Cmd Key Present";
	ASSERT_TRUE(ConfigObject.HasKey("ConfigString"))
		<< "ConfigString Key Present";
	ASSERT_TRUE(ConfigObject.HasKey("ConfigInteger"))
		<< "ConfigInteger";
	ASSERT_TRUE(ConfigObject.HasKey("ConfigDouble"))
		<< "ConfigDouble Key Present";
	ASSERT_TRUE(ConfigObject.HasKey("ConfigList"))
		<< "ConfigList Key Present";

	// check to see if commented out key is not present
	ASSERT_FALSE(ConfigObject.HasKey("Commented"))
		<< "Commented Key Not Present";
}

// tests to see if the config library file reading is functional
TEST(ConfigTest, TestConfigFile)
{
	// create a config object
	util::Config * TestConfig = new util::Config();

	// assert no config string
	ASSERT_STREQ(TestConfig->getConfig_String().c_str(), "")
		<< "empty config string";

	// assert empty config object
	ASSERT_TRUE(TestConfig->getConfigJSON().size() == 0)
		<< "empty config object";

	// assert no filename string
	ASSERT_STREQ(TestConfig->getFileName().c_str(), "")
		<< "empty filename string";

	// assert no config string
	ASSERT_STREQ(TestConfig->getFilePath().c_str(), "")
		<< "empty filepath string";

	// setup
	std::string filename = std::string(CONFIGFILENAME);
	std::string filepath = std::string(CONFIGFILEPATH);
	bool returnflag = TestConfig->setup(filepath, filename);

	// assert that config was successful
	ASSERT_TRUE(returnflag) << "successful config";

	// assert filename
	ASSERT_STREQ(TestConfig->getFileName().c_str(), filename.c_str())
		<< "filename string";

	// assert filepath
	ASSERT_STREQ(TestConfig->getFilePath().c_str(), filepath.c_str())
		<< "filepath string";

	// load config file
	TestConfig->loadConfigfile();

	// assert config string
	ASSERT_STRNE(TestConfig->getConfig_String().c_str(), "")
		<< "populated config string";

	// assert config object
	ASSERT_FALSE(TestConfig->getConfigJSON().size() == 0)
		<< "populated config object";

	// check the config data
	checkdata(TestConfig->getConfigJSON());

	// check the config string
	std::string configstring = std::string(CONFIGSTRING);
	ASSERT_STREQ(TestConfig->getConfig_String().c_str(), configstring.c_str())
		<< "populated config string";

	// cleanup
	delete(TestConfig);
}

// tests to see if the config library string parsing is functional
TEST(ConfigTest, TestConfigString)
{
	// create a config object
	util::Config * TestConfig = new util::Config();

	// assert no config string
	ASSERT_STREQ(TestConfig->getConfig_String().c_str(), "")
		<< "empty config string";

	// assert empty config object
	ASSERT_TRUE(TestConfig->getConfigJSON().size() == 0)
		<< "empty config object";

	// load config string
	std::string configstring = std::string(CONFIGSTRING);
	TestConfig->loadConfigstring(configstring);

	// assert config string
	ASSERT_STRNE(TestConfig->getConfig_String().c_str(), "")
		<< "populated config string";

	// assert config object
	ASSERT_FALSE(TestConfig->getConfigJSON().size() == 0)
		<< "populated config object";

	// check the config data
	checkdata(TestConfig->getConfigJSON());

	// check the config string
	ASSERT_STREQ(TestConfig->getConfig_String().c_str(), configstring.c_str())
		<< "populated config string";

	// cleanup
	delete(TestConfig);
}
