# Dependencies.cmake Manages external dependencies using FetchContent

include(FetchContent)

# Set FetchContent to be quiet by default
set(FETCHCONTENT_QUIET ON)

# Allow users to provide their own versions of dependencies
option(FINCH_USE_SYSTEM_DEPS
       "Use system-installed dependencies instead of FetchContent" OFF)

# ------------------------------------------------------------------------------
# fmt - Modern formatting library
# ------------------------------------------------------------------------------
if(NOT FINCH_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
    GIT_SHALLOW TRUE)

  # Disable fmt tests
  set(FMT_TEST
      OFF
      CACHE BOOL "" FORCE)
  set(FMT_DOC
      OFF
      CACHE BOOL "" FORCE)
  set(FMT_INSTALL
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(fmt REQUIRED)
endif()

# ------------------------------------------------------------------------------
# spdlog - Fast C++ logging library
# ------------------------------------------------------------------------------
if(NOT FINCH_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
    GIT_SHALLOW TRUE)

  # Configure spdlog
  set(SPDLOG_FMT_EXTERNAL
      ON
      CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_EXAMPLE
      OFF
      CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(SPDLOG_INSTALL
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(spdlog REQUIRED)
endif()

# ------------------------------------------------------------------------------
# CLI11 - Command line parser
# ------------------------------------------------------------------------------
if(NOT FINCH_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.1
    GIT_SHALLOW TRUE)

  # CLI11 is header-only, so we don't need to build anything
  set(CLI11_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(CLI11_BUILD_EXAMPLES
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(CLI11 REQUIRED)
endif()

# ------------------------------------------------------------------------------
# nlohmann_json - JSON for Modern C++
# ------------------------------------------------------------------------------
if(NOT FINCH_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)

  # Disable json tests
  set(JSON_BuildTests
      OFF
      CACHE BOOL "" FORCE)
  set(JSON_Install
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(nlohmann_json REQUIRED)
endif()

# ------------------------------------------------------------------------------
# GoogleTest - Unit testing framework (only if tests are enabled)
# ------------------------------------------------------------------------------
if(FINCH_BUILD_TESTS)
  if(NOT FINCH_USE_SYSTEM_DEPS)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.14.0
      GIT_SHALLOW TRUE)

    # For Windows: Prevent overriding the parent project's compiler/linker
    # settings
    set(gtest_force_shared_crt
        ON
        CACHE BOOL "" FORCE)

    # Disable install for googletest
    set(INSTALL_GTEST
        OFF
        CACHE BOOL "" FORCE)
  else()
    find_package(GTest REQUIRED)
  endif()
endif()

# ------------------------------------------------------------------------------
# Make dependencies available
# ------------------------------------------------------------------------------
if(NOT FINCH_USE_SYSTEM_DEPS)
  # Download and make available all declared dependencies
  FetchContent_MakeAvailable(fmt spdlog CLI11 nlohmann_json)

  if(FINCH_BUILD_TESTS)
    FetchContent_MakeAvailable(googletest)
  endif()

  # Disable warnings for third-party code
  if(TARGET fmt)
    disable_warnings(fmt)
  endif()

  if(TARGET spdlog)
    disable_warnings(spdlog)
  endif()
endif()

# ------------------------------------------------------------------------------
# Create interface targets for easier usage
# ------------------------------------------------------------------------------

# Note: Dependencies are linked directly in target CMakeLists.txt files This
# ensures proper export handling and avoids circular dependencies

# Print dependency information
function(print_dependency_info)
  message(STATUS "")
  message(STATUS "Dependencies:")
  message(STATUS "  Using system dependencies: ${FINCH_USE_SYSTEM_DEPS}")
  if(NOT FINCH_USE_SYSTEM_DEPS)
    message(STATUS "  fmt:           10.2.1")
    message(STATUS "  spdlog:        1.13.0")
    message(STATUS "  CLI11:         2.4.1")
    message(STATUS "  nlohmann_json: 3.11.3")
    if(FINCH_BUILD_TESTS)
      message(STATUS "  GoogleTest:    1.14.0")
    endif()
  endif()
  message(STATUS "")
endfunction()

# Call the function at the end
print_dependency_info()
