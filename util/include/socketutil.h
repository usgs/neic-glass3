/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SOCKETUTIL_H
#define SOCKETUTIL_H

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>  // note winsock2 MUST be declared before windows.h
#include <windows.h>
typedef SOCKADDR socket_addr;
typedef SOCKADDR_IN socket_addr_in;
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef sockaddr socket_addr;
typedef sockaddr_in socket_addr_in;
#endif

#include <string>

#define DEFAULT_BUFLEN 4096
#define TIMEOUT 20

namespace util {
/**
 * \brief Connect to a server
 *
 * Opens a socket connection to a server at the given host address and port.
 * \param hostname - A std::string containing host address to connect to
 * \param serviceport - A std::string containing the host port to connect to.
 * \return returns a SOCKET handle if successful, SOCKET_ERROR if not.
 */
SOCKET connectToServer(std::string hostname, std::string serviceport);

/**
 * \brief Send data to a connected server
 *
 * Sends a given message to a server using a given socket
 * \param ClientSocket - A SOCKET handle to the server to send to.
 * \param message - A std::string containing the message to send.
 */
void sendToServer(SOCKET ClientSocket, std::string message);

/**
 * \brief Recieve data from a connected server
 *
 * Recieves a message from a server using a given socket
 * \param ClientSocket - A SOCKET handle to the server to recieve from.
 * \return returns a std::string containing the recieved message.
 */
std::string recieveFromServer(SOCKET ClientSocket);

/**
 * \brief Disconnect from a connected server
 *
 * Disconnect from a server using a given socket
 * \param ClientSocket - A SOCKET handle to the server to disconnect.
 * \return returns a integer value containing the disconnect status.
 */
int disconnectFromServer(SOCKET ClientSocket);

/**
 * \brief Initialize sockets
 *
 * Initialize the socket system
 * \return returns a integer value containing the initialize status.
 */
int socketInitialize();

/**
 * \brief Cleanup sockets
 *
 * Clean up the socket system
 * \return returns a integer value containing the clean up status.
 */
int socketCleanup();

/**
 * \brief Get Socket Error
 *
 * Get the last socket error
 * \return returns a integer value containing the last socket error.
 */
int getSocketError();
}  // namespace util
#endif  // SOCKETUTIL_H
