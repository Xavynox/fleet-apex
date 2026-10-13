cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project definition
project(fleet-apex VERSION 0.2.0 LANGUAGES CXX)

# Default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options for the mod
option(FLEET_APEX_DEBUG_LOGS "Enable debug logging" OFF)
option(FLEET_APEX_ENABLE_PROFILING "Enable performance profiling" ON)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR FLEET_APEX_DEBUG_LOGS)
    add_compile_definitions(ENABLE_DEBUG_LOGS)
endif()

# Core engine sources (must match actual files)
set(APEX_CORE_SOURCES
    src/Engine/Profiler.cpp
    src/Engine/LevelAnalyzer.cpp
    src/Engine/AdaptiveEngine.cpp
    src/Engine/BottleneckClassifier.cpp
    src/Engine/CPULiberationEngine.cpp
    src/Engine/GPUAssistEngine.cpp
    src/Engine/FleetCore.cpp
)

# Header files
set(APEX_CORE_HEADERS
    include/Engine/Profiler.h
    include/Engine/LevelAnalyzer.h
    include/Engine/AdaptiveEngine.h
    include/Engine/BottleneckClassifier.h
    include/Engine/CPULiberationEngine.h
    include/Engine/GPUAssistEngine.h
    include/Engine/FleetCore.h
    include/APEXMod.h
)

# Mod sources
set(MOD_SOURCES
    src/main.cpp
)

# Add the engine library
add_library(fleet-apex-core STATIC ${APEX_CORE_SOURCES} ${APEX_CORE_HEADERS})

target_include_directories(fleet-apex-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Engine
)

# Main executable
add_executable(fleet-apex ${MOD_SOURCES})

target_link_libraries(fleet-apex PRIVATE fleet-apex-core)

# Include the core headers for the main file
target_include_directories(fleet-apex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Engine
)

# Geode SDK integration
# The Geode SDK provides CMake configuration that must be included
# GEODE_SDK should be set by the build environment (GitHub Actions, local install)
if(NOT GEODE_SDK)
    message(STATUS "GEODE_SDK not set - attempting SDK discovery")
    # Try common locations
    find_path(GEODE_SDK
        NAMESPACE GeodeSDK
        HINTS
        /usr/local/geode
        /opt/geode
        $ENV{HOME}/geode
    )
endif()

if(GEODE_SDK AND NOT GEODE_SDK_INITIALIZED)
    # Include the Geode SDK CMake configuration
    # This provides Geode-specific build targets and setup
    if(EXISTS "${GEODE_SDK}/CMakeLists.txt")
        add_subdirectory(${GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
    elseif(EXISTS "${HOME}/.local/geode/CMakeLists.txt")
        add_subdirectory("${HOME}/.local/geode" ${CMAKE_CURRENT_BINARY_DIR}/geode)
    endif()
    
    # Mark as initialized so we don't re-add
    set(GEODE_SDK_INITIALIZED TRUE CACHE BOOL "Geode SDK has been initialized")
endif()

# Configure the mod target using Geode's setup function
# This must be called after SDK integration to properly configure the mod
setup_geode_mod(fleet-apex)

# Platform-specific settings for Android
if(ANDROID)
    target_compile_definitions(fleet-apex PRIVATE ANDROID=1)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        add_compile_definitions(ANDROID_ARM64=1)
    endif()
endif()

# Install target
install(TARGETS fleet-apex
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
