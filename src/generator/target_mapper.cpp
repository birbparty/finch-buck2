#include <algorithm>
#include <finch/analyzer/project_analysis.hpp>
#include <finch/generator/target_mapper.hpp>

namespace finch::generator {

TargetMapper::TargetMapper() = default;
TargetMapper::~TargetMapper() = default;

Result<TargetMapper::MappedTarget, GenerationError>
TargetMapper::map_cmake_target(const analyzer::Target& cmake_target) {
    MappedTarget mapped;
    mapped.name = normalize_target_name(cmake_target.name);
    mapped.rule_type = determine_rule_type(cmake_target);
    mapped.srcs = transform_sources(cmake_target.sources);
    mapped.headers = cmake_target.headers;
    mapped.deps = resolve_dependencies(cmake_target.link_libraries);

    // Set basic properties
    if (!cmake_target.compile_definitions.empty()) {
        std::string defs_str = "[";
        for (size_t i = 0; i < cmake_target.compile_definitions.size(); ++i) {
            defs_str += "\"" + cmake_target.compile_definitions[i] + "\"";
            if (i < cmake_target.compile_definitions.size() - 1) {
                defs_str += ", ";
            }
        }
        defs_str += "]";
        mapped.properties["preprocessor_flags"] = defs_str;
    }

    if (!cmake_target.include_directories.empty()) {
        std::string includes_str = "[";
        for (size_t i = 0; i < cmake_target.include_directories.size(); ++i) {
            includes_str += "\"" + cmake_target.include_directories[i] + "\"";
            if (i < cmake_target.include_directories.size() - 1) {
                includes_str += ", ";
            }
        }
        includes_str += "]";
        mapped.properties["exported_headers"] = includes_str;
    }

    // Handle compiler options
    if (!cmake_target.compile_options.empty()) {
        std::string flags_str = "[";
        for (size_t i = 0; i < cmake_target.compile_options.size(); ++i) {
            flags_str += "\"" + cmake_target.compile_options[i] + "\"";
            if (i < cmake_target.compile_options.size() - 1) {
                flags_str += ", ";
            }
        }
        flags_str += "]";
        mapped.properties["compiler_flags"] = flags_str;
    }

    // Handle linker options for executables
    if (mapped.rule_type == Buck2RuleType::CxxBinary && !cmake_target.link_libraries.empty()) {
        std::string link_flags = "[";
        for (size_t i = 0; i < cmake_target.link_libraries.size(); ++i) {
            link_flags += "\"" + cmake_target.link_libraries[i] + "\"";
            if (i < cmake_target.link_libraries.size() - 1) {
                link_flags += ", ";
            }
        }
        link_flags += "]";
        mapped.properties["linker_flags"] = link_flags;
    }

    return Ok<TargetMapper::MappedTarget, GenerationError>(std::move(mapped));
}

Buck2RuleType TargetMapper::determine_rule_type(const analyzer::Target& target) {
    using TargetType = analyzer::Target::Type;

    if (target.type == TargetType::ExecutableTarget) {
        return Buck2RuleType::CxxBinary;
    } else if (target.type == TargetType::StaticLibrary ||
               target.type == TargetType::SharedLibrary) {
        return Buck2RuleType::CxxLibrary;
    } else if (target.type == TargetType::InterfaceLibrary) {
        return Buck2RuleType::CxxLibrary; // Interface libraries become header-only libraries
    } else if (target.type == TargetType::CustomTarget) {
        return Buck2RuleType::FileGroup;
    }
    return Buck2RuleType::Unknown;
}

std::vector<std::string> TargetMapper::transform_sources(const std::vector<std::string>& sources) {
    std::vector<std::string> transformed;
    transformed.reserve(sources.size());

    for (const auto& source : sources) {
        // Convert relative paths and filter out generated files
        if (source.find("${") == std::string::npos && source.find("$<") == std::string::npos) {
            transformed.push_back(source);
        }
    }

    return transformed;
}

std::vector<std::string> TargetMapper::resolve_dependencies(const std::vector<std::string>& deps) {
    std::vector<std::string> resolved;
    resolved.reserve(deps.size());

    for (const auto& dep : deps) {
        // Convert CMake target names to Buck2 target labels
        if (dep.find("::") != std::string::npos) {
            // External dependency - convert to Buck2 format
            std::string converted = dep;
            std::replace(converted.begin(), converted.end(), ':', '_');
            resolved.push_back("//" + converted);
        } else {
            // Internal dependency
            resolved.push_back(":" + normalize_target_name(dep));
        }
    }

    return resolved;
}

std::string TargetMapper::normalize_target_name(const std::string& cmake_name) {
    std::string normalized = cmake_name;

    // Replace invalid characters with underscores
    for (char& c : normalized) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            c = '_';
        }
    }

    // Ensure it doesn't start with a number
    if (!normalized.empty() && std::isdigit(normalized[0])) {
        normalized = "lib_" + normalized;
    }

    return normalized;
}

} // namespace finch::generator
