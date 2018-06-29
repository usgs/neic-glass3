#include <gtest/gtest.h>
#include <threadbaseclass.h>
#include <string>

#define TESTTHREADNAME "threadbasestub"
#define TESTSLEEPTIME 50
#define TESTSLEEPTIMECHANGE 100
#define TESTSTOPCOUNT 10
#define WAITTIME 2

class threadbasestub : public util::ThreadBaseClass {
 public:
	threadbasestub()
			: util::ThreadBaseClass(TESTTHREADNAME, TESTSLEEPTIME) {
		runcount = 0;
		startcount = false;
		setCheckInterval(1);
		kill = false;
	}

	~threadbasestub() {
	}

	bool startcount;

	bool kill;

	// count the runs
	int runcount;

 protected:
	bool work() override {
		if (kill) {
			return (false);
		}

		// keep running until count
		// is 10
		if ((startcount) && (runcount < TESTSTOPCOUNT)) {
			// increase count
			runcount++;

			// work successful
			setCheckWorkThread();
			return (true);
		}

		setCheckWorkThread();
		return (true);
	}
};

// tests to see if the threadbaseclass is functional
TEST(ThreadBaseClassTest, CombinedTest) {
	// create a threadbasestub
	threadbasestub * TestThreadBaseStub = new threadbasestub();

	// assert that class not started
	ASSERT_FALSE(TestThreadBaseStub->isStarted())<<
			"TestThreadBaseStub not started";

	// assert that class not running
	ASSERT_FALSE(TestThreadBaseStub->isRunning())<<
			"TestThreadBaseStub not running";

	// assert that check is true
	ASSERT_TRUE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is true";

	// check sleep time
	ASSERT_EQ(TestThreadBaseStub->getSleepTime(), TESTSLEEPTIME)<<
			"Check sleep time";

	// change sleeptime
	TestThreadBaseStub->setSleepTime(TESTSLEEPTIMECHANGE);

	// check changed sleep time
	ASSERT_EQ(TestThreadBaseStub->getSleepTime(), TESTSLEEPTIMECHANGE)<<
			"Check changed sleep time";

	// check thread name
	std::string testthreadname = std::string(TESTTHREADNAME);
	ASSERT_STREQ(TestThreadBaseStub->getThreadName().c_str(),
			testthreadname.c_str())<< "check thread name";

	// start the thread
	ASSERT_TRUE(TestThreadBaseStub->start())<< "start was successful";
	ASSERT_FALSE(TestThreadBaseStub->start())<<
			"second start was not successful";

	// assert that class started
	ASSERT_TRUE(TestThreadBaseStub->isStarted())<<
			"TestThreadBaseStub started";

	// wait a little while
	std::this_thread::sleep_for(std::chrono::seconds(WAITTIME / 2));

	// assert that class running
	ASSERT_TRUE(TestThreadBaseStub->isRunning())<< "TestThreadBaseStub running";

	// assert that check is true
	ASSERT_TRUE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is true";

	// tell stub to start counting
	TestThreadBaseStub->startcount = true;

	// wait a little while
	std::this_thread::sleep_for(std::chrono::seconds(WAITTIME));

	// assert that check is true
	ASSERT_TRUE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is true";

	// check count
	ASSERT_EQ(TestThreadBaseStub->runcount, TESTSTOPCOUNT)<< "Check count";

	// stop the thread
	ASSERT_TRUE(TestThreadBaseStub->stop())<< "stop was successful";

	// assert that class not started
	ASSERT_FALSE(TestThreadBaseStub->isStarted())<<
			"TestThreadBaseStub not started";

	// assert that class not running
	ASSERT_FALSE(TestThreadBaseStub->isRunning())<<
			"TestThreadBaseStub not running";

	// assert that check is true
	ASSERT_TRUE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is true";

	// cleanup
	delete (TestThreadBaseStub);
}

// tests to see if the queue is functional
TEST(ThreadBaseClassTest, KillTest) {
	// create a threadbasestub
	threadbasestub * TestThreadBaseStub = new threadbasestub();

	// start the thread
	ASSERT_TRUE(TestThreadBaseStub->start())<< "start was successful";

	// wait a little while
	std::this_thread::sleep_for(std::chrono::seconds(WAITTIME));

	// assert that check is true
	ASSERT_TRUE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is true";

	// kill it
	TestThreadBaseStub->kill = true;

	// wait a little while
	std::this_thread::sleep_for(std::chrono::seconds(WAITTIME));

	// assert that check is true
	ASSERT_FALSE(TestThreadBaseStub->check())<<
			"TestThreadBaseStub check is false";

	// cleanup
	delete (TestThreadBaseStub);
}
