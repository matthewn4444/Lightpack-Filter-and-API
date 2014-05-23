#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "log.h"

#define logf(...) if (mLog != 0) { mLog->logf(__VA_ARGS__); }
#define log(x) if (mLog != 0) { mLog->log(x); }

#define DEFAULT_BUFLEN 512

class Socket {
public:
    Socket()
        : mRecvTimeout(0)
    {
        InitSocket();
        mLog = 0;
        //mLog = new Log("socket.txt");
    }

    ~Socket() {
        if (mLog) {
            mLog->flush();
            delete mLog;
        }
        Close();
        WSACleanup();
    }

    bool Open(PCSTR hostname, unsigned int port) {
        if (mConnectSocket != INVALID_SOCKET) {
            Close();
        }

        struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;
        int iResult;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Convert port from number to string
        std::stringstream ss;
        ss << port;
        std::string& strPort = ss.str();

        // Resolve the server address and port
        iResult = getaddrinfo(hostname, strPort.c_str(), &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            return false;
        }

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

            // Create a SOCKET for connecting to server
            mConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (mConnectSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                return false;
            }

            // Connect to server.
            iResult = connect(mConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                Close();
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if (mConnectSocket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            return false;
        }
        mPort = port;
        return true;
    }

    void Close() {
        if (mConnectSocket != INVALID_SOCKET) {
            closesocket(mConnectSocket);
            mConnectSocket = INVALID_SOCKET;
            log("CLose socket");
        }
    }

    bool Send(char* message) {
        int length = strlen(message);
        if (mConnectSocket == INVALID_SOCKET || length == 0) {
            return false;
        }

        if (send(mConnectSocket, message, length, 0) == SOCKET_ERROR
            && shutdown(mConnectSocket, SD_SEND) == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            Close();
            WSACleanup();
            return false;
        }
        return true;
    }

    // Not safe for large character buffers
    int Receive(char* buffer, int timeout = 0) {
        if (mConnectSocket == INVALID_SOCKET) {
            return -1;
        }

        // Set the timeout
        if (timeout != mRecvTimeout) {
            mRecvTimeout = timeout;
            setsockopt(mConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
        }
        int iResult;
        int length = sizeof(buffer) > DEFAULT_BUFLEN ? sizeof(buffer) : DEFAULT_BUFLEN;

        iResult = recv(mConnectSocket, buffer, length, 0);
        if (iResult > 0) {
            int n = -1;
            for (int i = 0; i < length; i++) {
                if (buffer[i] == '\n') {
                    n = i;
                    break;
                }
            }
            if (n != -1) {
                // Remove \r if exists
                if ((n - 1) >= 0 && buffer[n - 1] == '\r') {
                    n--;
                }
                buffer[n] = '\0';
            }
            logf("Received: %s", buffer);
        }
        else if (iResult == 0) {
            buffer = '\0';
            logf("Connection closed");
        }
        else {
            buffer = '\0';

            // If there was a timeout and that was the error, then ignore it
            if (WSAGetLastError() == 10060 && mRecvTimeout > 0) {
                logf("IGNORE recv timeout error: %d", WSAGetLastError());
                iResult = 1;
            }
            else {
                logf("recv failed with error: %d", WSAGetLastError());
            }
        }
        return iResult;
    }

    inline unsigned int getPort() {
        return mPort;
    }

private:
    bool InitSocket() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        return true;
    }

    // Variables
    SOCKET mConnectSocket = INVALID_SOCKET;
    int mRecvTimeout;
    unsigned int mPort = 0;

    Log* mLog;
};



#endif // __SOCKET_HPP__