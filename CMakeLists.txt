cmake_minimum_required(VERSION 3.10)

# Set the project name
project(gssquared)

# Set default to static for our internal libraries
set(BUILD_SHARED_LIBS OFF)

# Mac-specific settings
if(APPLE)
    set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")

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
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/MacOS
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Frameworks
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gs2> ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/MacOS/
        COMMAND ${CMAKE_COMMAND} -E copy ${APP_ICON_MACOSX} ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/roms ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources/roms
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources/img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assets/img/atlas.png ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Resources/img/atlas.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/Info.plist ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/
        # Copy SDL dynamic libraries
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Frameworks/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3_image.dylib ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Frameworks/
        # Fix up runtime paths with install_name_tool if needed
        COMMAND install_name_tool -change @rpath/libSDL3.0.dylib @executable_path/../Frameworks/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/MacOS/gs2
        COMMAND install_name_tool -change @rpath/libSDL3_image.0.dylib @executable_path/../Frameworks/libSDL3_image.dylib ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/MacOS/gs2
        COMMAND install_name_tool -change @rpath/libSDL3.0.dylib @executable_path/../Frameworks/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/GSSquared.app/Contents/Frameworks/libSDL3_image.dylib
        DEPENDS gs2
    )
    add_custom_target(mac-cli-package
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/packages/mac-cli/resources/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/mac-cli/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/roms ${CMAKE_BINARY_DIR}/packages/mac-cli/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gs2> ${CMAKE_BINARY_DIR}/packages/mac-cli/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/mac-cli/resources/img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assets/img/atlas.png ${CMAKE_BINARY_DIR}/packages/mac-cli/resources/img/atlas.png
        # Copy SDL dynamic libraries
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/mac-cli/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/mac-cli/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3_image.dylib ${CMAKE_BINARY_DIR}/packages/mac-cli/lib/
        # Fix up runtime paths
        COMMAND install_name_tool -change @rpath/libSDL3.0.dylib @executable_path/lib/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/mac-cli/gs2
        COMMAND install_name_tool -change @rpath/libSDL3_image.0.dylib @executable_path/lib/libSDL3_image.dylib ${CMAKE_BINARY_DIR}/packages/mac-cli/gs2
        COMMAND install_name_tool -change @rpath/libSDL3.0.dylib @executable_path/lib/libSDL3.dylib ${CMAKE_BINARY_DIR}/packages/mac-cli/lib/libSDL3_image.dylib
        DEPENDS gs2
    )
    
    # Add DMG creation target
    add_custom_target(create-dmg
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/tmp/dmg_contents
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tmp/dmg_contents
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/packages/GSSquared.app ${CMAKE_BINARY_DIR}/tmp/dmg_contents/GSSquared.app
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_BINARY_DIR}/tmp/dmg_contents/README.md
        COMMAND hdiutil create -volname "GSSquared" -srcfolder ${CMAKE_BINARY_DIR}/tmp/dmg_contents -ov -format UDZO ${CMAKE_BINARY_DIR}/packages/GSSquared.dmg
        DEPENDS mac-package
    )
    
    # Custom target for development environment resources
    add_custom_target(mac-dev ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/roms ${CMAKE_BINARY_DIR}/resources/roms
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources/img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assets/img/atlas.png ${CMAKE_BINARY_DIR}/resources/img/atlas.png
        DEPENDS gs2
    )
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    # Linux-specific configuration here
    
    # Find the SDL libraries regardless of their exact filenames
    file(GLOB SDL3_LIBS "${CMAKE_SOURCE_DIR}/lib/libSDL3.so*")
    file(GLOB SDL3_IMAGE_LIBS "${CMAKE_SOURCE_DIR}/lib/libSDL3_image.so*")
    
    # Custom target for creating Linux package
    add_custom_target(linux-package
        # Delete contents of any current resources directory
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources

        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/linux-cli/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/roms ${CMAKE_BINARY_DIR}/packages/linux-cli/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gs2> ${CMAKE_BINARY_DIR}/packages/linux-cli/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/linux-cli/resources/img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assets/img/atlas.png ${CMAKE_BINARY_DIR}/packages/linux-cli/resources/img/atlas.png
        # Copy SDL dynamic libraries with correct names
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/packages/linux-cli/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3.so.0 ${CMAKE_BINARY_DIR}/packages/linux-cli/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/libSDL3_image.so.0 ${CMAKE_BINARY_DIR}/packages/linux-cli/lib/
        # Create necessary symlinks
        COMMAND ${CMAKE_COMMAND} -E create_symlink libSDL3.so.0 ${CMAKE_BINARY_DIR}/packages/linux-cli/lib/libSDL3.so
        COMMAND ${CMAKE_COMMAND} -E create_symlink libSDL3_image.so.0 ${CMAKE_BINARY_DIR}/packages/linux-cli/lib/libSDL3_image.so
        # Fix up runtime paths with chrpath
        COMMAND chrpath -r '$$ORIGIN/lib' ${CMAKE_BINARY_DIR}/packages/linux-cli/gs2
        COMMAND chrpath -r '$$ORIGIN' ${CMAKE_BINARY_DIR}/packages/linux-cli/lib/libSDL3_image.so.0
        DEPENDS gs2
    )
    # Custom target for development environment resources
    add_custom_target(linux-dev ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources/roms
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/roms ${CMAKE_BINARY_DIR}/resources/roms
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources/img
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assets/img/atlas.png ${CMAKE_BINARY_DIR}/resources/img/atlas.png
        DEPENDS gs2
    )
endif()

function(configure_sdl_builds)
    set(BUILD_SHARED_LIBS ON)
    
    # Force SDL to build with the correct SDK
    if(APPLE)
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk" CACHE PATH "SDK path" FORCE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Deployment target" FORCE)
    endif()
    
    set(SDL_SHARED ON)
    
    # Force rebuild SDL with these settings
    add_subdirectory(${CMAKE_SOURCE_DIR}/vendored/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(${CMAKE_SOURCE_DIR}/vendored/SDL_image EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS OFF)
endfunction()

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

configure_sdl_builds()

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Define flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -ggdb3 -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# When using sanitizers, we need to link with them too
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

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

add_library(gs2_devices_game src/devices/game/gamecontroller.cpp src/devices/game/mousewheel.cpp )

add_dependencies(gs2_devices_prodos_block prodos_block_firmware)

target_include_directories(gs2_devices_prodos_block PRIVATE
    ${CMAKE_SOURCE_DIR}/src/devices/prodos_block
)

add_library(gs2_devices_diskii_fmt
    src/devices/diskii/diskii_fmt.cpp
    src/devices/diskii/diskii_fmt.cpp
)

add_library(gs2_devices_pdblock2 src/devices/pdblock2/pdblock2.cpp )

add_library(gs2_devices_prodos_clock src/devices/prodos_clock/prodos_clock.cpp )

add_library(gs2_devices_tcp    src/devices/thunderclock_plus/thunderclockplus.cpp )

add_library(gs2_devices_languagecard     src/devices/languagecard/languagecard.cpp )

add_library(gs2_devices_keyboard     src/devices/keyboard/keyboard.cpp )

add_library(gs2_devices_speaker     src/devices/speaker/speaker.cpp )

add_library(gs2_devices_memexp     src/devices/memoryexpansion/memexp.cpp )

add_library(gs2_devices_parallel     src/devices/parallel/parallel.cpp )

add_library(gs2_cpu src/cpus/cpu_6502.cpp src/cpus/cpu_65c02.cpp )

add_library(gs2_util src/util/media.cpp src/util/ResourceFile.cpp src/util/dialog.cpp src/util/mount.cpp src/util/reset.cpp )

add_library(gs2_ui src/ui/AssetAtlas.cpp src/ui/Container.cpp src/ui/DiskII_Button.cpp src/ui/Unidisk_Button.cpp 
    src/ui/MousePositionTile.cpp src/ui/OSD.cpp src/ui/Tile.cpp src/ui/Button.cpp src/ui/MainAtlas.cpp
)

target_include_directories(gs2_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendored/SDL_image/include
)

# Add the executable
add_executable(gs2 src/gs2.cpp src/bus.cpp src/clock.cpp src/debug.cpp src/cpu.cpp src/memory.cpp src/opcodes.cpp src/test.cpp 
    src/display/text_40x24.cpp src/display/lores_40x48.cpp src/display/hgr_280x192.cpp src/display/display.cpp
    src/display/displayng.cpp src/display/ntsc.cpp src/display/font.cpp src/display/hgr.cpp src/display/Matrix3x3.cpp
    src/devices/loader.cpp 
    src/devices/diskii/diskii.cpp
    src/platforms.cpp
    src/event_poll.cpp
    src/devices.cpp src/slots.cpp src/systemconfig.cpp
    )

# Set up common include directories for all targets
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendored/SDL/include
    /usr/local/include
)

# Link SDL3 and required frameworks
target_link_libraries(gs2 PRIVATE 
    SDL3::SDL3-shared
    SDL3_image::SDL3_image-shared
)

if(APPLE)
    target_link_libraries(gs2 PRIVATE
        "-framework OpenGL" 
        "-framework Cocoa"
    )
endif()

target_link_libraries(gs2 
    PUBLIC gs2_cpu 
    gs2_devices_tcp 
    gs2_devices_keyboard 
    gs2_devices_diskii_fmt 
    gs2_devices_languagecard 
    gs2_devices_prodos_block
    gs2_devices_prodos_clock
    gs2_devices_speaker
    gs2_devices_game
    gs2_devices_memexp
    gs2_devices_parallel
    gs2_devices_pdblock2
    gs2_util
    gs2_ui
)

add_subdirectory(apps/nibblizer)

add_subdirectory(apps/diskid)

# Update the executable's include directories to remove redundant paths
target_include_directories(gs2 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendored/SDL/include
    ${CMAKE_SOURCE_DIR}/vendored/SDL_image/include
)

# Meta-target that builds all packages
if(APPLE)
    add_custom_target(packages
        DEPENDS mac-package mac-cli-package create-dmg mac-dev
    )
elseif(UNIX AND NOT APPLE)
    add_custom_target(packages
        DEPENDS linux-package linux-dev
    )
else()
    add_custom_target(packages
        COMMENT "No package targets defined for this platform"
    )
endif()
