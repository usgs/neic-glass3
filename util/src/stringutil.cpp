#include <stringutil.h>
#include <logger.h>
#include <string>
#include <sstream>
#include <regex>
#include <locale>
#include <codecvt>
#include <vector>

namespace util {

// string functions
// split a string into substrings using the provided delimiter
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}

	return (elems);
}

// remove all instances of the character provided from the string
std::string& removeChars(std::string& s, const std::string& chars) {  // NOLINT
	s.erase(remove_if(s.begin(), s.end(), [&chars](const char& c) {
		return (chars.find(c) != std::string::npos);
	}),
			s.end());

	return (s);
}
}  // namespace util
