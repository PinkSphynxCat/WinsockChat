#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <string>

#define DEFAULT_PORT "50000"
#define DEFAULT_BUFLEN 512

enum SEND_ERROR
{
    SEND_MSG_NO_ERROR = 0,
    SEND_INVALID_MSG_LEN,
    SEND_MSG_SOCKET_ERROR
};

enum RECV_ERROR
{
    RECV_MSG_NO_ERROR = 0,
    RECV_CLOSE_SOCKET,
    RECV_INVALID_MSG_LEN,
    RECV_MSG_SOCKET_ERROR,
};

SEND_ERROR sendWholeMessageLength(SOCKET targetSocket, int snedMessageLength);
SEND_ERROR sendWholeMessage(SOCKET targetSocket, int snedMessageLength, std::string sendMessage);

RECV_ERROR recvWholeMessageLength(SOCKET targetSocket, int& recvMessageLength);
RECV_ERROR recvWholeMessage(SOCKET targetSocket, int recvMessageLength, std::string& recvMessage);