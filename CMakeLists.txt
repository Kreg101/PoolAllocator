cmake_minimum_required(VERSION 3.23)
project(poolAllocator)

set(CMAKE_CXX_STANDARD 17)

add_executable(poolAllocator bin/main.cpp allocator/pools_allocator.h)
