cmake_minimum_required(VERSION 3.3)
project(vaje10-urejanje-stevil)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES mpi-urejanje-stevil.cpp)
add_executable(mpi-urejanje-stevil ${SOURCE_FILES})
