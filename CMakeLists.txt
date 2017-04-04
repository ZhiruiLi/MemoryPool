cmake_minimum_required(VERSION 3.6)
project(MemoryPool)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES main.cpp MemoryPool.cpp MemoryPool.h)
add_executable(MemoryPool ${SOURCE_FILES})
