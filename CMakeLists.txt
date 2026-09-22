cmake_minimum_required(VERSION 3.20)

project(LimitOrderBook LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lob_test lob.cpp LimitOrderBook.cpp)

target_compile_options(lob_test PRIVATE -Wall -Wextra -Wpedantic)