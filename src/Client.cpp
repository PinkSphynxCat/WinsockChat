#include "header/Client.hpp"
#include <windows.h>

void recvOnce(SOCKET connectSocket);

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    int iResult;

    SOCKET ConnectSocket = INVALID_SOCKET;

    struct addrinfo hints;
    struct addrinfo* ptr;
    struct addrinfo* result;

    char* hostname = argv[1];

    std::thread recvThread;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0)
    {
        printf("WSAStartup failed: %d", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(hostname, DEFAULT_PORT, &hints, &result);
    if(iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    for(ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(ConnectSocket == INVALID_SOCKET)
        {
            printf("socket failed: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if(iResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if(ConnectSocket == INVALID_SOCKET)
    {
        printf("connect failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    recvThread = std::thread(recvMessageLoop, ConnectSocket);
    recvThread.detach();
    sendMessageLoop(ConnectSocket);

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

void sendMessageLoop(SOCKET connectSocket)
{
    while(true)
    {
        std::string userInput;
        std::getline(std::cin, userInput);

        int userInputLength = userInput.size();
        SEND_ERROR sendError = sendWholeMessageLength(connectSocket, userInputLength);
        if(sendError == SEND_MSG_SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                break;
            }
            else
            {
                printf("accept is fine: WSAEWOULDBLOCK\n");
            }
        }

        sendWholeMessage(connectSocket, userInputLength, userInput);
        if(sendError == SEND_MSG_SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                break;
            }
            else
            {
                printf("accept is fine: WSAEWOULDBLOCK\n");
            }
        }

        if(userInput == "$QUIT")
        {
            int iResult = shutdown(connectSocket, SD_BOTH);
            break;
        }
    }
}

void recvMessageLoop(SOCKET connectSocket)
{
    while(true)
    {
        int messageLength = 0;
        std::string message = "";

        //printf("[CLIENT] recvWholeMessageLength BEGIN\n");
        RECV_ERROR recvError = recvWholeMessageLength(connectSocket, messageLength);
        //printf("[CLIENT] recvWholeMessageLength END\n");
        if(recvError == RECV_CLOSE_SOCKET)
        {
            printf("Connection closed.\n");
            closesocket(connectSocket);
        }
        else if(recvError == RECV_MSG_SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("recv failed: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                break;
            }
            else
            {
                printf("accept is fine: WSAEWOULDBLOCK\n");
            }
        }
        //printf("[CLIENT] %d\n", messageLength);
        
        
        //printf("[CLIENT] recvWholeMessage BEGIN\n");
        recvError = recvWholeMessage(connectSocket, messageLength, message);
        //printf("[CLIENT] recvWholeMessage END\n");
        if(recvError == RECV_CLOSE_SOCKET)
        {
            printf("Connection closed.\n");
            closesocket(connectSocket);
        }
        else if(recvError == RECV_MSG_SOCKET_ERROR)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("recv failed: %d\n", WSAGetLastError());
                closesocket(connectSocket);
                WSACleanup();
                break;
            }
            else
            {
                printf("accept is fine: WSAEWOULDBLOCK\n");
            }
        }
        
        printf("[CLIENT] %s\n", message.c_str());
    }
}