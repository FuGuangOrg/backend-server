cmake_minimum_required(VERSION 3.20)

project(fuguang-server VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories to match the original .vcxproj structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators like Visual Studio
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${config})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${config})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${config})
endforeach()

# Options
option(FUGUANG_ENABLE_INFERENCE "Enable AI inference capabilities" OFF)

# Qt setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Xml)

# Find vcpkg packages - make them optional for flexibility
find_package(asio CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET) 
find_package(spdlog CONFIG QUIET)
find_package(OpenCV CONFIG QUIET)
find_package(pugixml CONFIG QUIET)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBMODBUS libmodbus)
endif()

# CLI11 might be found differently depending on vcpkg version
find_package(CLI11 CONFIG QUIET)
if(NOT CLI11_FOUND)
    find_path(CLI11_INCLUDE_DIRS "CLI/CLI.hpp")
endif()

# Check for essential dependencies
set(MISSING_DEPS "")
if(NOT Qt6_FOUND)
    list(APPEND MISSING_DEPS "Qt6")
endif()
if(NOT OpenCV_FOUND)
    list(APPEND MISSING_DEPS "OpenCV")
endif()
if(NOT spdlog_FOUND)
    list(APPEND MISSING_DEPS "spdlog")
endif()

if(MISSING_DEPS)
    message(WARNING "Missing dependencies: ${MISSING_DEPS}")
    message(STATUS "Consider installing via vcpkg or system package manager")
    message(STATUS "Example: vcpkg install asio nlohmann-json spdlog opencv4 pugixml concurrentqueue libmodbus cli11")
endif()

# Global include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add subdirectories for each project
add_subdirectory(src/common)
add_subdirectory(src/event_bus) 
add_subdirectory(src/device_enum)
add_subdirectory(src/device_camera)
add_subdirectory(src/motion_control)
add_subdirectory(src/auto_focus)
add_subdirectory(src/fiber_end_server)

# Add inspection-algo if available and inference is enabled
if(FUGUANG_ENABLE_INFERENCE AND EXISTS ${CMAKE_SOURCE_DIR}/third_party/inspection-algo/CMakeLists.txt)
    add_subdirectory(third_party/inspection-algo)
endif()

# Print configuration summary
message(STATUS "FuGuang Server Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Inference Enabled: ${FUGUANG_ENABLE_INFERENCE}")
message(STATUS "  Qt Version: ${Qt6_VERSION}")
message(STATUS "  OpenCV Version: ${OpenCV_VERSION}")