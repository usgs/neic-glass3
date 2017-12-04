#include <gtest/gtest.h>
#include <socketutil.h>
#include <string>

// note that this test data is tailored for metadataserver,
// for lack of a better server to connect to
#define HOSTNAME "cwbpub.cr.usgs.gov"
#define HOSTPORT "2052"
#define MESSAGE "version\n"
#define RESPONSE "** Too few arguments to new format\n* <EOR>"

// test all aspects of FileUtil
TEST(SocketUtil, CombinedTests) {
	std::string servername = std::string(HOSTNAME);
	std::string serverport = std::string(HOSTPORT);

	// set up a socket to send the request to the server
	SOCKET ClientSocket = util::connectToServer(servername, serverport);

	// check to see if we got a connection
	ASSERT_NE(INVALID_SOCKET, ClientSocket)<< "connectToServer";

	// build the query
	std::string message = std::string(MESSAGE);

	// send message
	util::sendToServer(ClientSocket, message);

	// receive response
	std::string response = util::recieveFromServer(ClientSocket);
	std::string expectedresponse = std::string(RESPONSE);

	// check the response
	EXPECT_STREQ(response.c_str(), expectedresponse.c_str())
			<< "recieveFromServer";

	// done with socket
	int returncode = util::disconnectFromServer(ClientSocket);

	// check that we disconnected without error
	ASSERT_EQ(returncode, 0)<< "disconnectFromServer";
}
