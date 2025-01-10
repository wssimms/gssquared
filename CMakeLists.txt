cmake_minimum_required(VERSION 3.10)

set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX15.2.sdk")
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.2")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)  # This is the important one for .a files

# Set the project name
project(gssquared)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the compiler to Clang++
set(CMAKE_CXX_COMPILER clang++)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Define flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Find SDL2 package
find_package(SDL2 REQUIRED)

# Build prodos block firmware
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/src/devices/prodos_block/pd_block_firmware.hpp
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/devices/prodos_block
    DEPENDS 
        ${CMAKE_SOURCE_DIR}/src/devices/prodos_block/pd_block_firmware.a65
        ${CMAKE_SOURCE_DIR}/src/devices/prodos_block/bin2cpp.py
        ${CMAKE_SOURCE_DIR}/src/devices/prodos_block/Makefile
)

add_custom_target(prodos_block_firmware
    DEPENDS ${CMAKE_SOURCE_DIR}/src/devices/prodos_block/pd_block_firmware.hpp
)

add_library(gs2_devices_prodos_block
    src/devices/prodos_block/prodos_block.cpp
)

add_dependencies(gs2_devices_prodos_block prodos_block_firmware)

target_include_directories(gs2_devices_prodos_block PRIVATE
    ${CMAKE_SOURCE_DIR}/src/devices/prodos_block
)

add_library(gs2_devices_diskii
    src/devices/diskii/diskii_fmt.cpp
    src/devices/diskii/diskii_fmt.cpp
)

add_library(gs2_devices_tcp    src/devices/thunderclock_plus/thunderclockplus.cpp )

add_library(gs2_devices_languagecard     src/devices/languagecard/languagecard.cpp )

add_library(gs2_devices_keyboard     src/devices/keyboard/keyboard.cpp )

add_library(gs2_devices_speaker     src/devices/speaker/speaker.cpp )

add_library(gs2_cpu src/cpus/cpu_6502.cpp src/cpus/cpu_65c02.cpp )

add_library(gs2_util src/util/media.cpp)

# Add the executable
add_executable(gs2 gs2.cpp bus.cpp clock.cpp debug.cpp cpu.cpp memory.cpp opcodes.cpp test.cpp 
    display/text_40x24.cpp display/lores_40x48.cpp display/hgr_280x192.cpp display/display.cpp
    devices/loader.cpp 
    devices/diskii.cpp
    platforms.cpp
    event_poll.cpp)

# Include SDL2 headers
include_directories(${SDL2_INCLUDE_DIRS} 
    /opt/homebrew/Cellar/sdl2/2.30.10/include
    ${CMAKE_SOURCE_DIR}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src    # Makes "src" the root include directory
)

# Link SDL2 and required frameworks
target_link_libraries(gs2 PRIVATE 
    ${SDL2_LIBRARIES}
    "-framework OpenGL" 
    "-framework Cocoa"
)

target_link_libraries(gs2 
    PUBLIC gs2_cpu 
    gs2_devices_tcp 
    gs2_devices_keyboard 
    gs2_devices_diskii 
    gs2_devices_languagecard 
    gs2_devices_prodos_block
    gs2_devices_speaker
    gs2_util
)

add_subdirectory(apps/nibblizer)

add_subdirectory(apps/diskid)
