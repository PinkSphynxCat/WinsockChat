#include "header/Client.hpp"
#include <windows.h>

int main()
{    
    WSADATA wsaData;
    int iResult;

    SOCKET connectSocket = INVALID_SOCKET;

    struct addrinfo hints;
    struct addrinfo* ptr;
    struct addrinfo* result;

    const char* hostname = "127.0.0.1";

    std::thread recvThread;

    // Start up Winsocket
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0)
    {
        printf("WSAStartup failed: %d", iResult);
        return 1;
    }

    // Set up hints for getaddrinfo
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

    // Iterate the getaddrinfo result and try connecting server
    for(ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(connectSocket == INVALID_SOCKET)
        {
            printf("socket failed: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if(iResult == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if(connectSocket == INVALID_SOCKET)
    {
        printf("connect failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    recvThread = std::thread(recvMessageLoop, connectSocket);
    recvThread.detach();
    sendMessageLoop(connectSocket);

    closesocket(connectSocket);
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
        if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("send failed: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }

        sendWholeMessage(connectSocket, userInputLength, userInput);
        if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("send failed: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }
    
        if(userInput == "$QUIT")
        {
            shutdown(connectSocket, SD_BOTH);
            break;
        }
    }
}

void recvMessageLoop(SOCKET connectSocket)
{
    while(true)
    {
        int messageLength = 0;
        std::string message;

        RECV_ERROR recvError = recvWholeMessageLength(connectSocket, messageLength);
        if(recvError == RECV_CLOSE_SOCKET)
        {
            printf("Connection closed.\n");
            closesocket(connectSocket);
        }
        else if(recvError == RECV_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) 
        {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }
        
        recvError = recvWholeMessage(connectSocket, messageLength, message);
        if(recvError == RECV_CLOSE_SOCKET)
        {
            printf("Connection closed.\n");
            closesocket(connectSocket);
        }
        else if(recvError == RECV_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }
        
        printf("[CLIENT] %s\n", message.c_str());
    }
}