/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <string>

namespace util {
/**
 * \brief get the next file name in a directory
 *
 * Gets the next file name that matches a given extension from a given directory
 * \param path - A std::string containing the directory to search for a file
 * name
 * \param extension - A std::string containing the extension to filter with.
 * \param filename - A std::string containing the file name that was found
 * \return returns true if successful.
 */
bool getNextFileName(const std::string &path, const std::string &extension,
						std::string &filename);  // NOLINT

/**
 * \brief move a file from one directory to another
 *
 * Move a given file to a given directory
 * \param filename - A std::string containing the path and name of the file to
 * move
 * \param dirname - A std::string containing the directory to move the file to
 * move
 * \return returns true if successful.
 */
bool moveFileTo(std::string filename, const std::string &dirname);

/**
 * \brief copy a file from one directory to another
 *
 * Copy a given file to a given directory
 * \param from - A std::string containing the path and name of the file to
 * copy
 * \param dirname - A std::string containing the path and name of the
 * destination file
 * \return returns true if successful.
 */
bool copyFileTo(std::string from, std::string to);

/**
 * \brief delete a file
 *
 * Delete a given file
 * \param filename - A std::string containing the path and name of the file to
 * delete
 * \return returns true if successful.
 */
bool deleteFileFrom(std::string filename);
}  // namespace util
#endif  // FILEUTIL_H
