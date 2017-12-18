project(clogger)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(clogger SHARED src/clogger.cpp)
set_target_properties(clogger PROPERTIES POSITION_INDEPENDENT_CODE ON)
