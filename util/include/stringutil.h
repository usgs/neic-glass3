/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief timeutil.h
 *
 * stringutil.h is a set of functions that manage string functions not provided
 * buy the standard template library, including splitting strings and replacing
 * characters within strings.
 */
#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>
#include <vector>

namespace glass3 {
namespace util {
/**
 * \brief split a string into substrings
 *
 * Split a string into substrings using the provided delimiter,
 * creating a std::vector to hold the results
 *
 * \param s - A std::string containing the string to split
 * \param delim - A char containing the the delimiter to split with.
 * \return returns a std::vector containing the split std::string elements
 */
std::vector<std::string> split(const std::string &s, char delim);

/**
 * \brief remove all instances of characters from a string
 *
 * Remove all instances of the set of characters stored in chars from the given
 * string.
 * This is NOT removing a substring from a string.
 *
 * \param s - A std::string containing the string to remove characters from
 * \param chars - A std::string containing the characters to remove.
 * \return returns a std::string containing modified string.
 */
std::string& removeChars(std::string& s, const std::string& chars);  // NOLINT
}  // namespace util
}  // namespace glass3
#endif  // STRINGUTIL_H
