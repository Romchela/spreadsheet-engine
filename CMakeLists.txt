cmake_minimum_required(VERSION 3.15)
project(SpreadsheetEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(SpreadsheetEngine io-data.h engine.cpp reader.h reader.cpp utils.h solutions/solution.h solutions/one-thread-simple.h solutions/one-thread-simple.cpp writer.h writer.cpp)