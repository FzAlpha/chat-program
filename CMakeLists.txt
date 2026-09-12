cmake_minimum_required(VERSION 3.10)
project(TerminalChat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Server executable
add_executable(server src/server.cpp src/common.hpp)
target_link_libraries(server PRIVATE Threads::Threads)

# Client executable
add_executable(client src/client.cpp src/common.hpp)
target_link_libraries(client PRIVATE Threads::Threads)
