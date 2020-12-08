cmake_minimum_required(VERSION 3.10)
project(tinymuduo)

set(CMAKE_CXX_STANDARD 14)
aux_source_directory(muduo/base Base)
aux_source_directory(net Net)

include_directories(./muduo/base)
include_directories(net)
include_directories(.)

link_libraries(pthread)
add_executable(tinymuduo main.cpp ${Base} ${Net})