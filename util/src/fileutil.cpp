#include <fileutil.h>
#include <logger.h>
#include <stringutil.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define MOVEERROREXTENSION ".moveerror"

namespace util {
// get the next filename with the provided extension from a directory
bool getNextFileName(const std::string &path, const std::string &extension,
						std::string &filename) {  // NOLINT
	logger::log(
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
			logger::log("trace", "getnextfilename(): File not found.");
		} else {
			logger::log(
					"error",
					"getnextfilename(): Error " + std::to_string(error)
							+ " calling FindFirstFile with filter " + findfilter
							+ ".");
		}

		// FindClose(findfileshandle);
		return (false);
	}

	// loop through files in directory until we find a valid one
	while (filefound == false) {
		if (!(findfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// found something that isn't a directory
			filefound = true;
			continue;
		}

		// The preceding file wasn't one we can use.  Keep looking...
		if (FindNextFile(findfileshandle, &findfiledata) == 0) {
			error = GetLastError();
			if (error == ERROR_NO_MORE_FILES) {
				logger::log("trace", "getnextfilename(): No more files in "
							"directory.");
			} else {
				logger::log(
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
	filename = path + std::string("/") + std::string(findfiledata.cFileName);

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
		logger::log(
				"error",
				"Couldn't open directory " + path + " Error: "
						+ std::to_string(errno));
		return (false);
	}

	while ((ent = readdir(dir)) != NULL) {
		// convert filename to string
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

		// check to see if filename contains our extension.
		if (file_name.find("." + extension) != std::string::npos)
			files.push_back(file_name);
	}

	// find anything?
	if (files.size() == 0) {
		// nothing found
		filename = "";

		// done looking for files
		closedir(dir);
		return (false);
	}

	// sort filenames
	std::sort(files.begin(), files.end());

	// get the first filename in the list
	// and format it
	filename = path + std::string("/") + files[0];

	// done looking for files
	closedir(dir);
	return (true);
#endif
}

// move a file from one directory to another
bool moveFileTo(std::string filename, const std::string &dirname) {
	std::string fromStr;
	std::string toStr;

	// get where the file name starts
	int startPosOfFilename = static_cast<int>(filename.find_last_of("/"));

	// special case for windows paths
#ifdef _WIN32
	if (startPosOfFilename == std::string::npos)
	startPosOfFilename = static_cast<int>(filename.find_last_of("\\"));
#endif

	int filenameLength = static_cast<int>(filename.size()) - startPosOfFilename;

	std::string filenameNoPath;

	// build fromstring
	if (startPosOfFilename > 0 && filenameLength > 1) {
		// filename is assumed to contain path
		fromStr = filename;
		filenameNoPath = filename.substr(startPosOfFilename, filenameLength);
	} else {
		return (false);
	}

	// build tostring
#ifdef _WIN32
	toStr = dirname + "\\" + filenameNoPath;
#else
	toStr = dirname + "/" + filenameNoPath;
#endif

	logger::log("debug",
				"movefileto(): Moving file " + fromStr + " to " + toStr + ".");

	// move it!
	if (std::rename(fromStr.c_str(), toStr.c_str())) {
		return (true);
	} else {
		std::string badfilename;

		if (errno == EACCES) {
			// Somehow, we already dealt with this file - we're either testing,
			// or somehow got two copies of the same file.  Since they should
			// be identical, just log it and delete the file we had wanted to
			// move.
			logger::log(
					"warning",
					"movefileto(): Unable to move " + fromStr + " to " + toStr
							+ ": File already exists.");

			if (deleteFileFrom(fromStr) != true)
				return (false);

			return (true);
		} else if (errno == ENOENT) {
			// This happens on occasion, and really it's fine so let's not freak
			// out...
			logger::log(
					"warning",
					"movefileto(): Unable to move " + fromStr + " to " + toStr
							+ ": File Not Found Error.");

			return (true);
		} else if (errno == 0) {
			return (true);
		}

		// Something else happened...
		logger::log(
				"error",
				"movefileto(): Unable to move " + fromStr + " to " + toStr
						+ ": Error " + strerror(errno) + " errno: "
						+ std::to_string(errno) + ".");

		// Rather just leave the file alone, we'll want to rename it as an error
		// file.  This is so the inserter doesn't eternally attempt to insert
		// this one file.
		badfilename = filename + std::string(MOVEERROREXTENSION);
		if (!std::rename(filename.c_str(), badfilename.c_str())) {
			logger::log(
					"error",
					"movefileto(): Unable to rename " + fromStr + " to " + toStr
							+ ": Error " + strerror(errno) + ".");

			// All right, then...just delete the offending piece of $&@^@*#^&
			if (deleteFileFrom(fromStr) != true)
				return (false);
		}

		return (false);
	}
	return (true);
}

// copy a file
bool copyFileTo(std::string from, std::string to) {
	std::ifstream source(from, std::ios::binary);
	if (!source) {
		logger::log(
				"error",
				"copyfileto(): Unable to open source " + from
						+ " for copying.");
		return (false);
	}

	std::ofstream dest(to, std::ios::binary);
	if (!dest) {
		logger::log(
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

// delete a file from a directory
bool deleteFileFrom(std::string filename) {
	logger::log("debug", "deletefilefrom(): Deleting file " + filename + ".");

	// delete it
	if (!std::remove(filename.c_str())) {
		// check to see if file is still there
		if (std::ifstream(filename.c_str())) {
			logger::log(
					"error",
					"deletefilefrom(): Unable to delete file " + filename
							+ ": Error " + strerror(errno) + ".");

			return (false);
		}
	}
	return (true);
}
}  // namespace util
