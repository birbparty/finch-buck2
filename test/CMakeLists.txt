# test/CMakeLists.txt Build configuration for finch tests

# Create test executable
add_executable(finch-tests)

# Add test sources
target_sources(
  finch-tests
  PRIVATE dummy_test.cpp # Temporary test file
          # Core tests
          core/error_handling_test.cpp
          core/logging_test.cpp
          # Integration tests
          integration/otel_filesystem_test.cpp
          # Parser tests
          parser/lexer_test.cpp
          parser/parser_test.cpp
          parser/cpm_parser_test.cpp
          # Analyzer tests
          analyzer/cmake_evaluator_test.cpp
          # Add test files here as they are created Example:
          # unit/analyzer/dependency_analyzer_test.cpp
          # unit/generator/buck2_generator_test.cpp
          # integration/full_conversion_test.cpp Main test runner (will be
          # created) test_main.cpp
)

# Link test dependencies
target_link_libraries(finch-tests PRIVATE finch::core GTest::gtest
                                          GTest::gtest_main GTest::gmock)

# Apply compiler warnings to tests
set_project_warnings(finch-tests)

# Set test properties
set_target_properties(finch-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                             "${CMAKE_BINARY_DIR}/test/bin")

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(
  finch-tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
  PROPERTIES LABELS "unit")

# Add custom target for running tests with output
add_custom_target(
  run-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running finch tests...")

# Coverage target if enabled
if(FINCH_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Add coverage flags
    target_compile_options(finch-tests PRIVATE --coverage)
    target_link_options(finch-tests PRIVATE --coverage)

    # Also add to main library
    target_compile_options(finch-core PRIVATE --coverage)
    target_link_options(finch-core PRIVATE --coverage)

    # Add custom target for coverage report
    add_custom_target(
      coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o
              ${CMAKE_BINARY_DIR}/coverage/index.html
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating coverage report...")
  else()
    message(WARNING "Coverage is only supported with GCC or Clang")
  endif()
endif()
