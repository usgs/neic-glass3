#include <gtest/gtest.h>
#include <threadpool.h>
#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

#define THREADPOOLNAME "testpool"
#define NUMTHREADS 5
#define SLEEPTIME 100
#define MAXJOBS 3
#define CHECKTIME 1

#define TESTJOB1RESULT 25
int testjob1data = 0;
void testjob1() {
	testjob1data = 5 * 5;
}

#define TESTJOB2RESULT 360
int testjob2data = 0;
void testjob2() {
	testjob2data = 5 * 6 * 4 * 3;
}

#define TESTJOB3RESULT 231
int testjob3data = 0;
void testjob3() {
	testjob3data = 11 * 7 * 3;
}

#define TESTJOB4RESULT 99
int testjob4data = 0;
void testjob4() {
	testjob4data = 4489 / 45;
}

#define TESTJOB5RESULT 217800
int testjob5data = 0;
void testjob5() {
	testjob5data = 45 * 88 * 55;
}

int result = 0;
void badjob() {
	result = 0;
	throw std::invalid_argument("Bad Job");
}

// tests to see if the threadpool is functional
TEST(ThreadPoolTest, CombinedTest) {
	std::string poolname = std::string(THREADPOOLNAME);

	// create a threadpool object
	glass3::util::ThreadPool * ThreadPool = new glass3::util::ThreadPool(
			poolname, NUMTHREADS,
			SLEEPTIME,
			CHECKTIME);

	// assert threads running
	ASSERT_TRUE(ThreadPool->isRunning())<< "check threads running";

	// assert pool name
	ASSERT_STREQ(ThreadPool->getPoolName().c_str(), poolname.c_str())<<
	"pool name";

	// assert number of threads
	ASSERT_EQ(ThreadPool->getNumThreads(), NUMTHREADS)<<
	"check number of pool threads";

	// assert thread sleep time
	ASSERT_EQ(ThreadPool->getSleepTime(), SLEEPTIME)<<
	"check thread sleep time";

	// add some jobs
	ThreadPool->addJob(std::bind(&testjob1));
	ThreadPool->addJob(std::bind(&testjob2));
	ThreadPool->addJob(std::bind(&testjob3));
	ThreadPool->addJob(std::bind(&testjob4));
	ThreadPool->addJob(std::bind(&testjob5));

	// give time for jobs to finish
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// check job results
	ASSERT_EQ(testjob1data, TESTJOB1RESULT)<< "test job 1 is correct";
	ASSERT_EQ(testjob2data, TESTJOB2RESULT)<< "test job 2 is correct";
	ASSERT_EQ(testjob3data, TESTJOB3RESULT)<< "test job 3 is correct";
	ASSERT_EQ(testjob4data, TESTJOB4RESULT)<< "test job 4 is correct";
	ASSERT_EQ(testjob5data, TESTJOB5RESULT)<< "test job 5 is correct";

	// assert that check is true
	ASSERT_TRUE(ThreadPool->check())<< "ThreadPool check is true";

	// shutdown threadpool.
	delete (ThreadPool);
}

// tests to see if thread pool monitoring works
TEST(ThreadPoolTest, MonitoringTest) {
	std::string poolname = std::string(THREADPOOLNAME);

	// create a threadpool object
	glass3::util::ThreadPool * ThreadPool = new glass3::util::ThreadPool(
			poolname, NUMTHREADS,
			SLEEPTIME,
			CHECKTIME);

	// give time for jobs to be monitored
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that check is true
	ASSERT_TRUE(ThreadPool->check())<< "ThreadPool check is true";

	// add a bad job
	ThreadPool->addJob(std::bind(&badjob));

	// give time for jobs to be monitored
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that check is true
	ASSERT_TRUE(ThreadPool->check())<< "ThreadPool check is true";

	// give time for jobs to be monitored
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// assert that check is false
	ASSERT_FALSE(ThreadPool->check())<< "ThreadPool check is false";

	// shutdown threadpool.
	delete (ThreadPool);
}

// tests various failure conditions
TEST(ThreadPoolTest, FailTests) {
	glass3::util::ThreadPool * ThreadPool = new glass3::util::ThreadPool();

	// assert that check is false
	ASSERT_FALSE(ThreadPool->check())<< "ThreadPool check is false";

	// shutdown threadpool.
	delete (ThreadPool);

}

