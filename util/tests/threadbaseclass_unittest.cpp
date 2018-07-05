#include <gtest/gtest.h>
#include <threadbaseclass.h>
#include <string>

#define TESTTHREADNAME "threadbasestub"
#define TESTSLEEPTIME 50
#define TESTSLEEPTIMECHANGE 100
#define TESTSTOPCOUNT 10
#define WAITTIME 2

class threadbasestub : public glass3::util::ThreadBaseClass {
 public:
	threadbasestub()
			: glass3::util::ThreadBaseClass() {
		runcount = 0;
		startcount = false;
		setCheckInterval(1);
		kill = false;
	}

	threadbasestub(std::string& name, int time)
			: glass3::util::ThreadBaseClass(name, time) {
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

	void testSetRunning(bool running) {
		setRunning(running);
	}

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
	std::string name = std::string(TESTTHREADNAME);

	// create a threadbasestub
	threadbasestub * TestThreadBaseStub = new threadbasestub(name,
	TESTSLEEPTIME);

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
	std::string name = std::string(TESTTHREADNAME);

	// create a threadbasestub
	threadbasestub * TestThreadBaseStub = new threadbasestub(name,
	TESTSLEEPTIME);

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

	// assert that check is false
	ASSERT_FALSE(TestThreadBaseStub->check())<<
	"TestThreadBaseStub check is false";

	// cleanup
	delete (TestThreadBaseStub);
}

// tests various failure conditions
TEST(ThreadBaseClassTest, FailTests) {
	// create a threadbasestub
	threadbasestub * TestThreadBaseStub = new threadbasestub();

	// assert we can't stop what we've not started
	ASSERT_FALSE(TestThreadBaseStub->stop())<<
	"TestThreadBaseStub stop is false";

	// start the thread
	ASSERT_TRUE(TestThreadBaseStub->start())<< "start was successful";
	ASSERT_FALSE(TestThreadBaseStub->start())<<
	"second start was not successful";

	// allocation test
	TestThreadBaseStub->testSetRunning(false);
	ASSERT_FALSE(TestThreadBaseStub->start())<<
	"second start was not successful";

	// kill it
	TestThreadBaseStub->kill = true;

	// wait a little while
	std::this_thread::sleep_for(std::chrono::seconds(WAITTIME));
	ASSERT_FALSE(TestThreadBaseStub->stop())<<
	"stop after fail was not successful";

	TestThreadBaseStub->testSetRunning(true);
	TestThreadBaseStub->setCheckWorkThread(false);
	ASSERT_FALSE(TestThreadBaseStub->check())<<
	"TestThreadBaseStub check is false";
}
