#include "header/Server.hpp"
#include <vector>

constexpr int BACKLOG = 5;
constexpr long SELECT_TIMEOUT_SECONDS = 3 * 60;

int main()
{
    WSAData wasData;
    int iResult;

    SOCKET listenSocket = INVALID_SOCKET;
    std::vector<SOCKET> clientSockets;

    struct addrinfo hints;
    struct addrinfo* result = nullptr;

    TIMEVAL timeout;
    FD_SET readSet;

    ULONG nonblocking = 1;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wasData);
    if(iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Set up the hints for getaddrinfo
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if(iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create the listening socket
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(listenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if(iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    // Start listening on the socket
    if(listen(listenSocket, BACKLOG) == SOCKET_ERROR)
    {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    // Set the listening socket on non-blocking mode
    iResult = ioctlsocket(listenSocket, FIONBIO, &nonblocking);
    if(iResult == SOCKET_ERROR)
    {
        printf("ioctlsocket failed: %d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    // Set the timeout for select
    timeout.tv_sec = SELECT_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    // Select loop for listening socket and client sockets
    // Check if any socket is ready for read
    while(true)
    {
        // Set listening socket and client sockets to read set
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);
        for (auto socket: clientSockets)
            FD_SET(socket, &readSet);
        
        int numReadySockets = select(0, &readSet, nullptr, nullptr, &timeout);
        if(numReadySockets < 0)
        {
            printf("Select failed: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            for(auto socket: clientSockets)
                closesocket(socket);
            WSACleanup();
            return 1;
        }
        else if(numReadySockets == 0)
        {
            printf("Select timeout.\n");
            break;
        }

        // Check if there is new connection on the listening socket
        if(FD_ISSET(listenSocket, &readSet))
        {
            printf("Accept new connection\n");
            
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if(clientSocket == INVALID_SOCKET)
            {
                if(WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("accept failed: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    for(auto socket: clientSockets)
                        closesocket(socket);
                    WSACleanup();
                    return 1;
                }
                else
                {
                    printf("accept is fine: WSAEWOULDBLOCK\n");
                }
            }
            else
            {
                // Set the new client socket to non-blocking mode
                iResult = ioctlsocket(clientSocket, FIONBIO, &nonblocking);
                if(iResult == SOCKET_ERROR)
                {
                    printf("ioctlsocket failed: %d", WSAGetLastError());
                    closesocket(clientSocket);
                }
                else
                {
                    // Update client socket list
                    clientSockets.push_back(clientSocket);
                    
                    // Send welcome message to client
                    std::string welcomMessage = "Welcome to server!\r\n";
                    int welcomMessageLength = welcomMessage.size();
                    SEND_ERROR sendError = sendWholeMessageLength(clientSocket, welcomMessageLength);
                    if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        printf("send failed: %d\n", WSAGetLastError());
                        closesocket(clientSocket);
                        clientSockets.pop_back();
                    }
                    else
                    {
                        sendError = sendWholeMessage(clientSocket, welcomMessageLength, welcomMessage);
                        if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                        {
                            printf("send failed: %d\n", WSAGetLastError());
                            closesocket(clientSocket);
                            clientSockets.pop_back();
                        }
                    }
                }
            }
        }

        // Iterate from end for safely remove sockets
        for (int i = clientSockets.size() - 1; i >= 0; i--)
        {
            SOCKET currentSocket = clientSockets[i];
            if(FD_ISSET(currentSocket, &readSet))
            {
                printf("boardcast part...\n");

                int recvMessageLength = 0;
                RECV_ERROR recvError = recvWholeMessageLength(currentSocket, recvMessageLength);
                if(recvError == RECV_MSG_NO_ERROR)
                {
                    std::string recvMessage = "";
                    recvError = recvWholeMessage(currentSocket, recvMessageLength, recvMessage);
                    if(recvError == RECV_MSG_NO_ERROR)
                    {
                        // Check if client wnats to quit chatting
                        if(recvMessage == "$QUIT")
                        {
                            printf("SOCKET #%lld quit\n", currentSocket);
                            closesocket(currentSocket);
                            clientSockets.erase(clientSockets.begin() + i);
                            continue;
                        }

                        // Broadcast message to all other connected clients
                        for(int j = clientSockets.size() - 1; j >= 0; j--)
                        {   
                            SOCKET outSocket = clientSockets[j];
                            if(outSocket != listenSocket)
                            {
                                std::ostringstream ss;
                                if(outSocket != currentSocket)
                                    ss << "SOCKET #" << currentSocket << ": ";
                                ss << recvMessage << "\r\n";
                                std::string sendMessage = ss.str();
                                
                                int sendMessageLength = sendMessage.size();
                                SEND_ERROR sendError = sendWholeMessageLength(outSocket, sendMessageLength);
                                if(sendError == SEND_MSG_NO_ERROR)
                                {
                                    sendError = sendWholeMessage(outSocket, sendMessageLength, sendMessage);
                                    if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                                    {
                                        printf("send failed: %d\n", WSAGetLastError());
                                        closesocket(currentSocket);
                                        clientSockets.erase(clientSockets.begin() + j);
                                        i = clientSockets.size();
                                    }
                                }
                                else if(sendError == SEND_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                                {
                                    printf("send failed: %d\n", WSAGetLastError());
                                    closesocket(currentSocket);
                                    clientSockets.erase(clientSockets.begin() + j);
                                    i = clientSockets.size();
                                }
                            }
                        }
                    }
                    else if(recvError == RECV_CLOSE_SOCKET)
                    {
                        printf("Connection closed.\n");
                        closesocket(currentSocket);
                        clientSockets.erase(clientSockets.begin() + i);
                    }
                    else if(recvError == RECV_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        printf(">>>recv failed: %d\n", WSAGetLastError());
                        closesocket(currentSocket);
                        clientSockets.erase(clientSockets.begin() + i);
                    }
                }
                else if(recvError == RECV_CLOSE_SOCKET)
                {
                    printf("Connection closed.\n");
                    closesocket(currentSocket);
                    clientSockets.erase(clientSockets.begin() + i);
                }
                else if(recvError == RECV_MSG_SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("===recv failed: %d\n", WSAGetLastError());
                    closesocket(currentSocket);
                    clientSockets.erase(clientSockets.begin() + i);
                }
            }
        }
    }
    
    // Close all sockets
    for(auto clientSocket: clientSockets)
        closesocket(clientSocket);
    clientSockets.clear();
    closesocket(listenSocket);

    // Clean up Winsock
    WSACleanup();

    return 0;
}
