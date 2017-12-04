#include <socketutil.h>
#include <logger.h>
#include <string>

namespace util {
// connect to a server
SOCKET connectToServer(std::string hostname, std::string serviceport) {
	if (hostname == "") {
		logger::log("error", "connectToServer(): Bad hostname provided.");
		return (INVALID_SOCKET);
	}

	if (serviceport == "") {
		logger::log("error", "connectToServer(): Bad serviceport provided.");
		return (INVALID_SOCKET);
	}

	SOCKET ClientSocket = INVALID_SOCKET;
	int SocketResult;
	struct hostent * RemoteHost;

	// Initialize sockets
	SocketResult = socketInitialize();
	if (SocketResult != 0) {
		logger::log(
				"error",
				"connectToServer(): socketInitialize failed with error: "
						+ std::to_string(SocketResult) + ".");
		return (INVALID_SOCKET);
	}
	logger::log("trace", "connectToServer(): socketInitialize succeeded.");

	// Create a SOCKET for connecting to server
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		logger::log(
				"error",
				"connectToServer(): Socket creation failed with error: "
						+ std::to_string(getSocketError()) + ".");
		socketCleanup();
		return (INVALID_SOCKET);
	}
	logger::log("trace", "connectToServer(): Socket created.");

	// look up the host address
	if ((RemoteHost = gethostbyname(hostname.c_str())) == 0) {
		logger::log(
				"error",
				"connectToServer(): Error looking up host address from host "
				"name.");
		socketCleanup();
		return (INVALID_SOCKET);
	}
	logger::log("trace", "connectToServer(): Looked up host address.");

	socket_addr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_port = htons(atoi(serviceport.c_str()));

#ifdef _WIN32
	clientService.sin_addr.s_addr =
	inet_addr(inet_ntoa(*(struct in_addr*)(RemoteHost->h_addr_list[0])));
#else
	memcpy(&clientService.sin_addr, RemoteHost->h_addr, RemoteHost->h_length);
#endif

	if (connect(ClientSocket, reinterpret_cast<socket_addr *>(&clientService),
				sizeof(clientService)) == SOCKET_ERROR) {
		logger::log(
				"error",
				"connectToServer(): Connect failed with error: "
						+ std::to_string(getSocketError()) + ".");
		disconnectFromServer(ClientSocket);
		return (INVALID_SOCKET);
	}
	logger::log("trace", "connectToServer(): Socket Connected.");

	// set up socket options
#ifdef _WIN32
	DWORD timeout = TIMEOUT * 1000;
	SocketResult = setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO,
			reinterpret_cast<char *>(&timeout), sizeof(DWORD));
#else
	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
	SocketResult = setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO,
								(struct timeval *) &timeout,
								sizeof(struct timeval));
#endif

	if (SocketResult == SOCKET_ERROR) {
		logger::log(
				"error",
				"connectToServer(): Failed to set socket options with error: "
						+ std::to_string(getSocketError()) + ".");
	}

	return (ClientSocket);
}

// send data to a connected server
void sendToServer(SOCKET ClientSocket, std::string request) {
	if (ClientSocket == INVALID_SOCKET) {
		logger::log("error", "sendToServer(): Invalid socket.");
		return;
	}

	if (send(ClientSocket, request.c_str(), static_cast<int>(request.length()),
				0) == SOCKET_ERROR) {
		logger::log(
				"error",
				"sendToServer(): Send failed with error: "
						+ std::to_string(getSocketError()) + ".");
		disconnectFromServer(ClientSocket);
	}

	return;
}

// recieve data from a connected server
std::string recieveFromServer(SOCKET ClientSocket) {
	if (ClientSocket == INVALID_SOCKET) {
		logger::log("error", "recieveFromServer(): Invalid socket.");
		return ("");
	}

	int SocketResult;
	std::string response;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// recieve from our socket
	// NOTE TIMEOUT?!?
	SocketResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

	// done with socket
	if (SocketResult > 0) {
		// nullterminate the response after the eor
		for (int i = 0; i < recvbuflen; i++) {
			// look for the end of the "<EOR>"
			if (recvbuf[i] == '>') {
				// make sure we're at the "<EOR>"
				if ((recvbuf[i - 1] != 'R') && (recvbuf[i - 2] != 'O')
						&& (recvbuf[i - 3] != 'E') && (recvbuf[i - 4] != '<'))
					continue;

				// null terminate and we;re done
				recvbuf[i + 1] = 0;
				break;
			}
		}

		// convert the response to something modern
		response = recvbuf;
		logger::log("trace",
					"recieveFromServer(): Recieved: " + response + " .");

		return (response);
	} else if (SocketResult == 0) {
		logger::log("warning",
					"recieveFromServer(): Null result, Closing socket.");
		disconnectFromServer(ClientSocket);
		return ("");
	} else {
		int socketerror = getSocketError();
		if (socketerror == EAGAIN) {
			logger::log(
					"error",
					"recieveFromServer(): recv timed out (EAGAIN) waiting for "
					"response from server, Closing Socket.");
		} else {
			logger::log(
					"error",
					"recieveFromServer(): recv failed with error: "
							+ std::to_string(socketerror)
							+ ", Closing Socket.");
		}
		disconnectFromServer(ClientSocket);
		return ("");
	}

	// should never get here
	return ("");
}

// disconnect from a connected server
int disconnectFromServer(SOCKET ClientSocket) {
	int status = 0;

	if (ClientSocket != INVALID_SOCKET) {
#ifdef _WIN32
		status = shutdown(ClientSocket, SD_BOTH);
		if (status == 0) {
			status = closesocket(ClientSocket);
		}
#else
		status = shutdown(ClientSocket, SHUT_RDWR);
		if (status == 0) {
			status = close(ClientSocket);
		}
#endif
	}
	if (status == 0)
		socketCleanup();

	return (status);
}

// initalize sockets
int socketInitialize() {
#ifdef _WIN32
	WSADATA wsaData;

	// Initialize Winsock
	return(WSAStartup(MAKEWORD(2, 2), &wsaData));
#else
	return (0);
#endif
}

// cleanup sockets
int socketCleanup() {
#ifdef _WIN32
	// Cleanup Winsock
	return(WSACleanup());
#else
	return (0);
#endif
}

// get socket errors
int getSocketError() {
#ifdef _WIN32
	// Cleanup get socket error
	return(WSAGetLastError());
#else
	return (errno);
#endif
}
}  // namespace util
