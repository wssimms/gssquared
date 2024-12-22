cmake_minimum_required(VERSION 3.10)

# Set the project name
project(gssquared)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the compiler to Clang++
set(CMAKE_CXX_COMPILER clang++)

# Add the executable
add_executable(gs2 gs2.cpp bus.cpp clock.cpp debug.cpp cpu.cpp memory.cpp opcodes.cpp test.cpp)
