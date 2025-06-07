# tools/CMakeLists.txt Build configuration for buck2-cpp-cpm executable

# Create the main buck2-cpp-cpm executable
add_executable(buck2-cpp-cpm)

# Add source files - use the real main.cpp
target_sources(buck2-cpp-cpm PRIVATE main.cpp)

# Link dependencies - include all CLI components
target_link_libraries(buck2-cpp-cpm PRIVATE buck2-cpp-cpm::core CLI11::CLI11)

# Apply compiler warnings
set_project_warnings(buck2-cpp-cpm)

# Set properties
set_target_properties(
  buck2-cpp-cpm PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
                   OUTPUT_NAME "buck2-cpp-cpm")

# Installation
install(TARGETS buck2-cpp-cpm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
