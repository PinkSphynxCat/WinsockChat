cmake_minimum_required(VERSION 3.10.0)
project(Chatroom VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_BUILD_TYPE})

set(SRC_PATH ${CMAKE_CURRENT_LIST_DIR}/src)

set(SERVER_SRC ${SRC_PATH}/Server.cpp ${SRC_PATH}/Socket.cpp)
set(SERVER_LIB -lws2_32)
set(CLIENT_SRC ${SRC_PATH}/Client.cpp ${SRC_PATH}/Socket.cpp)
set(CLIENT_LIB -lws2_32 -lmswsock -ladvapi32 -lpthread)

add_executable(Server ${SERVER_SRC})
target_link_libraries(Server ${SERVER_LIB})
if(MSVC)
  target_compile_options(Server PRIVATE /W4 /WX)
else()
  target_compile_options(Server PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()


add_executable(Client ${CLIENT_SRC})
target_link_libraries(Client ${CLIENT_LIB})
if(MSVC)
  target_compile_options(Client PRIVATE /W4 /WX)
else()
  target_compile_options(Client PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()