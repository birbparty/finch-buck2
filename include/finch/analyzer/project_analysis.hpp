#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace finch::analyzer {

// Represents a CMake target (library, executable, etc.)
struct Target {
    enum class Type {
        StaticLibrary,
        SharedLibrary,
        ExecutableTarget,
        InterfaceLibrary,
        CustomTarget,
        Unknown
    };

    std::string name;
    Type type = Type::Unknown;
    std::filesystem::path source_directory;
    std::vector<std::string> sources;
    std::vector<std::string> headers;
    std::vector<std::string> include_directories;
    std::vector<std::string> compile_definitions;
    std::vector<std::string> compile_options;
    std::vector<std::string> link_libraries;
    std::unordered_map<std::string, std::string> properties;
};

// Represents the analysis results for a CMake project
struct ProjectAnalysis {
    std::string project_name;
    std::string project_version;
    std::vector<Target> targets;
    std::unordered_map<std::string, std::string> global_variables;
    std::unordered_map<std::string, std::string> cache_variables;
    std::vector<std::string> warnings;
};

} // namespace finch::analyzer
