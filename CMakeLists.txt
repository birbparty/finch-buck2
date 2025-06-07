cmake_minimum_required(VERSION 3.20)

# Project definition
project(
  buck2-cpp-cpm
  VERSION 0.1.0
  DESCRIPTION "CMake to Buck2 migration tool"
  LANGUAGES CXX)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds are not allowed. Please create a separate build directory."
  )
endif()

# Set project-wide C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

# Options
option(BUCK2_CPM_BUILD_TESTS "Build test suite" ON)
option(BUCK2_CPM_BUILD_EXAMPLES "Build examples" ON)
option(BUCK2_CPM_BUILD_DOCS "Build documentation" OFF)
option(BUCK2_CPM_ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(BUCK2_CPM_ENABLE_SANITIZERS "Enable sanitizers in Debug builds" ON)
option(BUCK2_CPM_ENABLE_WARNINGS "Enable compiler warnings" OFF)
option(BUCK2_CPM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(BUCK2_CPM_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(BUCK2_CPM_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)

# Set module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include helper modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CompilerWarnings)
include(Sanitizers)
include(Dependencies)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Generate version header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/finch/version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/finch/version.h" @ONLY)

# Create main library target
add_library(buck2-cpp-cpm-core)
add_library(buck2-cpp-cpm::core ALIAS buck2-cpp-cpm-core)

# Set library properties
target_include_directories(
  buck2-cpp-cpm-core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Apply compiler warnings if requested
if(BUCK2_CPM_ENABLE_WARNINGS)
  set_project_warnings(buck2-cpp-cpm-core)
endif()

# Apply sanitizers if requested
if(BUCK2_CPM_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  enable_sanitizers(buck2-cpp-cpm-core)
endif()

# Add subdirectories
add_subdirectory(src)

if(BUCK2_CPM_BUILD_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(test)
endif()

if(BUCK2_CPM_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

add_subdirectory(tools)

# Installation rules (temporarily disabled due to dependency export issues)
# TODO: Re-enable once we have a proper solution for exporting FetchContent
# dependencies
if(FALSE)
  install(
    TARGETS finch-core
    EXPORT finch-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    DIRECTORY include/finch
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/finch/version.h"
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/finch)

  # Export targets
  install(
    EXPORT finch-targets
    FILE finch-targets.cmake
    NAMESPACE finch::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finch)

  # Create and install package configuration files
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FinchConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FinchConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finch)

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FinchConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/FinchConfig.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/FinchConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finch)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Finch Contributors")
set(CPACK_PACKAGE_CONTACT "https://github.com/punk1290/buck2-cpp-cpm")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Generator-specific settings
set(CPACK_GENERATOR "TGZ;ZIP")
if(UNIX AND NOT APPLE)
  list(APPEND CPACK_GENERATOR "DEB" "RPM")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "buck2-cpp-cpm ${PROJECT_VERSION} Configuration Summary:")
message(STATUS "  Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:        ${CMAKE_CXX_STANDARD}")
message(
  STATUS
    "  Compiler:            ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
)
message(STATUS "")
message(STATUS "  Build tests:         ${BUCK2_CPM_BUILD_TESTS}")
message(STATUS "  Build examples:      ${BUCK2_CPM_BUILD_EXAMPLES}")
message(STATUS "  Build docs:          ${BUCK2_CPM_BUILD_DOCS}")
message(STATUS "  Enable coverage:     ${BUCK2_CPM_ENABLE_COVERAGE}")
message(STATUS "  Enable sanitizers:   ${BUCK2_CPM_ENABLE_SANITIZERS}")
message(STATUS "  Enable warnings:     ${BUCK2_CPM_ENABLE_WARNINGS}")
message(STATUS "  Warnings as errors:  ${BUCK2_CPM_WARNINGS_AS_ERRORS}")
message(STATUS "")
message(STATUS "  Install prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
