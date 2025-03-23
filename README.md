---
title: Winsock Chat

---

# Winsock Chat

### Currently, it only works in Debug mode
---

## Intro
### Server
In server.cpp
1. Startup Winsock2
2. Create socket with address info settings
3. Bind and listing socket
4. Use **select()** for accept client sockets and boardcast message to others
5. Close sockets and cleanup Winsock2

### Client
In client.cpp
1. Startup Winsock2
2. Create address info settings with hostname (default: 127.0.0.1/5000)
3. Create socket and connect to server
4. Create detached recv thread for recv server accept message and other clients boardcast messages
5. Send stdin string in main thread

### Socket
In socket.cpp
1. **sendWholeMessageLength()** send message length first and then **sendWholeMessage()** send message itself
2. **recvWholeMessageLength()** recv message length and use it for **recvWholeMessage()** to check how may message should be received with certain length
3. These functions used in server.cpp and client.cpp

## Issue
- In **Release** mode
    - When sever socket accept client, it cannot correctly send welcome message to client and will trigger recv error
    - when welcome message received successfully by client, the issue may occur when client try to boardcast its sended messages
    - (Following image: **Server** on left, **Client** on right)
    -  ![winsockchat](https://hackmd.io/_uploads/HksyQNankl.png)


## Environment
- OS: windows 11
- Compiler: MYSY2 GCC 14.2.0
- IDE: Visual Studio Code
    - C/C++ extensions
    - CMake extensions