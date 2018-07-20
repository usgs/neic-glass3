#include <stringutil.h>
#include <logger.h>
#include <string>
#include <sstream>
#include <regex>
#include <locale>
#include <codecvt>
#include <vector>

namespace glass3 {
namespace util {

// string functions
// split a string into substrings using the provided cDelimitersiter
std::vector<std::string> split(const std::string &sInput, char cDelimiter) {
	std::vector<std::string> vElements;
	// load the input string into the stream
	std::stringstream stream(sInput);
	std::string sItem;

	// while we have string left in the steam, get the next "line" from the
	// stream, who's end is identified by the delimiter character
	while (std::getline(stream, sItem, cDelimiter)) {
		// no empty strings
		if (!sItem.empty()) {
			// new substring to vector
			vElements.push_back(sItem);
		}
	}

	// return vector
	return (vElements);
}

// remove all instances of the character provided from the string
std::string& removeChars(std::string& sInput, const std::string& sRemoveChars) {  // NOLINT
	// call erase on our in/out string "s", to remove the junk that's left at
	// the end of the string after remove_if() is complete
	sInput.erase(
	// call remove_if() on our
	// string - this scans each character in the string and
	// deletes it if the function() that is our 3rd parameter
	// in remove_if() call, returns true
			remove_if(sInput.begin(), sInput.end(),
			// this is the start of our 3rd
			// parameter: a lambda declaration with a capture
			// clause[&sRemoveChars] - pass a reference to the "sRemoveChars"
			// string object into this lambda function, and  accept a single
			// additional variable (const char reference to the
			// current character in s)
					[&sRemoveChars](const char& aChar) {
						// body of our lambda function - return true if the
						// current char (from "s") is in the list of remove
						// sRemoveChars ("sRemoveChars")
						return (sRemoveChars.find(aChar) != std::string::npos);
					}),
			// final parameter in sInput.erase() call, which erases from the
			// return value of remove_if() which is one spot past the last
					// character not removed, to the end of the string
			sInput.end());
	// return the string
	return (sInput);
}
}  // namespace util
}  // namespace glass3
