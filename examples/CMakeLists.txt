# examples/CMakeLists.txt Build configuration for finch examples

# Function to add an example
function(add_finch_example name)
  add_executable(${name} ${ARGN})

  target_link_libraries(${name} PRIVATE finch::core CLI11::CLI11)

  # Apply warnings
  set_project_warnings(${name})

  # Set output directory
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                           "${CMAKE_BINARY_DIR}/examples")

  # Optionally install examples
  install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
                                  OPTIONAL)
endfunction()

# Add examples as they are created Example usage:
# add_finch_example(simple_conversion simple_conversion.cpp)
# add_finch_example(analyze_project analyze_project.cpp)
# add_finch_example(custom_rules custom_rules.cpp)

# Create a stub example to test the build
file(
  WRITE "${CMAKE_CURRENT_BINARY_DIR}/hello_finch.cpp"
  "#include <iostream>
#include <finch/version.h>

int main() {
    std::cout << \"Hello from finch \" << finch::VERSION_STRING << \"!\\n\";
    std::cout << \"Built with \" << finch::CXX_COMPILER << \" (C++\" << finch::CXX_STANDARD << \")\\n\";
    return 0;
}
")

add_finch_example(hello_finch "${CMAKE_CURRENT_BINARY_DIR}/hello_finch.cpp")

# Real examples
add_finch_example(error_handling_example error_handling_example.cpp)
add_finch_example(logging_example logging_example.cpp)
add_finch_example(simple_logging_example simple_logging_example.cpp)
add_finch_example(parser_example parser_example.cpp)
add_finch_example(cmake_evaluator_example cmake_evaluator_example.cpp)
