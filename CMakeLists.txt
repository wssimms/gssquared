cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)  # This is the important one for .a files

# Set the project name
project(gssquared)

#message(STATUS "Runtime output dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
#message(STATUS "Library output dir: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
#message(STATUS "Archive output dir: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
#message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the compiler to Clang++
set(CMAKE_CXX_COMPILER clang++)

# Choose one of these optimization levels:
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")  # No optimization
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")  # Basic optimization
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")  # Moderate optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")  # Aggressive optimization
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")  # Optimize for size

# Find SDL2 package
find_package(SDL2 REQUIRED)

add_library(gs2_devices_diskii
    src/devices/diskii/diskii_fmt.cpp
    src/devices/diskii/diskii_fmt.cpp
)

add_library(gs2_devices_tcp
    src/devices/thunderclock_plus/thunderclockplus.cpp
)

add_library(gs2_devices_keyboard
    src/devices/keyboard/keyboard.cpp
)

add_library(gs2_cpu
    src/cpus/cpu_6502.cpp
    src/cpus/cpu_65c02.cpp
)

# Add the executable
add_executable(gs2 gs2.cpp bus.cpp clock.cpp debug.cpp cpu.cpp memory.cpp opcodes.cpp test.cpp 
    display/text_40x24.cpp display/lores_40x48.cpp display/hgr_280x192.cpp display/display.cpp
    devices/speaker.cpp devices/loader.cpp 
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

target_link_libraries(gs2 PUBLIC gs2_cpu gs2_devices_tcp gs2_devices_keyboard gs2_devices_diskii)


add_subdirectory(apps/nibblizer)
