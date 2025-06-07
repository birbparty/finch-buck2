#include <filesystem>
#include <finch/analyzer/project_analysis.hpp>
#include <finch/generator/generator.hpp>
#include <finch/generator/rule_templates.hpp>
#include <finch/generator/starlark_writer.hpp>
#include <finch/generator/target_mapper.hpp>
#include <fstream>
#include <map>
#include <set>

namespace finch::generator {

namespace fs = std::filesystem;

Generator::Generator(const Config& config)
    : target_mapper_(std::make_unique<TargetMapper>()),
      template_registry_(std::make_unique<TemplateRegistry>()), config_(config) {}

Generator::~Generator() = default;

Result<Generator::GenerationResult, GenerationError>
Generator::generate(const analyzer::ProjectAnalysis& analysis) {
    GenerationResult result;
    result.targets_processed = 0;

    // Group targets by directory and generate BUCK files
    std::map<fs::path, std::vector<analyzer::Target>> targets_by_dir;
    for (const auto& target : analysis.targets) {
        targets_by_dir[target.source_directory].push_back(target);
    }

    // Generate BUCK file for each directory
    for (const auto& [dir, targets] : targets_by_dir) {
        fs::path output_path = config_.output_directory / "BUCK";
        if (targets_by_dir.size() > 1) {
            // Multiple directories - create BUCK files in subdirectories
            fs::path relative_dir = fs::relative(dir, fs::current_path());
            output_path = config_.output_directory / relative_dir / "BUCK";
        }

        auto buck_file_result = generate_buck_file(output_path, targets);
        if (!buck_file_result) {
            return Result<GenerationResult, GenerationError>(std::in_place_index<1>,
                                                             buck_file_result.error());
        }

        result.generated_files.push_back(output_path);
        result.targets_processed += targets.size();
    }

    // Generate .buckconfig
    auto config_result = generate_buckconfig(analysis);
    if (!config_result) {
        return Result<GenerationResult, GenerationError>(std::in_place_index<1>,
                                                         config_result.error());
    }

    result.generated_files.push_back(config_.output_directory / ".buckconfig");

    return Result<GenerationResult, GenerationError>{std::move(result)};
}

Result<void, GenerationError>
Generator::generate_buck_file(const fs::path& output_path,
                              const std::vector<analyzer::Target>& targets) {
    StarlarkWriter writer(true);

    // Add load statements - collect all needed symbols
    std::set<std::string> needed_symbols;
    for (const auto& target : targets) {
        auto mapped_result = target_mapper_->map_cmake_target(target);
        if (!mapped_result) {
            return Result<void, GenerationError>::error(mapped_result.error());
        }

        const auto& mapped = mapped_result.value();
        if (mapped.rule_type == Buck2RuleType::CxxLibrary) {
            needed_symbols.insert("cxx_library");
        } else if (mapped.rule_type == Buck2RuleType::CxxBinary) {
            needed_symbols.insert("cxx_binary");
        } else if (mapped.rule_type == Buck2RuleType::CxxTest) {
            needed_symbols.insert("cxx_test");
        }
    }

    if (!needed_symbols.empty()) {
        std::vector<std::string> symbols(needed_symbols.begin(), needed_symbols.end());
        writer.add_load("@prelude//cxx:cxx.bzl", symbols);
    }

    writer.add_blank_line();

    for (const auto& target : targets) {
        auto mapped_result = target_mapper_->map_cmake_target(target);
        if (!mapped_result) {
            return Result<void, GenerationError>::error(mapped_result.error());
        }

        const auto& mapped = mapped_result.value();
        const auto* rule_template = template_registry_->get_template(mapped.rule_type);
        if (!rule_template) {
            return Result<void, GenerationError>::error(GenerationError(
                GenerationError::Category::MissingTemplate, "No template found for rule type"));
        }

        // Use the template to generate the rule
        std::string rule_content = rule_template->generate(mapped);
        writer.add_rule(rule_content);
    }

    std::string content = writer.generate();
    return write_file(output_path, content);
}

Result<void, GenerationError>
Generator::generate_buckconfig(const analyzer::ProjectAnalysis& analysis) {
    fs::path config_path = config_.output_directory / ".buckconfig";

    std::string config_content = R"([buildfile]
name = BUCK

[parser]
polyglot_parsing_enabled = true
default_build_file_syntax = STARLARK

[project]
ide = vscode

[cxx]
default_platform = //toolchains:cxx
cxxflags = -std=c++20
cxxppflags = -Wall -Wextra

[repositories]
prelude = buck2/prelude
toolchains = toolchains
)";

    return write_file(config_path, config_content);
}

Result<void, GenerationError> Generator::write_file(const fs::path& path,
                                                    const std::string& content) {
    if (config_.dry_run) {
        return Ok<GenerationError>();
    }

    try {
        // Create directories if they don't exist
        fs::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file) {
            return Result<void, GenerationError>::error(
                GenerationError(GenerationError::Category::FileWriteError,
                                "Failed to open file for writing: " + path.string()));
        }

        file << content;
        return Ok<GenerationError>();
    } catch (const std::exception& e) {
        return Result<void, GenerationError>::error(
            GenerationError(GenerationError::Category::FileWriteError,
                            "Failed to write file: " + std::string(e.what())));
    }
}

} // namespace finch::generator
