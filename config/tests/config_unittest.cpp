#include "config.h"

#include <gtest/gtest.h>

#include <string>

#define CONFIGSTRING "{\"Cmd\":\"ConfigTest\",\"ConfigString\":\"aabbCCdegkdh.3343\", \"ConfigInteger\":4,\"ConfigDouble\":2.112,\"ConfigList\":[\"configitem1\", \"configitem2\"]}"
#define CONFIGFILENAME "configtest.d"
#define CONFIGFILEPATH "./testdata"
#define BADFILENAME "bad.d"
#define BADCONFIGSTRING "bad"

void checkdata(json::Object ConfigObject) {
	// assert config object
	ASSERT_FALSE(ConfigObject.size() == 0)<< "populated config object";

	// check to see if expected keys are present
	ASSERT_TRUE(ConfigObject.HasKey("Cmd"))<< "Cmd Key Present";
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
TEST(ConfigTest, TestFileLoading) {
	// setup
	std::string filename = std::string(CONFIGFILENAME);
	std::string filepath = std::string(CONFIGFILEPATH);

	// test loading on construction
	// create a config object
	util::Config * TestConfig = new util::Config(filepath, filename);

	// check the config data
	checkdata(TestConfig->getJSON());

	// test loading after construction
	// create a config object
	util::Config * TestConfig2 = new util::Config();

	// assert empty config object
	ASSERT_TRUE(TestConfig2->getJSON().size() == 0)<< "empty config object";

	// load config file
	checkdata(TestConfig2->parseJSONFromFile(filepath, filename));

	// cleanup
	delete (TestConfig);
	delete (TestConfig2);
}

// tests to see if the config library string parsing is functional
TEST(ConfigTest, TestStringParsing) {
	// setup
	std::string configstring = std::string(CONFIGSTRING);

	// test parsing on construction
	// create a config object
	util::Config * TestConfig = new util::Config(configstring);

	// check the config data
	checkdata(TestConfig->getJSON());

	// test parsing after construction
	// create a config object
	util::Config * TestConfig2 = new util::Config();

	// assert empty config object
	ASSERT_TRUE(TestConfig2->getJSON().size() == 0)<< "empty config object";

	// parse config string
	checkdata(TestConfig2->parseJSONFromString(configstring));

	// cleanup
	delete (TestConfig);
	delete (TestConfig2);
}

// tests failure cases
TEST(ConfigTest, FailTests) {
	// setup
	std::string badfilename = std::string(BADFILENAME);
	std::string badconfigstring = std::string(BADCONFIGSTRING);
	util::Config * TestConfig = new util::Config();

	// test empty string failure case
	try {
		TestConfig->parseJSONFromString("");
		FAIL();
	} catch (std::invalid_argument& e) {
		ASSERT_STREQ("Empty JSON string", e.what());
	}

	// test bad (non json) string failure case
	try {
		TestConfig->parseJSONFromString(badconfigstring);
		FAIL();
	} catch (std::invalid_argument& e) {
		ASSERT_STREQ("Invalid configuration string", e.what());
	}

	// test empty file name failure case
	try {
		TestConfig->parseJSONFromFile("", "");
		FAIL();
	} catch (std::invalid_argument& e) {
		ASSERT_STREQ("Empty file name", e.what());
	}

	// test bad file name failure case
	try {
		TestConfig->parseJSONFromFile("", badfilename);
		FAIL();
	} catch (std::ios_base::failure& e) {
		ASSERT_STREQ("Failed to open file: unspecified iostream_category error",
						e.what());
	}

	// cleanup
	delete (TestConfig);
}

