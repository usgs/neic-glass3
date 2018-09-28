#include <fileutil.h>
#include <logger.h>
#include <stringutil.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <errno.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

#define MOVEERROREXTENSION ".moveerror"

namespace glass3 {
namespace util {

// -------------------------------------------------getFirstFileNameByExtension
bool getFirstFileNameByExtension(const std::string &path,
									const std::string &extension,
									std::string &fileName) {  // NOLINT
	glass3::util::Logger::log(
			"trace",
			"getnextfilename(): Using path:" + path + " and extension: "
					+ extension);

#ifdef _WIN32
	std::string findfilter;
	HANDLE findfileshandle;
	WIN32_FIND_DATA findfiledata;
	int error = 0;
	bool filefound = false;

	// build our filter
	findfilter = path + std::string("\\*.") + extension;

	// find the first file in the directory
	findfileshandle = FindFirstFile(findfilter.c_str(), &findfiledata);
	if (findfileshandle == INVALID_HANDLE_VALUE) {
		error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND) {
			glass3::util::Logger::log("trace", "getnextfilename(): File not found.");
		} else {
			glass3::util::Logger::log(
					"error",
					"getnextfilename(): Error " + std::to_string(error)
					+ " calling FindFirstFile with filter " + findfilter
					+ ".");
		}

		return (false);
	}

	// loop through files in directory until we find a valid one
	while (filefound == false) {
		if (!(findfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// found something that isn't a directory
			filefound = true;
			break;
		}

		// The preceding file wasn't one we can use.  Keep looking...
		if (FindNextFile(findfileshandle, &findfiledata) == 0) {
			error = GetLastError();
			if (error == ERROR_NO_MORE_FILES) {
				glass3::util::Logger::log("trace", "getnextfilename(): No more files in "
						"directory.");
			} else {
				glass3::util::Logger::log(
						"error",
						"getnextfilename(): Error " + std::to_string(error)
						+ " calling FindNextFile with filter "
						+ findfilter + ".");
			}

			// no point in staying in the loop
			FindClose(findfileshandle);
			return (false);
		}
	}

	// format file name
	fileName = path + std::string("/") + std::string(findfiledata.cFileName);

	// done looking for files
	FindClose(findfileshandle);
	return (true);
#else
	// need
	DIR *dir;
	struct dirent *ent;
	struct stat st;
	std::vector<std::string> files;

	dir = opendir(path.c_str());

	if (dir == NULL) {
		glass3::util::Logger::log(
				"error",
				"Couldn't open directory " + path + " Error: "
						+ std::to_string(errno));
		return (false);
	}

	while ((ent = readdir(dir)) != NULL) {
		// convert fileName to string
		std::string file_name = std::string(ent->d_name);
		std::string full_file_name = path + "/" + file_name;

		// ensure that this isn't a file that failed to move
		if (file_name.find(std::string(MOVEERROREXTENSION))
				!= std::string::npos)
			continue;

		// check for directory
		if (stat(full_file_name.c_str(), &st) == -1)
			continue;
		if ((st.st_mode & S_IFDIR) != 0)
			continue;

		// check to see if fileName contains our extension.
		if (file_name.find("." + extension) != std::string::npos)
			files.push_back(file_name);
	}

	// find anything?
	if (files.size() == 0) {
		// nothing found
		fileName = "";

		// done looking for files
		closedir(dir);
		return (false);
	}

	// sort filenames
	std::sort(files.begin(), files.end());

	// get the first fileName in the list
	// and format it
	fileName = path + std::string("/") + files[0];

	// done looking for files
	closedir(dir);
	return (true);
#endif
}

// ---------------------------------------------------------moveFileTo
bool moveFileTo(std::string fileName, const std::string &dirName) {
	std::string fromStr;
	std::string toStr;
	std::string filenameNoPath;

	// get where the file name starts
	int startPosOfFilename = static_cast<int>(fileName.find_last_of("/"));

	// special case for windows paths
#ifdef _WIN32
	if (startPosOfFilename == std::string::npos)
	startPosOfFilename = static_cast<int>(fileName.find_last_of("\\"));
#endif

	int filenameLength = static_cast<int>(fileName.size()) - startPosOfFilename;

	// build fromstring
	if (startPosOfFilename > 0 && filenameLength > 1) {
		// fileName is assumed to contain path
		fromStr = fileName;
		filenameNoPath = fileName.substr(startPosOfFilename, filenameLength);
	} else {
		return (false);
	}

	// build tostring
#ifdef _WIN32
	toStr = dirName + "\\" + filenameNoPath;
#else
	toStr = dirName + "/" + filenameNoPath;
#endif

	glass3::util::Logger::log(
			"debug",
			"movefileto(): Moving file " + fromStr + " to " + toStr + ".");

	// move it!
	if (std::rename(fromStr.c_str(), toStr.c_str()) == 0) {
		return (true);
	} else {
		std::string badfilename;

		if (errno == EACCES) {
			// We are presuming the EACCES is an error when trying to overwrite
			// an existing file and not a permission issue with the "from" file.
			// Somehow, we already dealt with this file - we're either testing,
			// or somehow got two copies of the same file.  Since they should
			// be identical, just log it and delete the file we had wanted to
			// move.
			glass3::util::Logger::log(
					"warning",
					"movefileto(): Unable to move " + fromStr + " to " + toStr
							+ ": File already exists.");

			if (deleteFileFrom(fromStr) != true) {
				return (false);
			}

			return (true);
		} else if (errno == ENOENT) {
			// This is saying that it can't move the file likely because it
			// can't find or access the from file. This implies that either it's
			// somewhat ready but not fully ready for finding or someone came
			// along and removed it between the time we found it and the time
			// we could move it.
			glass3::util::Logger::log(
					"warning",
					"movefileto(): Unable to move " + fromStr + " to " + toStr
							+ ": File Not Found Error.");

			return (true);
		}

		// Something else happened...
		glass3::util::Logger::log(
				"error",
				"movefileto(): Unable to move " + fromStr + " to " + toStr
						+ ": Error " + strerror(errno) + " errno: "
						+ std::to_string(errno) + ".");

		// Rather just leave the file alone, we'll want to rename it as an error
		// file.  This is so the inserter doesn't eternally attempt to insert
		// this one file.
		badfilename = fileName + std::string(MOVEERROREXTENSION);
		if (std::rename(fileName.c_str(), badfilename.c_str()) != 0) {
			glass3::util::Logger::log(
					"error",
					"movefileto(): Unable to rename " + fromStr + " to " + toStr
							+ ": Error " + strerror(errno) + ".");

			// All right, then...just delete the offending piece of $&@^@*#^&
			if (deleteFileFrom(fromStr) != true)
				return (false);
		}

		return (false);
	}  // else std::rename != 0

	return (true);
}

// ---------------------------------------------------------copyFileTo
bool copyFileTo(std::string from, std::string to) {
	std::ifstream source(from, std::ios::binary);
	if (!source) {
		glass3::util::Logger::log(
				"error",
				"copyfileto(): Unable to open source " + from
						+ " for copying.");
		return (false);
	}

	std::ofstream dest(to, std::ios::binary);
	if (!dest) {
		glass3::util::Logger::log(
				"error",
				"copyfileto(): Unable to open destination" + to
						+ " for copying.");
		return (false);
	}

	// copy it
	dest << source.rdbuf();

	// close files
	source.close();
	dest.close();

	return (true);
}

// ---------------------------------------------------------deleteFileFrom
bool deleteFileFrom(std::string fileName) {
	glass3::util::Logger::log(
			"debug", "deletefilefrom(): Deleting file " + fileName + ".");

	// check to see if the file exists
	if (!std::ifstream(fileName.c_str())) {
		glass3::util::Logger::log(
				"error",
				"deletefilefrom(): Unable to delete file " + fileName
						+ ": file did not exist.");

		return (false);
	}

	// delete it
	std::remove(fileName.c_str());

	// check to see if file is still there
	if (std::ifstream(fileName.c_str())) {
		glass3::util::Logger::log(
				"error",
				"deletefilefrom(): Unable to delete file " + fileName
						+ ": Error " + strerror(errno) + ".");

		return (false);
	}

	return (true);
}
}  // namespace util
}  // namespace glass3
