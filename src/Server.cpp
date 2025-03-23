#include "header/Server.hpp"
#include <vector>

int main(int argc, char* argv[])
{
    WSAData wasData;
    int iResult, iSendResult, iRecvResult;

    int totalSocket = 0;
    SOCKET ListenSocket = INVALID_SOCKET;
    std::vector<SOCKET> socketVector(FD_SETSIZE);

    struct addrinfo hints;
    struct addrinfo* result = NULL;

    TIMEVAL timeout;
    FD_SET readSet;

    ULONG nonblocking = 1;

    iResult = WSAStartup(MAKEWORD(2, 2), &wasData);
    if(iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        std::cin.get(); return 1;
    }

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
        std::cin.get(); return 1;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(ListenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        std::cin.get(); return 1;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if(iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        std::cin.get(); return 1;
    }

    freeaddrinfo(result);

    if(listen(ListenSocket, 5) == SOCKET_ERROR)
    {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        std::cin.get(); return 1;
    }
    
    iResult = ioctlsocket(ListenSocket, FIONBIO, &nonblocking);
    if(iResult == SOCKET_ERROR)
    {
        printf("ioctlsocket failed: %ld", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        std::cin.get(); return 1;
    }
    
    timeout.tv_sec = 3 * 60;
    timeout.tv_usec = 0;
    while(true)
    {
        FD_ZERO(&readSet);
        FD_SET(ListenSocket, &readSet);

        for (int i = 0; i < totalSocket; ++i)
        {
            FD_SET(socketVector[i], &readSet);
        }

        int Total = select(0, &readSet, NULL, NULL, &timeout);

        if(Total < 0)
        {
            printf("Select failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            std::cin.get(); return 1;
        }
        else if(Total == 0)
        {
            printf("Select timeout.\n");
            break;
        }

        if(FD_ISSET(ListenSocket, &readSet))
        {
            printf("Accept new connection BEGIN\n");
            
            Total--;
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            if(ClientSocket == INVALID_SOCKET)
            {
                if(WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("accept failed: %d\n", WSAGetLastError());
                    closesocket(ListenSocket);
                    WSACleanup();
                    std::cin.get(); return 1;
                }
                else
                {
                    printf("accept is fine: WSAEWOULDBLOCK\n");
                }
            }

            iResult = ioctlsocket(ClientSocket, FIONBIO, &nonblocking);
            if(iResult == SOCKET_ERROR)
            {
                printf("ioctlsocket failed: %ld", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                std::cin.get(); return 1;
            }
            
            socketVector[totalSocket] = ClientSocket;
            totalSocket++;

            std::string welcomMessage = "Welcome to server!\r\n";
            int welcomMessageLength = welcomMessage.size();

            SEND_ERROR sendError = sendWholeMessageLength(ClientSocket, welcomMessageLength);
            if(sendError == SEND_MSG_SOCKET_ERROR)
            {
                if(WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("send failed: %d\n", WSAGetLastError());
                    socketVector.pop_back();
                    totalSocket--;
                    closesocket(ClientSocket);
                    WSACleanup();
                    std::cin.get(); return 1;
                }
                else
                {
                    printf("Send with WSAEWOULDBLOCK error.\n");
                }
            }

            // SOCKET ERROR OCCUR HERE when compile in RELEASE mode
            sendError = sendWholeMessage(ClientSocket, welcomMessageLength, welcomMessage);
            if(sendError == SEND_MSG_SOCKET_ERROR)
            {
                if(WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("send failed: %d\n", WSAGetLastError());
                    socketVector.pop_back();
                    totalSocket--;
                    closesocket(ClientSocket);
                    WSACleanup();
                    std::cin.get(); return 1;
                }
                else
                {
                    printf("Send with WSAEWOULDBLOCK error.\n");
                }
            }
            
            printf("Accept new connection END\n");
            
        
        }

        for (int i = 0; Total > 0 && i < totalSocket; i++)
        {
            SOCKET currentSocket = socketVector[i];
            if(FD_ISSET(currentSocket, &readSet))
            {
                Total--;
                printf("boardcast part...\n");

                int recvMessageLength = 0;
                RECV_ERROR recvError = recvWholeMessageLength(currentSocket, recvMessageLength);
                if(recvError == RECV_MSG_NO_ERROR)
                {
                    std::string recvMessage = "";
                    recvError = recvWholeMessage(currentSocket, recvMessageLength, recvMessage);
                    if(recvError == RECV_MSG_NO_ERROR)
                    {
                        if(recvMessage == "$QUIT")
                        {
                            socketVector.erase(socketVector.begin() + i);
                            totalSocket--;
                            closesocket(currentSocket);
                            printf("SOCKET #%d quit\n", currentSocket);
                            continue;
                        }

                        for(int i = 0; i < totalSocket; ++i)
                        {   
                            SOCKET outSocket = socketVector[i];
                            // if(outSocket != ListenSocket && outSocket != currentSocket)
                            if(outSocket != ListenSocket)
                            {
                                std::ostringstream ss;
                                if(outSocket != currentSocket)
                                {
                                    ss << "SOCKET #" << currentSocket << ": ";
                                }
                                ss << recvMessage << "\r\n";
                                std::string sendMessage = ss.str();
                                
                                int sendMessageLength = sendMessage.size();
                                SEND_ERROR sendError = sendWholeMessageLength(outSocket, sendMessageLength);
                                if(sendError == SEND_MSG_NO_ERROR)
                                {
                                    sendError = sendWholeMessage(outSocket, sendMessageLength, sendMessage);
                                    if(sendError == SEND_MSG_SOCKET_ERROR)
                                    {
                                        if(WSAGetLastError() != WSAEWOULDBLOCK)
                                        {
                                            printf("send failed: %d\n", WSAGetLastError());
                                            socketVector.erase(socketVector.begin() + i);
                                            totalSocket--;
                                            closesocket(currentSocket);
                                            WSACleanup();
                                        }
                                        else
                                        {
                                            printf("send is fine: WSAEWOULDBLOCK\n");
                                        }
                                    }
                                }
                                else if(sendError == SEND_MSG_SOCKET_ERROR)
                                {
                                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                                    {
                                        printf("send failed: %d\n", WSAGetLastError());
                                        socketVector.erase(socketVector.begin() + i);
                                        totalSocket--;
                                        closesocket(currentSocket);
                                        WSACleanup();
                                    }
                                    else
                                    {
                                        printf("send is fine: WSAEWOULDBLOCK\n");
                                    }
                                }
                            }
                        }
                    }
                    else if(recvError == RECV_CLOSE_SOCKET)
                    {
                        printf("Connection closed.\n");
                        socketVector.erase(socketVector.begin() + i);
                        totalSocket--;
                        closesocket(currentSocket);
                    }
                    else if(recvError == RECV_MSG_SOCKET_ERROR)
                    {
                        if(WSAGetLastError() != WSAEWOULDBLOCK)
                        {
                            printf(">>>recv failed: %d\n", WSAGetLastError());
                            socketVector.erase(socketVector.begin() + i);
                            totalSocket--;
                            closesocket(currentSocket);
                            WSACleanup();
                            std::cin.get(); return 1;
                        }
                        else
                        {
                            printf("recv is fine: WSAEWOULDBLOCK\n");
                        }   
                    }
                }
                else if(recvError == RECV_CLOSE_SOCKET)
                {
                    printf("Connection closed.\n");
                    socketVector.erase(socketVector.begin() + i);
                    totalSocket--;
                    closesocket(currentSocket);
                }
                else if(recvError == RECV_MSG_SOCKET_ERROR)
                {
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        printf("===recv failed: %d\n", WSAGetLastError());
                        socketVector.erase(socketVector.begin() + i);
                        totalSocket--; 
                        closesocket(currentSocket);
                        WSACleanup();
                        std::cin.get(); return 1;
                    }
                    else
                    {
                        printf("recv is fine: WSAEWOULDBLOCK\n");
                    }   
                }
            }
        }
    }
    
    for(int i = 0; i < totalSocket; ++i)
    {
        closesocket(socketVector[i]);
    }
    socketVector.clear();

    closesocket(ListenSocket);
    WSACleanup();

    std::cin.get(); return 0;
}
