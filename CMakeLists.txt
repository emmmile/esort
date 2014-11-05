cmake_minimum_required(VERSION 2.8)
#set(CMAKE_CXX_COMPILER g++-4.9)
project(esort)

add_definitions(-std=c++11)
set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -mtune=native -funroll-loops" )
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")

include_directories("${PROJECT_SOURCE_DIR}/timer")
add_subdirectory("${PROJECT_SOURCE_DIR}/timer")
link_directories("${PROJECT_SOURCE_DIR}/timer")

add_executable(main main.cpp)
target_link_libraries(main boost_filesystem boost_serialization boost_system timer)

#enable_testing()
add_test(test test)
