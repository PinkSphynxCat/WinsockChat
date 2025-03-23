#include "header/Socket.hpp"

SEND_ERROR sendWholeMessageLength(SOCKET targetSocket, int sendMessageLength)
{
    if(sendMessageLength < 0 || sendMessageLength > DEFAULT_BUFLEN)
    {
        return SEND_INVALID_MSG_LEN;
    }
    
    int iSendResult;
    int bytesSent;
    int bytesRemain;

    int32_t messageLength = htonl(sendMessageLength);
    char* messageLengthBuffer = (char*)&messageLength;

    iSendResult = 0;
    bytesSent = 0;
    bytesRemain = sizeof(messageLength) + 1;
    while(bytesRemain > 0)
    {
        iSendResult = send(targetSocket, messageLengthBuffer + bytesSent, bytesRemain, 0);
        if(iSendResult == SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                return SEND_MSG_SOCKET_ERROR;
            }   
        }
        bytesSent += iSendResult;
        bytesRemain -= bytesRemain;
    }
    return SEND_MSG_NO_ERROR;
}

SEND_ERROR sendWholeMessage(SOCKET targetSocket, int snedMessageLength, std::string sendMessage)
{
    int iSendResult;
    int bytesSent;
    int bytesRemain;

    int messageLength = snedMessageLength;
    const char* messageBuffer = sendMessage.c_str();

    if(messageLength < 0 || messageLength > DEFAULT_BUFLEN)
    {
        return SEND_INVALID_MSG_LEN;
    }

    iSendResult = 0;
    bytesSent = 0;
    bytesRemain = messageLength + 1;
    
    while(bytesRemain > 0)
    {
        iSendResult = send(targetSocket, messageBuffer + bytesSent, bytesRemain, 0);
        if(iSendResult == SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                return SEND_MSG_SOCKET_ERROR;
            }   
        }
        bytesSent += iSendResult;
        bytesRemain -= bytesRemain;
    }
    return SEND_MSG_NO_ERROR;
}

RECV_ERROR recvWholeMessageLength(SOCKET targetSocket, int& recvMessageLength)
{
    int iRecvResult;
    int bytesRecv;
    int bytesRemain;

    int32_t messageLength = 0;
    char* messageLengthBuffer = (char*)&messageLength;

    bytesRecv = 0;
    bytesRemain = sizeof(messageLength) + 1;
    while(bytesRemain > 0)
    {
        iRecvResult = recv(targetSocket, messageLengthBuffer + bytesRecv, bytesRemain, 0);
        if(iRecvResult == SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                return RECV_MSG_SOCKET_ERROR;
            }   
        }
        else if(iRecvResult == 0)
        {
            return RECV_CLOSE_SOCKET;
        }
        
        bytesRecv += iRecvResult;
        bytesRemain -= iRecvResult;
    }
    
    recvMessageLength = ntohl(messageLength);
    if(recvMessageLength < 0 || recvMessageLength > DEFAULT_BUFLEN)
    {
        return RECV_INVALID_MSG_LEN;
    }

    return RECV_MSG_NO_ERROR;
}

RECV_ERROR recvWholeMessage(SOCKET targetSocket, int recvMessageLength, std::string& recvMessage)
{
    if(recvMessageLength < 0 || recvMessageLength > DEFAULT_BUFLEN)
    {
        return RECV_INVALID_MSG_LEN;
    }

    int iRecvResult;
    int bytesRecv;
    int bytesRemain;
    
    char messageBuffer[DEFAULT_BUFLEN];
    ZeroMemory(messageBuffer, DEFAULT_BUFLEN);

    bytesRecv = 0;
    bytesRemain = recvMessageLength + 1;

    while(bytesRemain > 0)
    {
        iRecvResult = recv(targetSocket, messageBuffer + bytesRecv, bytesRemain, 0);
        if(iRecvResult == SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                return RECV_MSG_SOCKET_ERROR;
            }   
        }
        else if(iRecvResult == 0)
        {
            return RECV_CLOSE_SOCKET;
        }
        bytesRecv += iRecvResult;
        bytesRemain -= iRecvResult;
    }

    recvMessage = std::string(messageBuffer, bytesRecv);

    return RECV_MSG_NO_ERROR;
}
