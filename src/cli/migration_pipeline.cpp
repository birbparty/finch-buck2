#include <chrono>
#include <filesystem>
#include <finch/analyzer/cmake_evaluator.hpp>
#include <finch/cli/migration_pipeline.hpp>
#include <finch/cli/progress_reporter.hpp>
#include <finch/core/logging.hpp>
#include <finch/core/result.hpp>
#include <finch/generator/generator.hpp>
#include <finch/parser/parser.hpp>
#include <fstream>

namespace finch::cli {

namespace fs = std::filesystem;

MigrationPipeline::MigrationPipeline(const PipelineConfig& config) : config_(config) {
    // Initialize components with placeholder implementations
    // Note: These will be properly initialized when the actual components are ready
}

finch::Result<MigrationPipeline::MigrationResult, MigrationError> MigrationPipeline::execute() {
    auto start_time = std::chrono::steady_clock::now();

    MigrationResult result{.files_processed = 0,
                           .targets_generated = 0,
                           .errors_encountered = 0,
                           .warnings = {},
                           .duration = std::chrono::milliseconds(0)};

    // Phase 1: Discovery
    if (progress_) {
        progress_->start_phase(Phase::Discovery, "Discovering CMake files...");
    }

    auto files_result = discover_cmake_files();
    if (!files_result.has_value()) {
        return finch::Result<MigrationResult, MigrationError>(std::in_place_index<1>,
                                                              files_result.error());
    }

    auto cmake_files = files_result.value();
    if (progress_) {
        progress_->finish_phase(true);
    }

    // Phase 2: Parsing and Analysis
    if (progress_) {
        progress_->start_phase(Phase::Parsing, "Parsing CMake files...");
    }

    analyzer::ProjectAnalysis full_analysis;
    size_t current_file = 0;

    for (const auto& cmake_file : cmake_files) {
        if (progress_) {
            progress_->update_progress(++current_file, cmake_files.size());
            progress_->report_file(cmake_file.string());
        }

        auto file_analysis = process_file(cmake_file);
        if (!file_analysis.has_value()) {
            result.errors_encountered++;
            if (progress_) {
                progress_->report_error(file_analysis.error());
            }
            continue;
        }

        // Merge the file analysis into the full project analysis
        merge_analysis(full_analysis, file_analysis.value());
        result.files_processed++;
    }

    if (progress_) {
        progress_->finish_phase(result.errors_encountered == 0);
    }

    // Phase 3: Generation
    if (progress_) {
        progress_->start_phase(Phase::Generation, "Generating Buck2 files...");
    }

    auto gen_result = generate_buck_files(full_analysis);
    if (!gen_result.has_value()) {
        return finch::Result<MigrationResult, MigrationError>(std::in_place_index<1>,
                                                              gen_result.error());
    }

    if (progress_) {
        progress_->finish_phase(true);
    }

    // Calculate duration
    auto end_time = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return finch::Result<MigrationResult, MigrationError>{result};
}

finch::Result<std::vector<fs::path>, MigrationError> MigrationPipeline::discover_cmake_files() {
    std::vector<fs::path> cmake_files;

    try {
        fs::path source_path(config_.source_directory);

        if (!fs::exists(source_path)) {
            return finch::Result<std::vector<fs::path>, MigrationError>(
                std::in_place_index<1>,
                MigrationError(MigrationErrorKind::FileSystemError,
                               "Source directory does not exist: " + source_path.string()));
        }

        // Find all CMakeLists.txt and *.cmake files
        for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                if (path.filename() == "CMakeLists.txt" || path.extension() == ".cmake") {
                    cmake_files.push_back(path);
                }
            }
        }

        if (cmake_files.empty()) {
            return finch::Result<std::vector<fs::path>, MigrationError>(
                std::in_place_index<1>,
                MigrationError(MigrationErrorKind::FileSystemError,
                               "No CMake files found in " + source_path.string()));
        }

        return finch::Result<std::vector<fs::path>, MigrationError>{cmake_files};

    } catch (const fs::filesystem_error& e) {
        return finch::Result<std::vector<fs::path>, MigrationError>(
            std::in_place_index<1>, MigrationError(MigrationErrorKind::FileSystemError,
                                                   "Filesystem error: " + std::string(e.what())));
    }
}

finch::Result<analyzer::ProjectAnalysis, MigrationError>
MigrationPipeline::process_file(const fs::path& cmake_file) {
    // Read file content
    std::ifstream file(cmake_file);
    if (!file.is_open()) {
        return finch::Result<analyzer::ProjectAnalysis, MigrationError>(
            std::in_place_index<1>, MigrationError(MigrationErrorKind::FileSystemError,
                                                   "Cannot open file: " + cmake_file.string()));
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Create a placeholder implementation for now
    analyzer::ProjectAnalysis analysis;
    analysis.project_name = "parsed_project";
    analysis.project_version = "1.0.0";

    return finch::Result<analyzer::ProjectAnalysis, MigrationError>{analysis};
}

finch::Result<void, MigrationError>
MigrationPipeline::generate_buck_files(const analyzer::ProjectAnalysis& analysis) {
    // Create a placeholder implementation for now
    // In a real implementation, this would use the generator

    return finch::Result<void, MigrationError>{};
}

void MigrationPipeline::merge_analysis(analyzer::ProjectAnalysis& target,
                                       const analyzer::ProjectAnalysis& source) {
    // Merge project name and version (keep first encountered)
    if (target.project_name.empty() && !source.project_name.empty()) {
        target.project_name = source.project_name;
    }
    if (target.project_version.empty() && !source.project_version.empty()) {
        target.project_version = source.project_version;
    }

    // Merge targets
    target.targets.insert(target.targets.end(), source.targets.begin(), source.targets.end());

    // Merge global variables
    for (const auto& [key, value] : source.global_variables) {
        target.global_variables[key] = value;
    }

    // Merge cache variables
    for (const auto& [key, value] : source.cache_variables) {
        target.cache_variables[key] = value;
    }

    // Merge warnings
    target.warnings.insert(target.warnings.end(), source.warnings.begin(), source.warnings.end());
}

} // namespace finch::cli
