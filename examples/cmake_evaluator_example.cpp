#include <finch/analyzer/cmake_evaluator.hpp>
#include <finch/parser/parser.hpp>
#include <iostream>

using namespace finch;
using namespace finch::analyzer;

int main() {
    // Example CMake code
    const char* cmake_code = R"(
cmake_minimum_required(VERSION 3.15)
project(MyProject)

# Set some variables
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(BUILD_SHARED_LIBS ON)

# Options
option(ENABLE_TESTS "Enable testing" ON)
option(USE_SANITIZERS "Enable sanitizers" OFF)

# Conditional logic
if(BUILD_SHARED_LIBS)
    set(LIB_TYPE SHARED)
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(LIB_TYPE STATIC)
    set(LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

# List of sources
set(SOURCES
    main.cpp
    utils.cpp
    config.cpp
)
)";

    // Parse the CMake file
    Parser parser(cmake_code, "example.cmake");
    auto parse_result = parser.parse_file();

    if (parse_result.has_error()) {
        std::cerr << "Parse error: " << parse_result.error().message() << std::endl;
        return 1;
    }

    // Evaluate the CMake file
    CMakeFileEvaluator evaluator;
    auto eval_result = evaluator.evaluate_file(*parse_result.value());

    if (eval_result.has_error()) {
        std::cerr << "Evaluation error: " << eval_result.error().message() << std::endl;
        return 1;
    }

    // Print evaluated variables
    std::cout << "=== Evaluated CMake Variables ===" << std::endl;

    auto vars = evaluator.list_variables();
    for (const auto& var_name : vars) {
        auto var = evaluator.get_variable(var_name);
        if (var) {
            std::cout << var_name << " = ";

            // Print value based on type
            std::visit(
                [](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        std::cout << "\"" << v << "\"";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        std::cout << (v ? "ON" : "OFF");
                    } else if constexpr (std::is_same_v<T, double>) {
                        std::cout << v;
                    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                        std::cout << "[";
                        for (size_t i = 0; i < v.size(); ++i) {
                            if (i > 0)
                                std::cout << ", ";
                            std::cout << "\"" << v[i] << "\"";
                        }
                        std::cout << "]";
                    }
                },
                var->value);

            // Show confidence level
            std::cout << " (confidence: ";
            switch (var->confidence) {
            case Confidence::Certain:
                std::cout << "certain";
                break;
            case Confidence::Likely:
                std::cout << "likely";
                break;
            case Confidence::Uncertain:
                std::cout << "uncertain";
                break;
            case Confidence::Unknown:
                std::cout << "unknown";
                break;
            }
            std::cout << ")" << std::endl;
        }
    }

    // Check specific variables
    std::cout << "\n=== Specific Variable Checks ===" << std::endl;

    auto lib_type = evaluator.get_variable("LIB_TYPE");
    if (lib_type) {
        std::cout << "Library type: " << value_helpers::to_string(lib_type->value) << std::endl;
    }

    auto platform = evaluator.get_variable("PLATFORM_NAME");
    if (platform) {
        std::cout << "Platform: " << value_helpers::to_string(platform->value);
        if (platform->confidence != Confidence::Certain) {
            std::cout << " (will need platform select in Buck2)";
        }
        std::cout << std::endl;
    }

    auto sources = evaluator.get_variable("SOURCES");
    if (sources) {
        std::cout << "Source files: " << value_helpers::to_string(sources->value) << std::endl;
    }

    // Cache variables
    std::cout << "\n=== Cache Variables (Options) ===" << std::endl;
    auto enable_tests = evaluator.context().get_cache_variable("ENABLE_TESTS");
    if (enable_tests) {
        std::cout << "ENABLE_TESTS = " << value_helpers::to_string(enable_tests->value)
                  << std::endl;
    }

    auto use_sanitizers = evaluator.context().get_cache_variable("USE_SANITIZERS");
    if (use_sanitizers) {
        std::cout << "USE_SANITIZERS = " << value_helpers::to_string(use_sanitizers->value)
                  << std::endl;
    }

    return 0;
}
