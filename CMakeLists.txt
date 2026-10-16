cmake_minimum_required(VERSION 3.16)
project(Tmap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(Tmap main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(Tmap PRIVATE Threads::Threads)

target_compile_options(Tmap PRIVATE -Wall -Wextra)

target_link_options(Tmap PRIVATE -static-libstdc++ -static-libgcc)