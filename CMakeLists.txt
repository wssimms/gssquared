cmake_minimum_required(VERSION 3.10)

# Set the project name
project(gssquared)

# Mac-specific settings
if(APPLE)
    set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX15.2.sdk")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.2")

    # Mac-specific bundle configuration
    set(MACOSX_BUNDLE_ICON_FILE gs2.icns)
    set(APP_ICON_MACOSX ${CMAKE_SOURCE_DIR}/assets/img/gs2.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.bazyar.gs2")
    set(MACOSX_BUNDLE_BUNDLE_NAME "GSSquared")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 Jawaid Bazyar")
    
    # Custom target for creating Mac app bundle
    add_custom_target(mac-package
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/GSSquared.app/Contents/MacOS
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/GSSquared.app/Contents/Resources
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gs2> ${CMAKE_BINARY_DIR}/GSSquared.app/Contents/MacOS/
        COMMAND ${CMAKE_COMMAND} -E copy ${APP_ICON_MACOSX} ${CMAKE_BINARY_DIR}/GSSquared.app/Contents/Resources/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/resources/Info.plist ${CMAKE_BINARY_DIR}/GSSquared.app/Contents/
        DEPENDS gs2
    )
endif()

# Add SDL as a subdirectory and configure it
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
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

add_library(gs2_devices_prodos_block src/devices/prodos_block/prodos_block.cpp )

add_library(gs2_devices_game src/devices/game/gamecontroller.cpp )

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

add_library(gs2_util src/util/media.cpp src/util/ResourceFile.cpp src/util/dialog.cpp )

# Add the executable
add_executable(gs2 src/gs2.cpp src/bus.cpp src/clock.cpp src/debug.cpp src/cpu.cpp src/memory.cpp src/opcodes.cpp src/test.cpp 
    src/display/text_40x24.cpp src/display/lores_40x48.cpp src/display/hgr_280x192.cpp src/display/display.cpp
    src/devices/loader.cpp 
    src/devices/diskii.cpp
    src/platforms.cpp
    src/event_poll.cpp)

# Set up common include directories for all targets
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    /usr/local/include
)

# Link SDL3 and required frameworks
target_link_libraries(gs2 PRIVATE 
    SDL3::SDL3-shared
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
    gs2_devices_game
    gs2_util
)

add_subdirectory(apps/nibblizer)

add_subdirectory(apps/diskid)

# Update the executable's include directories to remove redundant paths
target_include_directories(gs2 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
