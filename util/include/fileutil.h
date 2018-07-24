/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief fileutl.h
 *
 * fileutl.h is a set of functions that manage file functions not provided
 * buy the standard template library, including directory file searching file
 * copying, file moving, and file deletion.
 */
#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <string>

namespace glass3 {
namespace util {
/**
 * \brief get the next file name in a directory
 *
 * Gets the next file name that matches a given extension from a given directory
 *
 * \param path - A std::string containing the directory to search for a file
 * name
 * \param extension - A std::string containing the extension to filter with.
 * \param fileName - A std::string containing the file name that was found
 * \return returns true if successful.
 */
bool getFirstFileNameByExtension(const std::string &path,
									const std::string &extension,
									std::string &fileName);  // NOLINT

/**
 * \brief move a file from one directory to another
 *
 * Move a given file to a given directory
 *
 * \param fileName - A std::string containing the path and name of the file to
 * move
 * \param dirName - A std::string containing the directory to move the file to
 * move
 * \return returns true if successful or expected error (ENOENT), or (EACCES -
 * trying to overwrite an existing file in the move), in either case the
 * fileName is expected to no longer exist after a return of true.
 */
bool moveFileTo(std::string fileName, const std::string &dirName);

/**
 * \brief copy a file from one directory to another
 *
 * Copy a given file to a given directory
 *
 * \param from - A std::string containing the path and name of the file to
 * copy
 * \param to - A std::string containing the path and name of the
 * destination file
 * \return returns true if successful.
 */
bool copyFileTo(std::string from, std::string to);

/**
 * \brief delete a file
 *
 * Delete a given file from the file system
 *
 * \param fileName - A std::string containing the path and name of the file to
 * delete
 * \return returns true if successful.
 */
bool deleteFileFrom(std::string fileName);
}  // namespace util
}  // namespace glass3
#endif  // FILEUTIL_H
