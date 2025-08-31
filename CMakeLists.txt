cmake_minimum_required(VERSION 4.0)
project(excserial)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(excserial main.cpp)

install(
        TARGETS excserial
        DESTINATION bin
)
