cmake_minimum_required(VERSION 3.6)
project(chatSercer2_0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

link_libraries(ws2_32 wsock32)

set(SOURCE_FILES main.cpp)
add_executable(chatSercer2_0 ${SOURCE_FILES})