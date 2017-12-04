#include <gtest/gtest.h>
#include <fileutil.h>
#include <stringutil.h>

#ifdef _WIN32
#include <direct.h>
#endif

#include <iostream>
#include <fstream>
#include <string>

#define WINDOWSSLASH "\\"
#define UNIXSLASH "/"
#define TESTDATA "testdata"
#define TESTPATH1 "testpath1"
#define TESTPATH2 "testpath2"
#define TESTFILENAME "testfile"
#define TESTCOPYFILE "copyfile"
#define TESTFILEEXTENSION "test"
#define COPYFILEEXTENSION "copy"

class FileUtil : public ::testing::Test {
 protected:
	virtual void SetUp() {
		// logger::log_init("fileutil", spdlog::level::debug, true, "./");

		int nError = 0;

		std::string testdata = std::string(TESTDATA);
		std::string testpath1 = std::string(TESTPATH1);
		std::string testpath2 = std::string(TESTPATH2);

		// set up testing paths
		firsttestpath = "./" + testdata + "/" + testpath1;
		secondtestpath = "./" + testdata + "/" + testpath2;

		// create testing directories
#ifdef _WIN32
		nError = _mkdir(firsttestpath.c_str());
		nError = _mkdir(secondtestpath.c_str());
#else
		mode_t nMode = 0733;
		nError = mkdir(firsttestpath.c_str(), nMode);
		nError = mkdir(secondtestpath.c_str(), nMode);
#endif
		if (nError != 0) {
			printf("Failed to create testing directory %s.\n",
					firsttestpath.c_str());
			printf("Failed to create testing directory %s.\n",
					secondtestpath.c_str());
		}

		std::string filename = std::string(TESTFILENAME);
		std::string copyfilename = std::string(TESTCOPYFILE);
		fileextension = std::string(TESTFILEEXTENSION);
		copyfileextenstion = std::string(COPYFILEEXTENSION);

		// build file name
		testfilename = firsttestpath + "/" + filename + "." + fileextension;

		// build moved file name
		movedfilename = secondtestpath + "/" + filename + "." + fileextension;

		// build copy source file name
		copysourcefilename = firsttestpath + "/" + copyfilename + "."
				+ copyfileextenstion;

		// build copy destionation file name
		copydestinationfilename = secondtestpath + "/" + copyfilename + "."
				+ copyfileextenstion;

		// create files
		// testfile
		std::ofstream outfile;
		outfile.open(testfilename, std::ios::out);

		if ((outfile.rdstate() & std::ifstream::failbit) != 0) {
			printf("Failed to create testing file %s.\n", testfilename.c_str());
		}

		// write data to file
		outfile << "This is a file generated as part of unit testing.";

		// done
		outfile.close();

		// copyfile
		std::ofstream copyfile;
		copyfile.open(copysourcefilename, std::ios::out);

		if ((copyfile.rdstate() & std::ifstream::failbit) != 0) {
			printf("Failed to create testing file %s.\n",
					copysourcefilename.c_str());
		}

		// write data to file
		copyfile << "This is a file generated as part of unit testing.";

		// done
		copyfile.close();
	}

	virtual void TearDown() {
		// file should already cleaned up as part of tests, but look for it
		// anyway in case this was part of some failed test
		if (std::ifstream(testfilename).good()) {
			std::remove(testfilename.c_str());
		}

		if (std::ifstream(movedfilename).good()) {
			std::remove(movedfilename.c_str());
		}

		if (std::ifstream(copysourcefilename).good()) {
			std::remove(copysourcefilename.c_str());
		}

		if (std::ifstream(copydestinationfilename).good()) {
			std::remove(copydestinationfilename.c_str());
		}

		// remove directories
#ifdef _WIN32
		RemoveDirectory(firsttestpath.c_str());
		RemoveDirectory(secondtestpath.c_str());
#else
		rmdir(firsttestpath.c_str());
		rmdir(secondtestpath.c_str());
#endif
	}

	std::string firsttestpath;
	std::string secondtestpath;
	std::string fileextension;
	std::string copyfileextenstion;
	std::string testfilename;
	std::string movedfilename;
	std::string copysourcefilename;
	std::string copydestinationfilename;
};

// test all aspects of FileUtil
TEST_F(FileUtil, CombinedTests) {
	// getnextfilename
	std::string foundfilename = "";
	bool result = util::getNextFileName(firsttestpath, fileextension,
										foundfilename);

	// make sure we found the file
	ASSERT_TRUE(result)<< "getnextfilename call";

	// check the file name
	ASSERT_STREQ(foundfilename.c_str(), testfilename.c_str())<<
			"expected vs, found file names.";

	// movefileto
	result = util::moveFileTo(testfilename, secondtestpath);

	// make sure we moved the file
	ASSERT_TRUE(result)<< "movefileto call";

	// look for the moved file
	foundfilename = "";
	result = util::getNextFileName(secondtestpath, fileextension,
									foundfilename);

	// make sure we found the moved file
	ASSERT_TRUE(result)<< "getnextfilename moved call";

	// check the moved file name
	ASSERT_STREQ(foundfilename.c_str(), movedfilename.c_str())<<
			"expected vs, moved file names.";

	// deletefilefrom
	result = util::deleteFileFrom(movedfilename);

	// make sure we found the file
	ASSERT_TRUE(result)<< "deletefilefrom call";

	// copyfileto
	result = util::copyFileTo(copysourcefilename, copydestinationfilename);

	// make sure we copied the file
	ASSERT_TRUE(result)<< "copyfileto call";

	// look for the copied file
	foundfilename = "";
	result = util::getNextFileName(secondtestpath, copyfileextenstion,
									foundfilename);

	// make sure we found the copied file
	ASSERT_TRUE(result)<< "getnextfilename copy call";

	// check the copied file name
	ASSERT_STREQ(foundfilename.c_str(), copydestinationfilename.c_str())<<
			"expected vs, copied file names.";
}
