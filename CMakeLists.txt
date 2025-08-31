cmake_minimum_required(VERSION 4.0)
project(excserial)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


message("Compiler = ${CMAKE_CXX_COMPILER_ID}")
message("Compiler FE = ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")

# Check if Clang-cl
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        message("Using Clang-cl, setting flags...")
    add_compile_options("/EHc")
endif()

add_executable(excserial main.cpp)

install(
        TARGETS excserial
        DESTINATION bin
)
