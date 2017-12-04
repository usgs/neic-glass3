/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>
#include <vector>

namespace util {
/**
 * \brief split a string into substrings
 *
 * Split a string into substrings using the provided delimiter,
 * creating a std::vector to hold the results
 * \param s - A std::string containing the string to split
 * \param delim - A char containing the the delimiter to split with.
 * \return returns a std::vector containing the split std::string elements
 */
std::vector<std::string> split(const std::string &s, char delim);

/**
 * \brief split a string into substrings
 *
 * Split a string into substrings using the provided delimiter, using a given
 * std::vector to hold the results
 * \param s - A std::string containing the string to split
 * \param delim - A char containing the the delimiter to split with.
 * \param elems - A std::vector of std::strings used to contain the split
 * elements
 * \return returns a std::vector containing the split std::string elements
 */
std::vector<std::string> &split(const std::string &s, char delim,
								std::vector<std::string> &elems); // NOLINT

/**
 * \brief convert a narrow string to a wide string
 *
 * Convert a narrow string to a wide string
 * \param s - A std::string containing the narrow string
 * \return returns a std::wstring containing the converted wide string
 */
std::wstring string2WString(const std::string& s);

/**
 * \brief convert a wide string to a narrow string
 *
 * Convert a wide string to a narrow string
 * \param s - A std::wstring containing the wide string
 * \return returns a std::string containing the converted narrow string
 */
std::string wString2String(const std::wstring& s);

/**
 * \brief remove all instances of characters from a string
 *
 * Remove all instances of the character provided from the given string
 * \param s - A std::string containing the string to remove characters from
 * \param chars - A std::string containing the characters to remove.
 * \return returns a std::string containing modified string.
 */
std::string& removeChars(std::string& s, const std::string& chars); // NOLINT

/**
 * \brief replace all instances of a substring in a string with a new string
 *
 * Replace all instances of the substring provided in the given string
 * with a new string.
 * \param s - A std::string containing the string to remove characters from
 * \param from - A std::string containing the characters to replace.
 * \param to - A std::string containing the replacement characters.
 * \return returns a std::string containing modified string.
 */
std::string& replaceChars(std::string& s, const std::string& from, // NOLINT
							const std::string& to);

/**
 * \brief is string a number
 *
 * Tests to see if a provided string contains only numbers
 * (and '.')
 * \param s - A std::string containing the string to test
 * \return returns a true if the string contains only numbers, false otherwise
 */
bool isStringNum(const std::string &s);

/**
 * \brief is string just characters
 *
 * Tests to see if a provided string contains only characters
 * \param s - A std::string containing the string to test
 * \return returns a true if the string contains only characters, false
 * otherwise
 */
bool isStringAlpha(const std::string &s);

/**
 * \brief is string just characters and numbers
 *
 * Tests to see if a provided string contains only characters and numbers
 * \param s - A std::string containing the string to test
 * \return returns a true if the string contains only characters and numbers,
 * false otherwise
 */
bool isStringAlphaNum(const std::string &s);

/**
 * \brief is string just characters, numbers, '-' and '.'
 *
 * Tests to see if a provided string contains only characters, numbers, '-'
 * and '.'
 * \param s - A std::string containing the string to test
 * \return returns a true if the string contains only characters, numbers, '-'
 * and '.',
 * false otherwise
 */
bool isStringAlphaNumDashPeriod(const std::string &s);
}  // namespace util
#endif  // STRINGUTIL_H
