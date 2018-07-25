#include <gtest/gtest.h>
#include <fileutil.h>
#include <stringutil.h>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
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
#define BADPATH1 "badpath1"

class FileUtil : public ::testing::Test {
 protected:
	virtual void SetUp() {
		// glass3::util::log_init("fileutil", spdlog::level::debug, true, "./");

		int nError1 = 0;
		int nError2 = 0;

		std::string testdata = std::string(TESTDATA);
		std::string testpath1 = std::string(TESTPATH1);
		std::string testpath2 = std::string(TESTPATH2);
		std::string badpath1 = std::string(BADPATH1);

		// set up testing paths
		firsttestpath = "./" + testdata + "/" + testpath1;
		secondtestpath = "./" + testdata + "/" + testpath2;
		badpath = "./" + testdata + "/" + badpath1;

		// create testing directories
#ifdef _WIN32
		nError1 = _mkdir(firsttestpath.c_str());
		nError2 = _mkdir(secondtestpath.c_str());
#else
		mode_t nMode = 0733;
		nError1 = mkdir(firsttestpath.c_str(), nMode);
		nError2 = mkdir(secondtestpath.c_str(), nMode);
#endif
		if (nError1 != 0) {
			printf("Failed to create testing directory %s.\n",
					firsttestpath.c_str());
		}
		if (nError2 != 0) {
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

		// build bad file name
		badfilename = badpath + "/" + filename + "." + fileextension;

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
		// file should already be cleaned up as part of tests, but look for it
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
	std::string badpath;
	std::string fileextension;
	std::string copyfileextenstion;
	std::string testfilename;
	std::string movedfilename;
	std::string badfilename;
	std::string copysourcefilename;
	std::string copydestinationfilename;
};

// test all aspects of FileUtil
TEST_F(FileUtil, CombinedTests) {
	// getnextfilename
	std::string foundfilename = "";
	bool result = glass3::util::getFirstFileNameByExtension(firsttestpath,
															fileextension,
															foundfilename);

	// make sure we found the file
	ASSERT_TRUE(result)<< "getnextfilename call";

	// check the file name
	ASSERT_STREQ(foundfilename.c_str(), testfilename.c_str())<<
	"expected vs, found file names.";

	// movefileto
	result = glass3::util::moveFileTo(testfilename, secondtestpath);

	// make sure we moved the file
	ASSERT_TRUE(result)<< "movefileto call";

	// look for the moved file
	foundfilename = "";
	result = glass3::util::getFirstFileNameByExtension(secondtestpath,
														fileextension,
														foundfilename);

	// make sure we found the moved file
	ASSERT_TRUE(result)<< "getnextfilename moved call";

	// check the moved file name
	ASSERT_STREQ(foundfilename.c_str(), movedfilename.c_str())<<
	"expected vs, moved file names.";

	// deletefilefrom
	result = glass3::util::deleteFileFrom(movedfilename);

	// make sure we found the file
	ASSERT_TRUE(result)<< "deletefilefrom call";

	// copyfileto
	result = glass3::util::copyFileTo(copysourcefilename,
										copydestinationfilename);

	// make sure we copied the file
	ASSERT_TRUE(result)<< "copyfileto call";

	// look for the copied file
	foundfilename = "";
	result = glass3::util::getFirstFileNameByExtension(secondtestpath,
														copyfileextenstion,
														foundfilename);

	// make sure we found the copied file
	ASSERT_TRUE(result)<< "getnextfilename copy call";

	// check the copied file name
	ASSERT_STREQ(foundfilename.c_str(), copydestinationfilename.c_str())<<
	"expected vs, copied file names.";
}

// test failure cases
TEST_F(FileUtil, FailTests) {
	std::string foundfilename = "";
	bool result = glass3::util::getFirstFileNameByExtension(badpath,
															fileextension,
															foundfilename);

	// make sure we found the file
	ASSERT_FALSE(result)<< "getnextfilename call";

	// movefileto
	result = glass3::util::moveFileTo(badfilename, badpath);

	// make sure we moved the file
	ASSERT_TRUE(result)<< "movefileto call";

	// movefileto
	result = glass3::util::moveFileTo(badfilename, secondtestpath);

	// make sure we moved the file
	ASSERT_TRUE(result)<< "movefileto call";

	// copyfileto
	result = glass3::util::copyFileTo(badfilename, copydestinationfilename);

	// make sure we copied the file
	ASSERT_FALSE(result)<< "copyfileto call";

	// copyfileto
	result = glass3::util::copyFileTo(copysourcefilename, badfilename);

	// make sure we copied the file
	ASSERT_FALSE(result)<< "copyfileto call";

	// deletefilefrom
	result = glass3::util::deleteFileFrom(badfilename);

	// make sure we found the file
	ASSERT_FALSE(result)<< "deletefilefrom call";
}
