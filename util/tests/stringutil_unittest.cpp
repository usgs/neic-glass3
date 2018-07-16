#include <gtest/gtest.h>
#include <stringutil.h>
#include <string>
#include <vector>

#define STRINGTOSPLIT "one two three"
#define ONE "one"
#define TWO "two"
#define THREE "three"
#define SPLITCOUNT 3
#define REMOVEDSTRING "onetwothree"

// tests to see if split is functional
TEST(StringUtil, split) {
	std::string stringtosplit = std::string(STRINGTOSPLIT);
	int expectedcount = SPLITCOUNT;
	std::string expectedone = std::string(ONE);
	std::string expectedtwo = std::string(TWO);
	std::string expectedthree = std::string(THREE);

	std::vector<std::string> splitstring = glass3::util::split(stringtosplit,
																' ');
	int count = splitstring.size();

	// make sure we split into the right number
	ASSERT_EQ(count, expectedcount);

	// check the elements
	ASSERT_STREQ(splitstring[0].c_str(), expectedone.c_str());
	ASSERT_STREQ(splitstring[1].c_str(), expectedtwo.c_str());
	ASSERT_STREQ(splitstring[2].c_str(), expectedthree.c_str());
}

// tests to see if remove_chars is functional
TEST(StringUtil, remove_chars) {
	std::string stringtoremove = std::string(STRINGTOSPLIT);
	std::string expectedremovedstring = std::string(REMOVEDSTRING);

	std::string removedstring = glass3::util::removeChars(stringtoremove, " ");

	// check the string
	ASSERT_STREQ(removedstring.c_str(), expectedremovedstring.c_str());
}
