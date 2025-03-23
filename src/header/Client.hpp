#include "Socket.hpp"
#include <thread>

void sendMessageLoop(SOCKET connectSocket);
void recvMessageLoop(SOCKET connectSocket);