cmake_minimum_required(VERSION 3.10)

# Set the project name
project(gssquared)

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

# Add the executable
add_executable(gs2 gs2.cpp bus.cpp clock.cpp debug.cpp cpu.cpp memory.cpp opcodes.cpp test.cpp 
    display/text_40x24.cpp display/lores_40x48.cpp display/hgr_280x192.cpp display/display.cpp
    devices/keyboard.cpp devices/speaker.cpp devices/loader.cpp devices/thunderclockplus.cpp
    event_poll.cpp)

# Include SDL2 headers
include_directories(${SDL2_INCLUDE_DIRS} /opt/homebrew/Cellar/sdl2/2.30.10/include)

# Link SDL2 and required frameworks
target_link_libraries(gs2 PRIVATE 
    ${SDL2_LIBRARIES}
    "-framework OpenGL" 
    "-framework Cocoa"
)
