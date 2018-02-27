#include <gtest/gtest.h>
#include <queue.h>
#include <string>
#include <memory>

#define TESTDATA1 "{\"HighPass\":1.000000,\"LowPass\":1.000000}"
#define TESTDATA2 "{\"HighPass\":2.000000,\"LowPass\":2.000000}"
#define TESTDATA3 "{\"HighPass\":3.000000,\"LowPass\":3.000000}"

// tests to see if the queue is functional
TEST(QueueTest, CombinedTest) {
	// create a queue
	util::Queue * TestQueue = new util::Queue();

	// assert an empty queue was created
	ASSERT_EQ(TestQueue->size(), 0)<< "empty queue constructed";

	// create input data
	std::string inputstring1 = std::string(TESTDATA1);
	std::shared_ptr<json::Object> inputdata1 = std::make_shared<json::Object>(
			json::Deserialize(inputstring1));

	std::string inputstring2 = std::string(TESTDATA2);
	std::shared_ptr<json::Object> inputdata2 = std::make_shared<json::Object>(
			json::Deserialize(inputstring2));

	std::string inputstring3 = std::string(TESTDATA3);
	std::shared_ptr<json::Object> inputdata3 = std::make_shared<json::Object>(
			json::Deserialize(inputstring3));

	// add data to queue
	TestQueue->addDataToQueue(inputdata1);
	TestQueue->addDataToQueue(inputdata2);
	TestQueue->addDataToQueue(inputdata3);

	// assert three items in the queue
	ASSERT_EQ(TestQueue->size(), 3)<< "3 items in queue";

	// get data from queue
	std::shared_ptr<json::Object> outputobject = TestQueue->getDataFromQueue();

	// assert that we got something
	ASSERT_TRUE(outputobject != NULL)<< "non-null queue data";

	std::string outputstring1 = json::Serialize(*outputobject);

	// assert that we got the object we expect
	ASSERT_STREQ(inputstring1.c_str(), outputstring1.c_str());

	// assert two items in the queue
	ASSERT_EQ(TestQueue->size(), 2)<< "2 items in queue";

	// clear queue
	TestQueue->clearQueue();

	// assert no items in the queue
	ASSERT_EQ(TestQueue->size(), 0)<< "no items in queue";

	// cleanup
	delete (TestQueue);
}
