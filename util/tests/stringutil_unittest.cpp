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
#define NUMBERSTRING "-123.456"
#define CHARACTERSTRING "aAbBcDs"
#define CHARACTERNUMBERSTRING "123Aas456"
#define CHARNUMDASHPERIODSTRING "12.45dDga-"
#define STRINGTOREPLACE "/one/two/three/in"
#define REPLACEDSTRING "\\one\\two\\three\\in"

// tests to see if split is functional
TEST(StringUtil, split) {
	std::string stringtosplit = std::string(STRINGTOSPLIT);
	int expectedcount = SPLITCOUNT;
	std::string expectedone = std::string(ONE);
	std::string expectedtwo = std::string(TWO);
	std::string expectedthree = std::string(THREE);

	std::vector<std::string> splitstring = util::split(stringtosplit, ' ');
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

	std::string removedstring = util::removeChars(stringtoremove, " ");

	// check the string
	ASSERT_STREQ(removedstring.c_str(), expectedremovedstring.c_str());
}

// tests to see if replace_chars is functional
TEST(StringUtil, replace_chars) {
	std::string stringtoreplace = std::string(STRINGTOREPLACE);
	std::string expectedreplacedstring = std::string(REPLACEDSTRING);

	std::string replacedstring = util::replaceChars(stringtoreplace, "/", "\\");

	// check the string
	ASSERT_STREQ(replacedstring.c_str(), expectedreplacedstring.c_str());
}

// tests to see if IsStringNum is functional
TEST(StringUtil, IsStringNum) {
	std::string numstring = std::string(NUMBERSTRING);
	std::string charstring = std::string(CHARACTERSTRING);

	// check true case
	bool isnumber = util::isStringNum(numstring);
	ASSERT_TRUE(isnumber);

	// check false case
	isnumber = util::isStringNum(charstring);
	ASSERT_FALSE(isnumber);
}

// tests to see if IsStringAlpha is functional
TEST(StringUtil, IsStringAlpha) {
	std::string numstring = std::string(NUMBERSTRING);
	std::string charstring = std::string(CHARACTERSTRING);

	// check true case
	bool isnumber = util::isStringAlpha(charstring);
	ASSERT_TRUE(isnumber);

	// check false case
	isnumber = util::isStringAlpha(numstring);
	ASSERT_FALSE(isnumber);
}

// tests to see if IsStringAlphaNum is functional
TEST(StringUtil, IsStringAlphaNum) {
	std::string charnumstring = std::string(CHARACTERNUMBERSTRING);
	std::string charnumdashperstring = std::string(CHARNUMDASHPERIODSTRING);

	// check true case
	bool ischarnum = util::isStringAlphaNum(charnumstring);
	ASSERT_TRUE(ischarnum);

	// check false case
	ischarnum = util::isStringAlphaNum(charnumdashperstring);
	ASSERT_FALSE(ischarnum);
}

// tests to see if IsStringAlpha is functional
TEST(StringUtil, IsStringAlphaNumDashPeriod) {
	std::string charnumstring = std::string(CHARACTERNUMBERSTRING);
	std::string charnumdashperstring = std::string(CHARNUMDASHPERIODSTRING);

	// check true case
	bool ischarnumdashper = util::isStringAlphaNumDashPeriod(
			charnumdashperstring);
	ASSERT_TRUE(ischarnumdashper);

	// check alternate true case
	ischarnumdashper = util::isStringAlphaNumDashPeriod(charnumstring);
	ASSERT_TRUE(ischarnumdashper);
}
