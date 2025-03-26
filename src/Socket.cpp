#include "header/Socket.hpp"

SEND_ERROR sendWholeMessageLength(SOCKET targetSocket, int sendMessageLength)
{
    if(sendMessageLength < 0 || sendMessageLength > DEFAULT_BUFLEN)
    {
        return SEND_INVALID_MSG_LEN;
    }
    
    int totalBytes = sizeof(int32_t) + 1;
    int bytesSent = 0;

    int32_t messageLength = htonl(sendMessageLength);
    char* messageLengthBuffer = (char*)&messageLength;

    while(bytesSent < totalBytes)
    {
        int result = send(targetSocket, messageLengthBuffer + bytesSent, totalBytes - bytesSent, 0);
        
        if(result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            return SEND_MSG_SOCKET_ERROR;
        else if(result == 0)
            return SEND_CLOSE_SOCKET;
        
        bytesSent += result;
    }
    return SEND_MSG_NO_ERROR;
}

SEND_ERROR sendWholeMessage(SOCKET targetSocket, int sendMessageLength, std::string sendMessage)
{
    if(sendMessageLength < 0 || sendMessageLength > DEFAULT_BUFLEN)
    {
        return SEND_INVALID_MSG_LEN;
    }
    int totalBytes = sendMessageLength + 1;
    int bytesSent = 0;
    const char* messageBuffer = sendMessage.c_str();
    
    while(bytesSent < totalBytes)
    {
        int result = send(targetSocket, messageBuffer + bytesSent, totalBytes - bytesSent, 0);
        
        if(result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            return SEND_MSG_SOCKET_ERROR;
        else if(result == 0)
            return SEND_CLOSE_SOCKET;
        
        bytesSent += result;
    }
    return SEND_MSG_NO_ERROR;
}

RECV_ERROR recvWholeMessageLength(SOCKET targetSocket, int& recvMessageLength)
{
    int totalBytes = sizeof(int32_t) + 1;
    int bytesRecv = 0;
    int32_t messageLength = 0;
    char* messageLengthBuffer = (char*)&messageLength;

    while(bytesRecv < totalBytes)
    {
        int result = recv(targetSocket, messageLengthBuffer + bytesRecv, totalBytes - bytesRecv, 0);
        
        if(result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            return RECV_MSG_SOCKET_ERROR;
        else if(result == 0)
            return RECV_CLOSE_SOCKET;
        
        bytesRecv += result;
    }
    
    recvMessageLength = ntohl(messageLength);
    if(recvMessageLength < 0 || recvMessageLength > DEFAULT_BUFLEN)
        return RECV_INVALID_MSG_LEN;

    return RECV_MSG_NO_ERROR;
}

RECV_ERROR recvWholeMessage(SOCKET targetSocket, int recvMessageLength, std::string& recvMessage)
{
    if(recvMessageLength < 0 || recvMessageLength > DEFAULT_BUFLEN)
    {
        return RECV_INVALID_MSG_LEN;
    }
    int totalBytes = recvMessageLength + 1;
    int bytesRecv = 0;    
    char messageBuffer[DEFAULT_BUFLEN] = {0};

    while(bytesRecv < totalBytes)
    {
        int result = recv(targetSocket, messageBuffer + bytesRecv, totalBytes - bytesRecv, 0);
        
        if(result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            return RECV_MSG_SOCKET_ERROR;
        else if(result == 0)
            return RECV_CLOSE_SOCKET;
        
        bytesRecv += result;
    }

    recvMessage.assign(messageBuffer, bytesRecv);

    return RECV_MSG_NO_ERROR;
}
