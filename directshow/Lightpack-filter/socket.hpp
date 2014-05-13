#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>

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
        mLog = new Log("socket.txt");
    }

    ~Socket() {
        mLog->flush();
        delete mLog;
        Close();
        WSACleanup();
    }

    bool Open(PCSTR hostname, PCSTR port) {
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

        // Resolve the server address and port
        iResult = getaddrinfo(hostname, port, &hints, &result);
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
        return true;
    }

    void Close() {
        if (mConnectSocket != INVALID_SOCKET) {
            closesocket(mConnectSocket);
            mConnectSocket = INVALID_SOCKET;
        }
    }

    // Not safe for large character buffers
    bool Receive(char* buffer, int timeout = 0) {
        if (mConnectSocket == INVALID_SOCKET) {
            return false;
        }

        // Set the timeout
        if (timeout != mRecvTimeout) {
            mRecvTimeout = timeout;
            setsockopt(mConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
        }
        int iResult;

        iResult = recv(mConnectSocket, buffer, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            int n = -1;
            for (int i = 0; i < DEFAULT_BUFLEN; i++) {
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
            return true;
        }
        else if (iResult == 0) {
            buffer = '\0';
            logf("Connection closed\n");
            return false;
        }
        else {
            buffer = '\0';
            logf("recv failed with error: %d\n", WSAGetLastError());
            return false;
        }
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

    Log* mLog;
};



#endif // __SOCKET_HPP__