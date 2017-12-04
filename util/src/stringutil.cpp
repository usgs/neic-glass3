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

	split(s, delim, elems);

	return (elems);
}

std::vector<std::string> &split(const std::string &s, char delim,
								std::vector<std::string> &elems) {  // NOLINT
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}

	return (elems);
}

// convert a narrow string to a wide string
std::wstring string2WString(const std::string& s) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(s);
	return (wide);
}

// convert a wide string to a narrow string
std::string wString2String(const std::wstring& s) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string narrow = converter.to_bytes(s);
	return (narrow);
}

// remove all instances of the character provided from the string
std::string& removeChars(std::string& s, const std::string& chars) {  // NOLINT
	s.erase(remove_if(s.begin(), s.end(), [&chars](const char& c) {
		return chars.find(c) != std::string::npos;
	}),
			s.end());

	return (s);
}

std::string& replaceChars(std::string& s, const std::string& from,  // NOLINT
							const std::string& to) {
	if (!from.empty()) {
		for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos;
				pos += to.size()) {
			s.replace(pos, from.size(), to);
		}
	}

	return (s);
}

// is this string a number
bool isStringNum(const std::string &s) {
	// use regex to figure out if this string contains a number (decimal point
	// and negitive sign allowed)
	return (std::regex_match(s, std::regex("-?[0-9]+([.][0-9]+)?")));
}

// is this string JUST characters
bool isStringAlpha(const std::string &s) {
	// use regex to figure out if this string contains only letters
	return (std::regex_match(s, std::regex("^[A-Za-z]+$")));
}

// does this string contain JUST characters and numbers
bool isStringAlphaNum(const std::string &s) {
	// use regex to figure out if this string contains letters and numbers
	return (std::regex_match(s, std::regex("^[A-Za-z0-9]+$")));
}

// does this string contain characters, numbers, '-', and '.'
bool isStringAlphaNumDashPeriod(const std::string &s) {
	// use regex to figure out if this string contains letters and numbers
	return (std::regex_match(s, std::regex("^[A-Za-z0-9\\.\\-]+$")));
}
}  // namespace util
