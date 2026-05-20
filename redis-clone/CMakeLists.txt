cmake_minimum_required(VERSION 3.10)
project(redis-clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Release)

include_directories(src)
file(GLOB SOURCES "src/*.cpp")

add_executable(server ${SOURCES})
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
